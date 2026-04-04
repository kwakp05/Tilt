#include <expected>
#include <format>
#include <iostream>
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
    return create_expression(p.expression)
        .transform([](Expression&& exp)
        {
                return std::format("{} : {}", to_pretty_string(exp), to_pretty_string(get_type(exp)));
        });
}

