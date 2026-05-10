#include <algorithm>
#include <expected>
#include <format>
#include <functional>
#include <iostream>
#include <optional>
#include <ranges>
#include <string>
#include <utility>
#include <variant>

#include "Constant.h"
#include "Engine.h"
#include "Expression.h"
#include "IdentifierMap.h"
#include "InductiveType.h"
#include "Parsed.h"
#include "Reducer.h"
#include "VariantUtils.h"

std::expected<void, Engine::ErrorType> Engine::process(ParsedInductiveType p)
{
    std::string identifier{ p.identifier.identifier };
    if (identifiers.contains(identifier))
        return std::unexpected("'" + identifier + "' has already been declared");
    return create_inductive_type(p)
        .transform([this, &identifier](InductiveType&& type)
        {
                for (Constructor const& c : type.constructors)
                {
                    identifiers.insert(std::format("{}.{}", identifier, c.name), clone(c));
                }
                identifiers.insert(std::format("{}.rec", identifier), clone(type.rec));
                identifiers.insert(identifier, std::move(type));

        });
}

std::expected<std::string, Engine::ErrorType> Engine::process(ParsedCheckCommand p)
{
    std::optional<Expression> exp;
    return create_expression(p.expression)
        .and_then([&exp, this](Expression&& e)
            {
                exp = std::move(e);
                return get_type(*exp);
            })
        .transform([&exp, this](Expression&& type_exp)
        {
                return std::format("{} : {}", to_pretty_string(*exp), to_pretty_string(type_exp));
        });
}

std::expected<void, Engine::ErrorType> Engine::process(ParsedConstant p)
{
    std::string identifier{ p.identifier };
    if (identifiers.contains(identifier))
        return std::unexpected("'" + identifier + "' has already been declared");
    return create_constant(p)
        .and_then([this](Constant&& c) -> std::expected<Constant, std::string>
            {
                if (auto value_type = get_type(c.value); !value_type)
                {
                    return std::unexpected(value_type.error());
                }
                else if (value_type != c.type)
                {
                    return std::unexpected(std::format(
                        "Type mismatch\n  {}\nhas type\n  {}\nbut is expected to have type\n  {}",
                        to_pretty_string(c.value),
                        to_pretty_string(*value_type),
                        to_pretty_string(c.type)
                    ));
                }
                return std::move(c);
            })
        .transform([this, &identifier](Constant&& constant)
            {
                identifiers.insert(identifier, std::move(constant));
            });
}

std::optional<IdentifierReferenceType> Engine::scope_find(std::string const& s) const
{
    return identifiers.scope_find(s);
}

std::expected<Expression, std::string> Engine::get_type(Expression const& exp) const
{
    return ::get_type(exp, identifiers);
}

namespace
{

struct BoundArgument
{
    std::string name;
    Expression type;
};

class IdentifierMapWrapper
{
public:
    using ValueType = combine_variants_t<IdentifierReferenceType, std::variant<std::reference_wrapper<BoundArgument const>>>;

    explicit IdentifierMapWrapper(IdentifierMap const& wrapped) : wrapped(wrapped) {}

    std::optional<ValueType> scope_find(std::string const& identifier) const
    {
        auto matches = [&identifier](auto const& x) { return x == identifier; };
        if (auto result = std::ranges::find_if(bound_arguments, matches, &BoundArgument::name); result != bound_arguments.end())
            return *result;

        return wrapped.scope_find(identifier)
            .transform([](IdentifierReferenceType t)
                {
                    return std::visit([](auto&& x) -> ValueType { return std::cref(x); }, t);
                });
    }

    void emplace_bound_argument(BoundArgument const& arg)
    {
        bound_arguments.emplace_back(arg);
    }

    void emplace_bound_argument(BoundArgument&& arg) = delete;

    void erase(std::string const& identifier)
    {
        auto matches = [&identifier](auto const& x) { return x == identifier; };
        if (auto result = std::ranges::find_if(bound_arguments, matches, &BoundArgument::name); result != bound_arguments.end())
            bound_arguments.erase(result);
    }

private:
    IdentifierMap const& wrapped;
    std::vector<std::reference_wrapper<BoundArgument const>> bound_arguments;
};

class ScopedBoundArgument
{
public:
    explicit ScopedBoundArgument(IdentifierMapWrapper& identifiers, auto&&... args) :
        identifiers(identifiers),
        arg(std::forward<decltype(args)>(args)...)
    {
        identifiers.emplace_bound_argument(arg);
    }

    ~ScopedBoundArgument()
    {
        identifiers.erase(arg.name);
    }

