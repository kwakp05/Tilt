#pragma once

#include <string>
#include <unordered_map>

#include "Parsed.h"

class Engine
{
public:
    void process(ParsedInductiveType p);

private:
    std::unordered_map<std::string, ParsedInductiveType> identifiers;
};
