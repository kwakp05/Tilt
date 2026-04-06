#pragma once

#include <expected>
#include <string>
#include <unordered_map>
#include <variant>

#include "IdentifierMap.h"
#include "InductiveType.h"
#include "Parsed.h"

class Engine
{
public:
    using ErrorType = std::string;

    std::expected<void, ErrorType> process(ParsedInductiveType p);
    std::expected<std::string, ErrorType> process(ParsedCheckCommand p);
    IdentifierValueType const* scope_find(std::string const& s) const;
    std::expected<Expression, std::string> get_type(Expression const& p) const;

private:
    IdentifierMap identifiers;
};

std::expected<Expression, std::string> get_type(Expression const& p, IdentifierMap const& identifiers);
