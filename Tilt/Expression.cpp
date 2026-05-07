#include <cassert>
#include <expected>
#include <format>
#include <generator>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include "Expression.h"
#include "Parsed.h"
#include "StringLiteral.h"

NamedExpression::NamedExpression(NamedExpressionView const& other)
    : name(std::string{ other.name }), exp(clone(other.exp)) {}

namespace
{

Identifier create_identifier(ParsedHierarchicalIdentifier p)
{
    return Identifier{
        p.components
        | std::views::transform([](auto&& x) { return std::string{x.identifier}; })
        | std::ranges::to<std::vector>()
    };
}

FunctionApplication create_function_application(std::span<Expression> expressions)
{
    assert(expressions.size() >= 2);
    if (expressions.size() == 2)
        return { std::make_unique<Expression>(std::move(expressions[0])), std::make_unique<Expression>(std::move(expressions[1])) };
    return {
        std::make_unique<Expression>(create_function_application(expressions.first(expressions.size() - 1))),
        std::make_unique<Expression>(std::move(expressions.back()))
    };
}

bool is_function_next(std::span<ExpressionToken> tokens)
{
    if (tokens.size() < 3)
        return false;
    if (!std::get_if<ParsedOpenParen>(&tokens[0]))
        return false;
    if (!std::get_if<ParsedHierarchicalIdentifier>(&tokens[1]))
        return false;
    if (!std::get_if<ParsedColon>(&tokens[2]))
        return false;
    return true;
}

struct IntermediateExpression
{
    Expression expression;
    std::span<ExpressionToken> remainder;
};

struct EndParse {};

std::expected<IntermediateExpression, std::string> create_expression_helper(std::span<ExpressionToken> tokens);

std::expected<IntermediateExpression, std::string> create_function(std::span<ExpressionToken> tokens)
{
    if (tokens.size() < 7)
        return std::unexpected("Not enough tokens for function. Expected at least 7 tokens but got " + std::to_string(tokens.size()));
    ParsedOpenParen const* paren = std::get_if<ParsedOpenParen>(&tokens[0]);
    ParsedHierarchicalIdentifier const* identifier = std::get_if<ParsedHierarchicalIdentifier>(&tokens[1]);
    ParsedColon const* colon = std::get_if<ParsedColon>(&tokens[2]);
    if (paren && identifier && colon && identifier->components.size() == 1)
    {
        std::optional<Expression> param_type;
        return create_expression_helper(tokens.subspan<3>())
            .and_then([&param_type](IntermediateExpression&& exp) -> std::expected<IntermediateExpression, std::string>
                {
                    if (
                        exp.remainder.size() < 2
                        || !std::holds_alternative<ParsedClosedParen>(exp.remainder[0])
                        || !std::holds_alternative<ParsedOperatorFunction>(exp.remainder[1]))
                        return std::unexpected("FAIL");
                    param_type = std::move(exp.expression);
                    return create_expression_helper(exp.remainder.subspan<2>());
                })
            .transform([identifier, &param_type](IntermediateExpression&& exp)
                {
                    return IntermediateExpression{
                        .expression = Expression{Function{
                            .param_name = std::string{identifier->components[0].identifier},
                            .param_type = std::make_unique<Expression>(std::move(*param_type)),
                            .return_type = std::make_unique<Expression>(std::move(exp.expression)),
                        }},
                        .remainder = exp.remainder
                    };
                });
    }
    return std::unexpected("Unable to parse function");
}

template <size_t Index, Parsed P, StringLiteral ErrorMsg>
auto expect_token(std::span<ExpressionToken> tokens)
{
    return [tokens](auto&& unused) -> std::expected<P*, std::string>
        {
            if (tokens.size() <= Index)
                return std::unexpected("unexpected end of input; " + std::string{ ErrorMsg.value });

            if (auto p = std::get_if<P>(&tokens[Index]))
                return p;

            return std::unexpected("unexpected token; " + std::string{ ErrorMsg.value });
        };
};

std::expected<IntermediateExpression, std::string> create_function_abstraction(std::span<ExpressionToken> tokens)
{
    std::string identifier;
    std::optional<Expression> param_type;

    return expect_token<0, ParsedKeywordFun, "expected 'fun'">(tokens)(0)
        .and_then(expect_token<1, ParsedOpenParen, "expected '('">(tokens))
        .and_then(expect_token<2, ParsedHierarchicalIdentifier, "expected identifier">(tokens))
        .and_then([&identifier](ParsedHierarchicalIdentifier* p) -> std::expected<ParsedHierarchicalIdentifier*, std::string>
            {
                if (p->components.size() != 1)
                    return std::unexpected("invalid binder name, it must be atomic");
                identifier = std::string{ p->components[0].identifier };
                return p;
            })
        .and_then(expect_token<3, ParsedColon, "expected ':'">(tokens))
        .and_then([tokens](auto&& unused) { return create_expression_helper(tokens.subspan<4>()); })
        .and_then([&param_type](IntermediateExpression&& exp) -> std::expected<IntermediateExpression, std::string>
            {
                std::span<ExpressionToken> remainder = exp.remainder;
                param_type = std::move(exp.expression);

                return expect_token<0, ParsedClosedParen, "expected ')'">(remainder)(0)
                    .and_then(expect_token<1, ParsedOperatorFunctionAbstraction, "expected '=>'">(remainder))
                    .and_then([remainder](auto&& unused) { return create_expression_helper(remainder.subspan<2>()); });
            })
        .transform([&identifier, &param_type](IntermediateExpression&& exp)
            {
                return IntermediateExpression{
                    .expression = FunctionAbstraction{
                        .param_name = std::move(identifier),
                        .param_type = std::make_unique<Expression>(std::move(*param_type)),
                        .return_value = std::make_unique<Expression>(std::move(exp.expression))
                    },
                    .remainder = exp.remainder
                };
            });
}

std::expected<IntermediateExpression, std::string> create_expression_helper(std::span<ExpressionToken> tokens)
{
    std::vector<Expression> expressions;
    while (!tokens.empty())
    {
        auto result = std::visit([tokens](Parsed auto&& token) -> std::expected<std::variant<IntermediateExpression, EndParse>, std::string>
            {
                using T = std::remove_cvref_t<decltype(token)>;
                if constexpr (std::is_same_v<T, ParsedHierarchicalIdentifier>)
                    return IntermediateExpression{ Expression{ create_identifier(token) }, tokens.subspan<1>() };
                else if constexpr (std::is_same_v<T, ParsedUniverseType>)
                    return IntermediateExpression{ Expression{ Universe { token.level + 1 } }, tokens.subspan<1>() };
                else if constexpr (std::is_same_v<T, ParsedUniverseProp>)
                    return IntermediateExpression{ Expression{ Universe { 0 } }, tokens.subspan<1>() };
                else if constexpr (std::is_same_v<T, ParsedUniverseSort>)
                    return IntermediateExpression{ Expression{ Universe { token.level } }, tokens.subspan<1>() };
                else if constexpr (std::is_same_v<T, ParsedOpenParen>)
                {
                    if (is_function_next(tokens))
                        return create_function(tokens);
                    else
                    {
                        return create_expression_helper(tokens.subspan<1>())
                            .and_then([](IntermediateExpression&& exp) -> std::expected<IntermediateExpression, std::string>
                                {
                                    if (exp.remainder.empty())
                                        return std::unexpected("unexpedcted end of input; expected ';'");
                                    if (!std::holds_alternative<ParsedClosedParen>(exp.remainder[0]))
                                        return std::unexpected("unexpected token; expected ';'");
                                    return IntermediateExpression{ std::move(exp.expression), exp.remainder.subspan<1>() };
                                });
                    }
                }
                else if constexpr (std::is_same_v<T, ParsedClosedParen>)
                    return EndParse{};
                else if constexpr (std::is_same_v<T, ParsedColon>)
                    return std::unexpected("unexpected ':'");
                else if constexpr (std::is_same_v<T, ParsedOperatorFunction>)
                    return std::unexpected("unexpected '->'");
                else if constexpr (std::is_same_v<T, ParsedOperatorFunctionAbstraction>)
                    return std::unexpected("unexpected '=>'");
                else if constexpr (std::is_same_v<T, ParsedKeywordFun>)
                    return create_function_abstraction(tokens);
                else
                    static_assert(false, "UNREACHABLE");
            }, tokens[0]);

        if (result)
        {
            if (std::holds_alternative<EndParse>(*result))
                break;
            else
            {
                assert(std::holds_alternative<IntermediateExpression>(*result));
                auto exp = std::move(std::get<IntermediateExpression>(*result));
                tokens = exp.remainder;
                expressions.push_back(std::move(exp.expression));
            }
        }
        else
        {
            return std::unexpected(result.error());
        }
    }

    if (expressions.empty())
        return std::unexpected("unexpected end of input; expected expression");
    else if (expressions.size() == 1)
        return IntermediateExpression{ .expression = std::move(expressions[0]), .remainder=tokens };
    return IntermediateExpression{ .expression = create_function_application(expressions), .remainder = tokens };
}
}

