#pragma once

#include <memory>
#include <string>
#include <variant>

#include "Parsed.h"

struct Expression;

struct Identifier
{
    std::string name;
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
    std::variant<Identifier, Function, FunctionAbstraction, FunctionApplication> value;
};

Expression create_expression(ParsedExpression p);
Expression get_type(Expression p);

