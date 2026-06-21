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

struct ParsedUniverseType
{
    int level;
    std::string_view remainder;
};

struct ParsedUniverseProp
{
    std::string_view remainder;
};

struct ParsedUniverseSort
{
    int level;
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

struct ParsedDigits
{
    int digits;
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

struct ParsedOperatorFunction
{
    std::string_view remainder;
};

struct ParsedOperatorFunctionAbstraction
{
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

struct ParsedKeywordCheck
{
    std::string_view remainder;
};

struct ParsedKeywordReduce
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

struct ParsedKeywordType
{
    std::string_view remainder;
};

struct ParsedKeywordProp
{
    std::string_view remainder;
};

struct ParsedKeywordSort
{
    std::string_view remainder;
};

struct ParsedKeywordFun
{
    std::string_view remainder;
};

using ExpressionToken = std::variant<
    ParsedHierarchicalIdentifier,
    ParsedUniverseType,
    ParsedUniverseProp,
    ParsedUniverseSort,
    ParsedOpenParen,
    ParsedClosedParen,
    ParsedColon,
    ParsedOperatorFunction,
    ParsedOperatorFunctionAbstraction,
    ParsedKeywordFun
>;

struct ParsedExpression
{
    std::string_view remainder;
    std::vector<ExpressionToken> tokens;
};

struct ParsedConstant
{
    std::string_view identifier;
    ParsedExpression type;
    ParsedExpression value;
    std::string_view remainder;
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

struct ParsedParameter
{
    ParsedIdentifier identifier;
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
    std::vector<ParsedParameter> parameters;
    std::string_view remainder;
};

struct ParsedCheckCommand
{
    ParsedExpression expression;
    std::string_view remainder;
};

struct ParsedReduceCommand
{
    ParsedExpression expression;
    std::string_view remainder;
};

struct ParsedEvalCommand
{
    ParsedExpression expression;
    std::string_view remainder;
};

struct ParsedProgram
{
    std::vector<std::variant<ParsedInductiveType, ParsedCheckCommand, ParsedReduceCommand, ParsedConstant>> statements;
    std::string_view remainder;
};
