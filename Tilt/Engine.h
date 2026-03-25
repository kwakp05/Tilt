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

    std::expected<void, ErrorType> process(ParsedInductiveType p);
    std::expected<std::string, ErrorType> process(ParsedCheckCommand p);

private:
    std::unordered_map<std::string, std::variant<InductiveType>> identifiers;
};

