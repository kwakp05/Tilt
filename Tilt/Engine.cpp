#include <expected>
#include <format>
#include <iostream>
#include <optional>
#include <ranges>
#include <string>
#include <variant>

#include "Engine.h"
#include "Expression.h"
#include "IdentifierMap.h"
#include "InductiveType.h"
#include "Parsed.h"
#include "Scope.h"

std::expected<void, Engine::ErrorType> Engine::process(ParsedInductiveType p)
{
    std::string identifier{ p.identifier.identifier };
    if (identifiers.contains(identifier))
        return std::unexpected("'" + identifier + "' has already been declared");
    return create_inductive_type(p)
        .transform([this, &identifier](InductiveType&& type)
        {
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

IdentifierValueType const* Engine::scope_find(std::string const& s) const
{
    return identifiers.scope_find(s);
}

std::expected<Expression, std::string> Engine::get_type(Expression const& exp) const
{
    return ::get_type(exp, identifiers);
}

namespace
{
std::expected<Expression, std::string> get_type(Constructor const& value, IdentifierMap const& identifiers)
{
    return clone(value.type);
}

std::expected<Expression, std::string> get_type(InductiveType const& value, IdentifierMap const& identifiers)
{
    return clone(value.type);
}

std::expected<Expression, std::string> get_type(IdentifierValueType const& value, IdentifierMap const& identifiers)
{
    return std::visit([&identifiers](auto&& x) { return get_type(x, identifiers); }, value);
}

std::expected<Expression, std::string> get_type(Identifier const& identifier, IdentifierMap const& identifiers)
{
    auto joined_view = identifier.components | std::views::join_with('.');
    std::string full_identifier = std::string{ joined_view.begin(), joined_view.end() };
    if (auto resolved = resolve_identifier<IdentifierMap, InductiveType, Constructor>(identifier.components, identifiers))
    {
        return std::visit([&full_identifier, &identifiers](auto&& resolved) -> std::expected<Expression, std::string>
            {
                using U = std::remove_cvref_t<decltype(resolved.get())>;
                if constexpr (std::is_same_v<U, IdentifierMap>)
                    return std::unexpected("invalid identifier " + full_identifier);
                else
                    return get_type(resolved, identifiers);
            }, *resolved);
    }
    return std::unexpected("invalid identifier " + full_identifier);
}
}

std::expected<Expression, std::string> get_type(Expression const& exp, IdentifierMap const& identifiers)
{
    return std::visit([&identifiers](auto&& x) -> std::expected<Expression, std::string>
        {
            using T = std::remove_cvref_t<decltype(x)>;
            if constexpr (std::is_same_v<T, Universe>)
                return Expression{ Identifier{ {"TYPE"} } };
            else if constexpr (std::is_same_v<T, Identifier>)
                return get_type(x, identifiers);
            else if constexpr (std::is_same_v<T, Function>)
                return Expression{ Identifier{ {"TYPE"} } };
            else if constexpr (std::is_same_v<T, FunctionAbstraction>)
                return Expression{ Identifier{ {"TYPE"} } };
            else if constexpr (std::is_same_v<T, FunctionApplication>)
                return Expression{ Identifier{ {"TYPE"} } };
            else
                static_assert(false, "UNREACHABLE");
        }, exp.value);
}
