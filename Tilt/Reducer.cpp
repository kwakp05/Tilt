#include <expected>
#include <format>
#include <memory>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "Expression.h"
#include "IdentifierMap.h"
#include "Logger.h"
#include "Reducer.h"

Expression delta_reduce(Expression const& target, std::vector<std::string> const& identifier, Expression const& constant)
{
    return std::visit([&constant, &identifier](auto&& exp) -> Expression
        {
            using T = std::remove_cvref_t<decltype(exp)>;
            if constexpr (std::is_same_v<T, Universe>)
                return exp;
            else if constexpr (std::is_same_v<T, Identifier>)
            {
                if (exp.components == identifier)
                    return clone(constant);
                else
                    return exp;
            }
            else if constexpr (std::is_same_v<T, Function>)
            {
                return Function{
                    .param_name=exp.param_name,
                    .param_type=std::make_unique<Expression>(delta_reduce(*exp.param_type, identifier, constant)),
                    .return_type=std::make_unique<Expression>(delta_reduce(*exp.return_type, identifier, constant)),
                };
            }
            else if constexpr (std::is_same_v<T, FunctionAbstraction>)
            {
                return FunctionAbstraction{
                    .param_name=exp.param_name,
                    .param_type=std::make_unique<Expression>(delta_reduce(*exp.param_type, identifier, constant)),
                    .return_value=std::make_unique<Expression>(delta_reduce(*exp.return_value, identifier, constant)),
                };
            }
            else if constexpr (std::is_same_v<T, FunctionApplication>)
            {
                return FunctionApplication{
                    .function=std::make_unique<Expression>(delta_reduce(*exp.function, identifier, constant)),
                    .argument=std::make_unique<Expression>(delta_reduce(*exp.argument, identifier, constant)),
                };
            }
            else
                static_assert(false, "UNREACHABLE");
        }, target);
}

