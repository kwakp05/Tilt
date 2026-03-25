#pragma once

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

InductiveType create_inductive_type(ParsedInductiveType p);
Constructor create_constructor(ParsedConstructor p);
