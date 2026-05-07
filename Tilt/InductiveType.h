#pragma once

#include <expected>
#include <string>
#include <vector>

#include "Expression.h"
#include "Parsed.h"

struct Recursor
{
    Expression type;
};

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
    Recursor rec;
};

std::expected<InductiveType, std::string> create_inductive_type(ParsedInductiveType p);
std::expected<Constructor, std::string> create_constructor(ParsedConstructor p);
Recursor create_recursor(std::string const& name, std::vector<Constructor> const& constructors, int motive_universe);

Constructor clone(Constructor const& c);
Recursor clone(Recursor const& rec);

std::string to_pretty_string(InductiveType const& p);
std::string to_pretty_string(Constructor const& p);
std::string to_pretty_string(Recursor const& rec);
