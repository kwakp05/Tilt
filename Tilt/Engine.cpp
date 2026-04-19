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
    return clone(value.type);
}

std::expected<Expression, std::string> get_type_helper(InductiveType const& value, IdentifierMapWrapper& identifiers)
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

std::expected<Expression, std::string> get_type_helper(Expression const& exp, IdentifierMapWrapper& identifiers)
{
    return std::visit([&identifiers](auto&& x) -> std::expected<Expression, std::string>
        {
            using T = std::remove_cvref_t<decltype(x)>;
            if constexpr (std::is_same_v<T, Universe>)
                return Expression{ Identifier{ {"TYPE"} } };
            else if constexpr (std::is_same_v<T, Identifier>)
                return get_type_helper(x, identifiers);
            else if constexpr (std::is_same_v<T, Function>)
                return Expression{ Identifier{ {"TYPE"} } };
            else if constexpr (std::is_same_v<T, FunctionAbstraction>)
                return get_type_helper(x, identifiers);
            else if constexpr (std::is_same_v<T, FunctionApplication>)
                return get_type_helper(x, identifiers);
            else
                static_assert(false, "UNREACHABLE");
        }, exp);
}

}

std::expected<Expression, std::string> get_type(Expression const& exp, IdentifierMap const& identifiers)
{
    IdentifierMapWrapper identifiers_wrapper{ identifiers };
    return get_type_helper(exp, identifiers_wrapper);
}
