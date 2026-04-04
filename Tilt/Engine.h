#pragma once

#include <expected>
#include <string>
#include <unordered_map>
#include <variant>

#include "InductiveType.h"
#include "Parsed.h"

class Engine
{
public:
    using ErrorType = std::string;
    using ValueType = std::variant<InductiveType>;

    std::expected<void, ErrorType> process(ParsedInductiveType p);
    std::expected<std::string, ErrorType> process(ParsedCheckCommand p);
    ValueType const* find_identifier(std::string const& s) const;

private:
    std::unordered_map<std::string, ValueType> identifiers;
};

