#pragma once

#include <expected>
#include <string>
#include <vector>

#include "Expression.h"
#include "IdentifierMap.h"

Expression delta_reduce(Expression const& target, std::vector<std::string> const& identifier, Expression const& constant);
std::expected<Expression, std::string> reduce(Expression const& target, IdentifierMap const& identifiers);

struct ReducedResult
{
    bool result;
    Expression lhs_reduced;
    Expression rhs_reduced;
};

std::expected<ReducedResult, std::string> definitionally_equivalent(Expression const& lhs, Expression const& rhs, IdentifierMap const& identifiers);
std::expected<ReducedResult, std::string> definitionally_equivalent(Expression const& lhs, Expression const& rhs, IdentifierMapWrapper& identifiers);
