#pragma once

#include <concepts>
#include <expected>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

using ParseError = std::string;

template <typename T>
concept Parsed = requires (T a)
{
    { a.remainder } -> std::same_as<std::string_view&>;
};

template <typename T>
concept ParseResult = std::same_as<T, std::expected<typename T::value_type, typename T::error_type>>
    && Parsed<typename T::value_type>
    && std::same_as<typename T::error_type, ParseError>;

template <typename T>
concept Parser = std::invocable<T, std::string_view> && ParseResult<std::invoke_result_t<T, std::string_view>>;

struct NatConstant
{
    std::string name;
    int value;
    std::string type = "Nat";
};

struct BoolConstant
{
    std::string name;
    bool value;
    std::string type = "Bool";
};

struct ParsedConstant
{
    std::variant<NatConstant, BoolConstant> value;
    std::string_view remainder;
};

struct Expression
{
};

struct CheckCommand
{
    Expression expression;
};

struct EvalCommand
{
    Expression expression;
};

using Command = std::variant<CheckCommand, EvalCommand>;

struct ParsedCheckCommand
{
    std::string_view remainder;
};

struct ParsedEvalCommand
{
    std::string_view remainder;
};

struct ParsedCommandName
{
    std::variant<ParsedCheckCommand, ParsedEvalCommand> value;
    std::string_view remainder;

};

struct ParsedType
{
    std::string_view name;
    std::string_view remainder;
};

struct ParsedNatLiteral
{
    std::string_view literal;
    std::string_view remainder;
};

struct ParsedBoolLiteral
{
    std::string_view literal;
    std::string_view remainder;
};

struct ParsedRawIdentifier
{
    std::string_view identifier;
    std::string_view remainder;
};

struct ParsedIdentifier
{
    std::string_view identifier;
    std::string_view remainder;
};

struct ParsedHierarchicalIdentifier
{
    std::vector<ParsedIdentifier> components;
    std::string_view remainder;
};

struct ParsedControl
{
    std::string_view control;
    std::string_view remainder;
};

struct ParsedFunctionOperator
{
    std::string_view remainder;
};

struct ParsedOperator
{
    std::variant<ParsedFunctionOperator> value;
    std::string_view remainder;
};

struct ParsedOpenParen
{
    std::string_view remainder;
};

struct ParsedClosedParen
{
    std::string_view remainder;
};

struct ParsedHash
{
    std::string_view remainder;
};

struct ParsedColon
{
    std::string_view remainder;
};

struct ParsedDot
{
    std::string_view remainder;
};

struct ParsedVerticalBar
{
    std::string_view remainder;
};

struct ParsedAssignment
{
    std::string_view remainder;
};

struct ParsedKeywordDef
{
    std::string_view remainder;
};

struct ParsedKeywordAxiom
{
    std::string_view remainder;
};

struct ParsedKeywordTheorem
{
    std::string_view remainder;
};

struct ParsedKeywordInductive
{
    std::string_view remainder;
};

struct ParsedKeywordWhere
{
    std::string_view remainder;
};

struct ParsedKeyword
{
    std::variant<ParsedKeywordDef, ParsedKeywordAxiom, ParsedKeywordTheorem, ParsedKeywordInductive, ParsedKeywordWhere> value;
    std::string_view identifier;
    std::string_view remainder;
};

struct ParsedExpression
{
    std::string_view remainder;
    std::vector<std::variant<ParsedIdentifier, ParsedOpenParen, ParsedClosedParen, ParsedColon, ParsedOperator>> tokens;
};

struct ParsedTheorem
{
    std::string_view identifier;
    ParsedExpression type;
    ParsedExpression value;
    std::string_view remainder;
};

struct ParsedAxiom
{
    std::string_view identifier;
    ParsedExpression type;
    std::string_view remainder;
};

struct ParsedConstructor
{
    ParsedIdentifier identifier;
    ParsedExpression type;
    std::string_view remainder;
};

struct ParsedInductiveType
{
    ParsedIdentifier identifier;
    ParsedExpression type;
    std::vector<ParsedConstructor> constructors;
    std::string_view remainder;
};

