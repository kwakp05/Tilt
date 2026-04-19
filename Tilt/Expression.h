#pragma once

#include <expected>
#include <memory>
#include <span>
#include <string>
#include <variant>
#include <vector>

#include "Parsed.h"

struct Universe;
struct Identifier;
struct Function;
struct FunctionAbstraction;
struct FunctionApplication;

using Expression = std::variant<Universe, Identifier, Function, FunctionAbstraction, FunctionApplication>;

struct Universe
{
    int level;

    bool operator==(Universe const&) const = default;
};

struct Identifier
{
    std::vector<std::string> components;

    bool operator==(Identifier const&) const = default;
};

struct Function
{
    std::string param_name;
    std::unique_ptr<Expression> param_type;
    std::unique_ptr<Expression> return_type;
};

struct FunctionAbstraction
{
    std::string param_name;
    std::unique_ptr<Expression> param_type;
    std::unique_ptr<Expression> return_value;
};

struct FunctionApplication
{
    std::unique_ptr<Expression> function;
    std::unique_ptr<Expression> argument;
};

template <class T>
concept FunctionComponent =
    std::same_as<T, Identifier>
    || std::same_as<T, Function>
    || std::same_as<T, FunctionAbstraction>
    || std::same_as<T, FunctionApplication>;

std::expected<Expression, std::string> create_expression(ParsedExpression p);
std::expected<Expression, std::string> create_expression(std::span<ExpressionToken> tokens);
std::string to_pretty_string(Expression const& p);
Expression clone(Expression const& exp);

bool operator==(Function const& lhs, Function const& rhs);
bool operator==(FunctionAbstraction const& lhs, FunctionAbstraction const& rhs);
bool operator==(FunctionApplication const& lhs, FunctionApplication const& rhs);