std::expected<Expression, std::string> create_expression(ParsedExpression p)
{
    return create_expression(p.tokens);
}

std::expected<Expression, std::string> create_expression(std::span<ExpressionToken> tokens)
{
    return create_expression_helper(tokens)
        .and_then([](IntermediateExpression&& exp) -> std::expected<Expression, std::string>
            {
                if (!exp.remainder.empty())
                    return std::unexpected("There is still more to parse");
                return std::move(exp.expression);
            });
}

std::string to_pretty_string(Expression const& exp)
{
    return std::visit([](auto&& x)
    {
            using T = std::remove_cvref_t<decltype(x)>;
            if constexpr (std::is_same_v<T, Universe>)
            {
                if (x.level == 0)
                    return std::string{ "Prop" };
                else
                    return std::format("Type {}", x.level - 1);
            }
            else if constexpr (std::is_same_v<T, Identifier>)
                return x.components | std::views::join_with('.') | std::ranges::to<std::string>();
            else if constexpr (std::is_same_v<T, Function>)
                return std::format("({} : {}) -> {}", x.param_name, to_pretty_string(*x.param_type), to_pretty_string(*x.return_type));
            else if constexpr (std::is_same_v<T, FunctionAbstraction>)
                return std::format("(fun {} : {} => {})", x.param_name, to_pretty_string(*x.param_type), to_pretty_string(*x.return_value));
            else if constexpr (std::is_same_v<T, FunctionApplication>)
                return std::format("({} {})", to_pretty_string(*x.function), to_pretty_string(*x.argument));
            else
                static_assert(false, "UNREACHABLE");
    }, exp);
}

