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
    identifiers[identifier] = create_inductive_type(p);
    return {};
}

std::expected<std::string, Engine::ErrorType> Engine::process(ParsedCheckCommand p)
{

    Expression term = create_expression(p.expression);
    return std::format("{} : {}", "HI", "HI");
}

