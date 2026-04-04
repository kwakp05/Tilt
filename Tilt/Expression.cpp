#include <expected>
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <variant>

#include "Expression.h"
#include "Parsed.h"

namespace
{
struct IntermediateExpression
{
    Expression expression;
    std::span<ExpressionToken> remainder;
};

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

std::expected<IntermediateExpression, std::string> create_expression_helper(std::span<ExpressionToken> tokens)
{
    if (tokens.empty())
        return std::unexpected("unexpected end of input; expected expression");
    if (tokens.size() == 1 || std::holds_alternative<ParsedClosedParen>(tokens[1]))
        return std::visit([](Parsed auto&& token) -> std::expected<Expression, std::string>
            {
                using T = std::remove_cvref_t<decltype(token)>;
                if constexpr (std::is_same_v<T, ParsedHierarchicalIdentifier>)
                    return Expression{ Identifier{
                        token.components
                        | std::views::transform([](auto&& x) { return std::string{x.identifier}; })
                        | std::ranges::to<std::vector>()
                    } };
                else if constexpr (std::is_same_v<T, ParsedUniverseType>)
                    return Expression{ Universe { token.level + 1 } };
                else if constexpr (std::is_same_v<T, ParsedUniverseProp>)
                    return Expression{ Universe { 0 } };
                else if constexpr (std::is_same_v<T, ParsedUniverseSort>)
                    return Expression{ Universe { token.level } };
                else
                    return std::unexpected("how do I parse that");
            }, tokens[0])
        .transform([tokens](Expression&& expression)
            {
                return IntermediateExpression{ .expression = std::move(expression), .remainder = tokens.subspan<1>()};
            });

    return create_function(tokens);
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
                return std::format("Sort {}", x.level);
            else if constexpr (std::is_same_v<T, Identifier>)
                return x.components | std::views::join_with('.') | std::ranges::to<std::string>();
            else if constexpr (std::is_same_v<T, Function>)
                return std::format("({} : {}) -> {}", x.param_name, to_pretty_string(*x.param_type), to_pretty_string(*x.return_type));
            else if constexpr (std::is_same_v<T, FunctionAbstraction>)
                return std::format("fun {} : {} => {}", x.param_name, to_pretty_string(*x.param_type), to_pretty_string(*x.return_type));
            else if constexpr (std::is_same_v<T, FunctionApplication>)
                return std::format("{} {}", to_pretty_string(*x.function), to_pretty_string(*x.argument));
            else
                static_assert(false, "UNREACHABLE");
    }, exp.value);
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
                    .return_type = std::make_unique<Expression>(clone(*x.return_type)),
                } };
            else if constexpr (std::is_same_v<T, FunctionApplication>)
                return Expression{ FunctionApplication{
                    .function = std::make_unique<Expression>(clone(*x.function)),
                    .argument = std::make_unique<Expression>(clone(*x.argument)),
                } };
            else
                static_assert(false, "UNREACHABLE");
        }, exp.value);
}
