#pragma once

#include <string>
#include <vector>

#include "Expression.h"

Expression delta_reduce(Expression const& target, std::vector<std::string> const& identifier, Expression const& constant);