Expression clone(Expression const& exp)
{
    return std::visit([](auto&& x)
        {
            using T = std::remove_cvref_t<decltype(x)>;
            if constexpr (std::is_same_v<T, Universe>)
                return Expression{ x };
            else if constexpr (std::is_same_v<T, Identifier>)
                return Expression{ x };
            else if constexpr (std::is_same_v<T, Function>)
                return Expression{ Function{
                    .param_name = x.param_name,
                    .param_type = std::make_unique<Expression>(clone(*x.param_type)),
                    .return_type = std::make_unique<Expression>(clone(*x.return_type)),
                } };
            else if constexpr (std::is_same_v<T, FunctionAbstraction>)
                return Expression{ FunctionAbstraction{
                    .param_name = x.param_name,
                    .param_type = std::make_unique<Expression>(clone(*x.param_type)),
                    .return_value = std::make_unique<Expression>(clone(*x.return_value)),
                } };
            else if constexpr (std::is_same_v<T, FunctionApplication>)
                return Expression{ FunctionApplication{
                    .function = std::make_unique<Expression>(clone(*x.function)),
                    .argument = std::make_unique<Expression>(clone(*x.argument)),
                } };
            else
                static_assert(false, "UNREACHABLE");
        }, exp);
}

std::generator<NamedExpressionView> get_function_args(Expression const& function)
{
    if (Function const* ptr = std::get_if<Function>(&function))
    {
        co_yield NamedExpressionView{ ptr->param_name, *ptr->param_type};
        co_yield std::ranges::elements_of(get_function_args(*ptr->return_type));
    }
}

bool operator==(Function const& lhs, Function const& rhs)
{
    return *lhs.param_type == *rhs.param_type && *lhs.return_type == *rhs.return_type;
}

bool operator==(FunctionAbstraction const& lhs, FunctionAbstraction const& rhs)
{
    return *lhs.param_type == *rhs.param_type && *lhs.return_value == *rhs.return_value;
}

bool operator==(FunctionApplication const& lhs, FunctionApplication const& rhs)
{
    return *lhs.function == *rhs.function && *lhs.argument == *rhs.argument;
}
