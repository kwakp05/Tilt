#include <expected>
#include <format>
#include <iostream>
#include <optional>
#include <string>

#include "Engine.h"
#include "Expression.h"
#include "InductiveType.h"
#include "Parsed.h"

std::expected<void, Engine::ErrorType> Engine::process(ParsedInductiveType p)
{
    std::string identifier{ p.identifier.identifier };
    if (identifiers.contains(identifier))
        return std::unexpected("'" + identifier + "' has already been declared");
    return create_inductive_type(p)
        .transform([this, &identifier](InductiveType&& type)
        {
                identifiers[identifier] = std::move(type);
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

std::variant<InductiveType> const* Engine::find_identifier(std::string const& s) const
{
    if (auto it = identifiers.find(s); it != identifiers.end())
        return &it->second;
    return nullptr;
}

std::expected<Expression, std::string> Engine::get_type(Expression const& exp) const
{
    return ::get_type(exp, identifiers);
}

std::expected<Expression, std::string> get_type(Expression const& exp, IdentifierMap const& identifiers)
{
    return std::visit([&identifiers](auto&& x) -> std::expected<Expression, std::string>
        {
            using T = std::remove_cvref_t<decltype(x)>;
            if constexpr (std::is_same_v<T, Universe>)
                return Expression{ Identifier{ {"TYPE"} } };
            else if constexpr (std::is_same_v<T, Identifier>)
            {
                if (auto it = identifiers.find(x.components[0]); it != identifiers.end())
                    return get_type(it->second, identifiers);
                return std::unexpected("invalid identifier " + x.components[0]);
            }
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

std::expected<Expression, std::string> get_type(IdentifierValueType const& value, IdentifierMap const& identifiers)
{
    return std::visit([](auto&& x)
        {
            using T = std::remove_cvref_t<decltype(x)>;
            if constexpr (std::is_same_v<T, InductiveType>)
                return clone(x.type);
            else
                static_assert(false, "UNREACHABLE");
        }, value);
}
