#pragma once

#include <expected>
#include <string>
#include <vector>

#include "Expression.h"
#include "Parsed.h"

struct Constructor
{
    std::string name;
    Expression type;
};

struct InductiveType
{
    std::string name;
    Expression type;
    std::vector<Constructor> constructors;
};

std::expected<InductiveType, std::string> create_inductive_type(ParsedInductiveType p);
std::expected<Constructor, std::string> create_constructor(ParsedConstructor p);
std::string to_pretty_string(InductiveType const& p);
