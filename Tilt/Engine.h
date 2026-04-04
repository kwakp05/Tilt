#pragma once

#include <expected>
#include <string>
#include <unordered_map>
#include <variant>

#include "InductiveType.h"
#include "Parsed.h"

using IdentifierValueType = std::variant<InductiveType>;
using IdentifierMap = std::unordered_map<std::string, IdentifierValueType>;

class Engine
{
public:
    using ErrorType = std::string;

    std::expected<void, ErrorType> process(ParsedInductiveType p);
    std::expected<std::string, ErrorType> process(ParsedCheckCommand p);
    IdentifierValueType const* find_identifier(std::string const& s) const;
    std::expected<Expression, std::string> get_type(Expression const& p) const;

private:
    IdentifierMap identifiers;
};

std::expected<Expression, std::string> get_type(Expression const& p, IdentifierMap const& identifiers);
std::expected<Expression, std::string> get_type(IdentifierValueType const& value, IdentifierMap const& identifiers);
