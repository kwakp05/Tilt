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
    std::string type_name;
    std::string name;
    Expression type;

    Constructor(std::string type_name, std::string name, Expression&& type)
        : type_name(std::move(type_name)), name(std::move(name)), type(std::move(type)) {}
};

struct InductiveType
{
    std::string name;
    Expression type;
    std::vector<Constructor> constructors;
    std::vector<NamedExpression> parameters;
    Recursor rec;
    Recursor rec1;
};

std::expected<InductiveType, std::string> create_inductive_type(ParsedInductiveType p);
std::expected<Constructor, std::string> create_constructor(ParsedConstructor p, std::string type_name);
Recursor create_recursor(std::string const& name, std::vector<NamedExpression> const& parameters, std::vector<Constructor> const& constructors, int motive_universe);

Constructor clone(Constructor const& c);
Recursor clone(Recursor const& rec);

std::string to_pretty_string(InductiveType const& p);
std::string to_pretty_string(Constructor const& p);
std::string to_pretty_string(Recursor const& rec);
