#pragma once

#include <algorithm>
#include <concepts>
#include <expected>
#include <generator>
#include <memory>
#include <ranges>
#include <span>
#include <stdexcept>
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

struct NamedExpressionView
{
    std::string_view name;
    Expression const& exp;
};

struct NamedExpression
{
    std::string name;
    Expression exp;

    NamedExpression(std::string name, Expression exp) : name(std::move(name)), exp(std::move(exp)) {}
    explicit NamedExpression(NamedExpressionView const& other);
};

template<typename R>
concept ExpressionInputRange = std::ranges::input_range<R> && std::same_as<std::ranges::range_value_t<R>, Expression>;

template<typename R>
concept NamedExpressionInputRange = std::ranges::input_range<R> && std::same_as<std::ranges::range_value_t<R>, NamedExpression>;

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
NamedExpression clone(NamedExpression const& exp);
Universe clone(Universe const& universe);
Identifier clone(Identifier const& identifier);
Function clone(Function const& function);
FunctionAbstraction clone(FunctionAbstraction const& abstraction);
FunctionApplication clone(FunctionApplication const& application);
std::generator<NamedExpressionView> get_function_args(Expression const& function);

Expression create_function_from_terms(NamedExpressionInputRange auto&& terms)
{
    if (std::ranges::empty(terms))
        throw std::runtime_error("Cannot call create_function_from_terms with empty range");

    std::vector<NamedExpression> vec;
    std::ranges::move(terms, std::back_inserter(vec));
    auto rev_terms = std::views::reverse(vec);

    Expression result = std::move(std::ranges::begin(rev_terms)->exp);
    for (NamedExpression& named_exp : rev_terms | std::views::drop(1))
    {
        result = Function{
            .param_name = std::move(named_exp.name),
            .param_type = std::make_unique<Expression>(std::move(named_exp.exp)),
            .return_type = std::make_unique<Expression>(std::move(result))
        };
    }
    return result;
}

Expression create_function_application_from_terms(ExpressionInputRange auto&& terms)
{
    if (std::ranges::empty(terms))
        throw std::runtime_error("Cannot call create_function_application_from_terms with empty range");

    Expression result = std::move(*std::ranges::begin(terms));
    for (Expression& exp : terms | std::views::drop(1))
    {
        result = FunctionApplication{
            .function = std::make_unique<Expression>(std::move(result)),
            .argument = std::make_unique<Expression>(std::move(exp))
        };
    }
    return result;
}

bool operator==(Function const& lhs, Function const& rhs);
bool operator==(FunctionAbstraction const& lhs, FunctionAbstraction const& rhs);
bool operator==(FunctionApplication const& lhs, FunctionApplication const& rhs);
