#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include "Parsed.h"
#include "ParserUtils.h"

template <class Func>
inline auto effect(Func const& func)
{
    return [&func](auto x) {
        if constexpr (std::is_void_v<std::invoke_result_t<Func, decltype(x)>>)
        {
            func(x);
            return std::expected<decltype(x), ParseError>{x};
        }
        else
        {
            auto output = func(x);
            return output;
        }
    };
}

struct parse_identifier
{
    std::expected<ParsedIdentifier, ParseError> operator()(std::string_view input) const;
};

struct parse_control
{
    std::expected<ParsedControl, ParseError> operator()(std::string_view input) const;
};

struct parse_operator
{
    std::expected<ParsedOperator, ParseError> operator()(std::string_view input) const;
};

struct parse_open_paren
{
    std::expected<ParsedOpenParen, ParseError> operator()(std::string_view input) const;
};

struct parse_closed_paren
{
    std::expected<ParsedClosedParen, ParseError> operator()(std::string_view input) const;
};

struct parse_colon
{
    std::expected<ParsedColon, ParseError> operator()(std::string_view input) const;
};

struct parse_vertical_bar
{
    std::expected<ParsedVerticalBar, ParseError> operator()(std::string_view input) const;
};

struct parse_command_name
{
    std::expected<ParsedCommandName, ParseError> operator()(std::string_view input) const;
};

struct parse_expression
{
    std::expected<ParsedExpression, ParseError> operator()(std::string_view input) const;
};

struct parse_hash
{
    std::expected<ParsedHash, ParseError> operator()(std::string_view input) const;
};

struct parse_assignment
{
    std::expected<ParsedAssignment, ParseError> operator()(std::string_view input) const;
};

struct parse_type_name
{
    std::expected<ParsedType, ParseError> operator()(std::string_view input) const;
};

struct parse_nat_literal
{
    std::expected<ParsedNatLiteral, ParseError> operator()(std::string_view input) const;
};

struct parse_bool_literal
{
    std::expected<ParsedBoolLiteral, ParseError> operator()(std::string_view input) const;
};

struct parse_keyword
{
    std::expected<ParsedKeyword, ParseError> operator()(std::string_view input) const;
};

struct parse_keyword_def
{
    std::expected<ParsedKeywordDef, ParseError> operator()(std::string_view input) const;
};

struct parse_keyword_axiom
{
    std::expected<ParsedKeywordAxiom, ParseError> operator()(std::string_view input) const;
};

struct parse_keyword_theorem
{
    std::expected<ParsedKeywordTheorem, ParseError> operator()(std::string_view input) const;
};

struct parse_keyword_inductive
{
    std::expected<ParsedKeywordInductive, ParseError> operator()(std::string_view input) const;
};

struct parse_keyword_where
{
    std::expected<ParsedKeywordWhere, ParseError> operator()(std::string_view input) const;
};

struct parse_constant
{
    std::expected<ParsedConstant, ParseError> operator()(std::string_view input) const;
};

std::expected<Command, ParseError> parse_command(std::string_view input);

struct parse_axiom
{
    std::expected<ParsedAxiom, ParseError> operator()(std::string_view input) const;
};

struct parse_theorem
{
    std::expected<ParsedTheorem, ParseError> operator()(std::string_view input) const;
};

struct parse_constructor
{
    std::expected<ParsedConstructor, ParseError> operator()(std::string_view input) const;
};

struct parse_inductive_type
{
    std::expected<ParsedInductiveType, ParseError> operator()(std::string_view input) const;
};

