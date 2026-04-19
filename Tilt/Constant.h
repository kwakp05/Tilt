#pragma once

#include <expected>
#include <string>

#include "Expression.h"

struct Constant
{
    std::string name;
    Expression type;
    Expression value;
};

std::expected<Constant, std::string> create_constant(ParsedConstant p);
std::string to_pretty_string(Constant const& p);
