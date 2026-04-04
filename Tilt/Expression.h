#pragma once

#include <expected>
#include <memory>
#include <span>
#include <string>
#include <variant>
#include <vector>

#include "Parsed.h"

struct Expression;

struct Universe
{
    int level;
};

struct Identifier
{
    std::vector<std::string> components;
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
    std::unique_ptr<Expression> return_type;
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

struct Expression
{
    std::variant<Universe, Identifier, Function, FunctionAbstraction, FunctionApplication> value;
};

std::expected<Expression, std::string> create_expression(ParsedExpression p);
std::expected<Expression, std::string> create_expression(std::span<ExpressionToken> tokens);
std::string to_pretty_string(Expression const& p);
Expression clone(Expression const& exp);