    ScopedBoundArgument(ScopedBoundArgument const& other) = delete;
    ScopedBoundArgument(ScopedBoundArgument&& other) = delete;
    ScopedBoundArgument& operator=(ScopedBoundArgument const& other) = delete;
    ScopedBoundArgument& operator=(ScopedBoundArgument&& other) = delete;

private:
    IdentifierMapWrapper& identifiers;
    BoundArgument arg;
};

std::expected<Expression, std::string> get_type_helper(Expression const& exp, IdentifierMapWrapper& identifiers);

std::expected<Expression, std::string> get_type_helper(Constructor const& value, IdentifierMapWrapper& identifiers)
{
    std::optional<std::reference_wrapper<InductiveType const>> inductive_type = identifiers.scope_find(value.type_name)
        .and_then([](auto&& v) -> std::optional<std::reference_wrapper<InductiveType const>>
            {
                if (auto ptr = std::get_if<std::reference_wrapper<InductiveType const>>(&v))
                    return *ptr;
                return {};
            });

    if (inductive_type)
    {
        std::vector<NamedExpression> terms = inductive_type->get().parameters
            | std::views::transform([](NamedExpression const& x) { return clone(x); })
            | std::ranges::to<std::vector<NamedExpression>>();
        terms.emplace_back("", clone(value.type));
        return create_function_from_terms(terms);
    }

    return std::unexpected(std::format(
        "unable to compute type of constructor {}, '{}' does not reference an inductive type",
        value.name,
        value.type_name
    ));
}

std::expected<Expression, std::string> get_type_helper(InductiveType const& value, IdentifierMapWrapper& identifiers)
{
    std::vector<NamedExpression> terms = value.parameters
        | std::views::transform([](NamedExpression const& x) { return clone(x); })
        | std::ranges::to<std::vector<NamedExpression>>();
    terms.emplace_back("", clone(value.type));

    return create_function_from_terms(terms);
}

std::expected<Expression, std::string> get_type_helper(Recursor const& value, IdentifierMapWrapper& identifiers)
{
    return clone(value.type);
}

std::expected<Expression, std::string> get_type_helper(Constant const& value, IdentifierMapWrapper& identifiers)
{
    return clone(value.type);
}

std::expected<Expression, std::string> get_type_helper(BoundArgument const& value, IdentifierMapWrapper& identifiers)
{
    return clone(value.type);
}

std::expected<Expression, std::string> get_type_helper(IdentifierValueType const& value, IdentifierMapWrapper& identifiers)
{
    return std::visit([&identifiers](auto&& x) { return get_type_helper(x, identifiers); }, value);
}

std::expected<Expression, std::string> get_type_helper(Universe const& universe, IdentifierMapWrapper& identifiers)
{
    return Expression{ Universe{universe.level + 1} };
}

std::expected<Expression, std::string> get_type_helper(Identifier const& identifier, IdentifierMapWrapper& identifiers)
{
    auto joined_view = identifier.components | std::views::join_with('.');
    std::string full_identifier = std::string{ joined_view.begin(), joined_view.end() };
    if (auto resolved = identifiers.scope_find(full_identifier))
    {
        return std::visit([&identifiers](auto&& resolved) -> std::expected<Expression, std::string>
            {
                return get_type_helper(resolved, identifiers);
            }, *resolved);
    }
    return std::unexpected("invalid identifier " + full_identifier);
}

std::expected<Expression, std::string> get_type_helper(FunctionApplication const& application, IdentifierMapWrapper& identifiers)
{
    auto function_type = get_type_helper(*application.function, identifiers)
        .and_then([](Expression&& function_type) -> std::expected<Function, std::string>
            {
                if (auto ptr = std::get_if<Function>(&function_type))
                    return std::move(*ptr);
                return std::unexpected("Function expected but this expression has type " + to_pretty_string(function_type));
            });

    if (!function_type)
        return std::unexpected(function_type.error());

    return get_type_helper(*application.argument, identifiers)
        .and_then([&function_type, &application](Expression&& argument_type) -> std::expected<Expression, std::string>
            {
                if (argument_type != *function_type->param_type)
                    return std::unexpected(std::format(
                        "Application type mismatch: The argument\n  {}\nhas type\n  {}\nbut is expected to have type\n  {}",
                        to_pretty_string(*application.argument),
                        to_pretty_string(argument_type),
                        to_pretty_string(*function_type->param_type)
                    ));
                return std::move(delta_reduce(
                    *function_type->return_type,
                    { function_type->param_name },
                    *application.argument
                ));
            });
}

std::expected<Expression, std::string> get_type_helper(FunctionAbstraction const& abstraction, IdentifierMapWrapper& identifiers)
{
    ScopedBoundArgument guard{ identifiers, abstraction.param_name, clone(*abstraction.param_type) };
    return get_type_helper(*abstraction.return_value, identifiers)
        .transform([&abstraction, &identifiers](Expression&& exp)
            {
                return Expression{ Function{
                    .param_name = abstraction.param_name,
                    .param_type = std::make_unique<Expression>(clone(*abstraction.param_type)),
                    .return_type = std::make_unique<Expression>(std::move(exp))
                }};
            });
}

std::expected<Expression, std::string> get_type_helper(Function const& function, IdentifierMapWrapper& identifiers)
{
    auto expect_type = [&identifiers](Expression const& e)
        {
            return get_type_helper(e, identifiers)
                .and_then([&e](Expression&& type) -> std::expected<Universe, std::string>
                    {
                        if (auto u = std::get_if<Universe>(&type))
                            return std::move(*u);
                        return std::unexpected(std::format("type expected, got\n  ({} : {})", to_pretty_string(e), to_pretty_string(type)));
                    });
        };

    std::optional<Universe> param_type;

    return expect_type(*function.param_type)
        .transform([&param_type](Universe&& u) { param_type = u; })
        .and_then([&function, &expect_type, &identifiers]() {
                ScopedBoundArgument arg{ identifiers, function.param_name, clone(*function.param_type) };
                return expect_type(*function.return_type);
            })
        .transform([&param_type](Universe&& return_type)
            {
                if (return_type.level == 0)
                    return Universe{0};
                return Universe{std::max(param_type->level, return_type.level)};

            });
}

std::expected<Expression, std::string> get_type_helper(Expression const& exp, IdentifierMapWrapper& identifiers)
{
    return std::visit([&identifiers](auto&& x) -> std::expected<Expression, std::string>
        {
            return get_type_helper(x, identifiers);
        }, exp);
}

}

std::expected<Expression, std::string> get_type(Expression const& exp, IdentifierMap const& identifiers)
{
    IdentifierMapWrapper identifiers_wrapper{ identifiers };
    return get_type_helper(exp, identifiers_wrapper);
}