namespace
{
std::expected<Expression, std::string> reduce(Expression const& target, IdentifierMapWrapper& identifiers);

std::expected<Expression, std::string> reduce_helper(Universe const& target, IdentifierMapWrapper& identifiers)
{
    return target;
}

std::expected<Expression, std::string> reduce_helper(Identifier const& target, IdentifierMapWrapper& identifiers)
{
    std::string identifier{ target.components | std::views::join_with('.') | std::ranges::to<std::string>() };
    if (auto value = identifiers.scope_find(identifier); value)
    {
        return std::visit([&target, &identifiers](auto&& ref_wrapped) -> std::expected<Expression, std::string>
            {
                auto& x = ref_wrapped.get();
                using T = std::remove_cvref_t<decltype(x)>;
                if constexpr (std::is_same_v<T, Constant>) // Delta reduce
                    return reduce(x.value, identifiers);
                else if constexpr (std::is_same_v<T, SubstitutedIdentifier>) // Beta reduce
                    return reduce(x.value, identifiers);
                else
                    return target;
            }, *value);
    }
    return std::unexpected(std::format("Unknown identifier '{}'", identifier));
}

std::expected<Expression, std::string> reduce_helper(Function const& target, IdentifierMapWrapper& identifiers)
{
    auto reduced_param_type = reduce(*target.param_type, identifiers);
    if (!reduced_param_type)
        return std::unexpected(reduced_param_type.error());

    auto guard = identifiers.emplace_bound_identifier(target.param_name, clone(*reduced_param_type));
    auto reduced_return_type = reduce(*target.return_type, identifiers);
    if (!reduced_return_type)
        return std::unexpected(reduced_return_type.error());

    return Function{
        .param_name = target.param_name,
        .param_type = std::make_unique<Expression>(std::move(*reduced_param_type)),
        .return_type = std::make_unique<Expression>(std::move(*reduced_return_type))
    };
}

std::expected<Expression, std::string> reduce_helper(FunctionAbstraction const& target, IdentifierMapWrapper& identifiers)
{
    return clone(target);
}

Expression const& get_leftmost_application(Expression const& target)
{
    if (auto application_ptr = std::get_if<FunctionApplication>(&target); application_ptr)
        return get_leftmost_application(*application_ptr->function);
    return target;
}

template <class T>
std::add_pointer_t<T const> identifier_lookup_helper(Expression const& target, IdentifierMapWrapper const& identifiers)
{
    if (auto identifier_ptr = std::get_if<Identifier>(&target))
        return identifiers.get_if<T>(identifier_ptr->components | std::views::join_with('.') | std::ranges::to<std::string>());
    return nullptr;
}

Recursor const* get_if_recursor(Expression const& target, IdentifierMapWrapper const& identifiers)
{
    return identifier_lookup_helper<Recursor>(target, identifiers);
}

Constructor const* get_if_constructor(Expression const& target, IdentifierMapWrapper const& identifiers)
{
    return identifier_lookup_helper<Constructor>(target, identifiers);
}

std::expected<Expression, std::string> reduce_helper(FunctionApplication const& target, IdentifierMapWrapper& identifiers)
{
    auto reduced_function = reduce(*target.function, identifiers);
    if (!reduced_function)
        return std::unexpected(reduced_function.error());

    auto reduced_argument = reduce(*target.argument, identifiers);
    if (!reduced_argument)
        return std::unexpected(reduced_argument.error());

    TILT_LOG_DEBUG(std::format("Reduced function: {}", to_pretty_string(*reduced_function)));
    TILT_LOG_DEBUG(std::format("Reduced argument: {}", to_pretty_string(*reduced_argument)));

    if (auto abstraction_ptr = std::get_if<FunctionAbstraction>(&*reduced_function); abstraction_ptr)
    {
        // Beta reduce
        auto guard = identifiers.emplace_substituted_identifier(abstraction_ptr->param_name, clone(*reduced_argument));
        return reduce(*abstraction_ptr->return_value, identifiers);
    }

    Recursor const* recursor = get_if_recursor(get_leftmost_application(*reduced_function), identifiers);
    Constructor const* constructor = get_if_constructor(get_leftmost_application(*reduced_argument), identifiers);

    if (recursor && constructor)
    {
        // Iota reduce
        InductiveType const* inductive_type = identifiers.get_if<InductiveType>(constructor->type_name);
        if (!inductive_type)
            return std::unexpected(std::format("BUG: Unable to find type '{}' for constructor '{}'", constructor->type_name, constructor->name));

        std::vector<Constructor> const& constructors = inductive_type->constructors;
        size_t constructor_index = 0;
        if (auto it = std::ranges::find(constructors, constructor->name, &Constructor::name); it == constructors.end())
            return std::unexpected(std::format("BUG: Unable to find constructor '{}' for type '{}'", constructor->name, constructor->type_name));
        else
            constructor_index = std::distance(constructors.begin(), it);

        size_t constructor_term_index = constructor_index + 2;
        auto recursor_terms = get_application_terms(*target.function) | std::views::drop(constructor_term_index);
        if (auto itr = std::ranges::begin(recursor_terms); itr != recursor_terms.end())
        {
            Expression const& minor_premise = *itr;
            std::vector<Expression> result_terms;
            result_terms.emplace_back(clone(minor_premise));
            result_terms.append_range(
                get_application_terms(*reduced_argument)
                | std::views::drop(1)
                | std::views::transform([](Expression const& x) { return clone(x); })
            );
            return reduce(create_function_application_from_terms(result_terms), identifiers);
        }

        return std::unexpected(std::format("BUG: Unable to get index '{}' of '{}' recursor for constructor '{}'", constructor_term_index, constructor->type_name, constructor->name));
    }

    return FunctionApplication{
        .function = std::make_unique<Expression>(std::move(*reduced_function)),
        .argument = std::make_unique<Expression>(std::move(*reduced_argument)),
    };
}

std::expected<Expression, std::string> reduce(Expression const& target, IdentifierMapWrapper& identifiers)
{
    TILT_LOG_DEBUG(std::format("Reducing {}", to_pretty_string(target)));
    return std::visit([&identifiers](auto const& exp) { return reduce_helper(exp, identifiers); }, target);
}
}

std::expected<Expression, std::string> reduce(Expression const& target, IdentifierMap const& identifiers)
{
    IdentifierMapWrapper wrapper{ identifiers };
    return reduce(target, wrapper);
}

std::expected<ReducedResult, std::string> definitionally_equivalent(Expression const& lhs, Expression const& rhs, IdentifierMap const& identifiers)
{
    IdentifierMapWrapper wrapper{ identifiers };
    return definitionally_equivalent(lhs, rhs, wrapper);
}

std::expected<ReducedResult, std::string> definitionally_equivalent(Expression const& lhs, Expression const& rhs, IdentifierMapWrapper& identifiers)
{
    auto lhs_reduced = reduce(lhs, identifiers);
    if (!lhs_reduced)
        return std::unexpected(lhs_reduced.error());

    auto rhs_reduced = reduce(rhs, identifiers);
    if (!rhs_reduced)
        return std::unexpected(rhs_reduced.error());

    return ReducedResult{
        .result = *lhs_reduced == *rhs_reduced,
        .lhs_reduced = std::move(*lhs_reduced),
        .rhs_reduced = std::move(*rhs_reduced)
    };
}
