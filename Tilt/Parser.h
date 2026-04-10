#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <type_traits>

#include "Parsed.h"
#include "ParserUtils.h"

struct parse_raw_identifier
{
    std::expected<ParsedRawIdentifier, ParseError> operator()(std::string_view input) const;
};

struct parse_identifier
{
    std::expected<ParsedIdentifier, ParseError> operator()(std::string_view input) const;
};

struct parse_digits
{
    std::expected<ParsedDigits, ParseError> operator()(std::string_view input) const;
};

struct parse_hierarchical_identifier
{
    std::expected<ParsedHierarchicalIdentifier, ParseError> operator()(std::string_view input) const;
};

struct parse_control
{
    std::expected<ParsedControl, ParseError> operator()(std::string_view input) const;
};

struct parse_operator_function
{
    std::expected<ParsedOperatorFunction, ParseError> operator()(std::string_view input) const;
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

struct parse_dot
{
    std::expected<ParsedDot, ParseError> operator()(std::string_view input) const;
};

struct parse_vertical_bar
{
    std::expected<ParsedVerticalBar, ParseError> operator()(std::string_view input) const;
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

struct parse_universe_type
{
    std::expected<ParsedUniverseType, ParseError> operator()(std::string_view input) const;
};

struct parse_universe_prop
{
    std::expected<ParsedUniverseProp, ParseError> operator()(std::string_view input) const;
};

struct parse_universe_sort
{
    std::expected<ParsedUniverseSort, ParseError> operator()(std::string_view input) const;
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

struct parse_keyword_check
{
    std::expected<ParsedKeywordCheck, ParseError> operator()(std::string_view input) const;
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

struct parse_keyword_type
{
    std::expected<ParsedKeywordType, ParseError> operator()(std::string_view input) const;
};

struct parse_keyword_prop
{
    std::expected<ParsedKeywordProp, ParseError> operator()(std::string_view input) const;
};

struct parse_keyword_sort
{
    std::expected<ParsedKeywordSort, ParseError> operator()(std::string_view input) const;
};

struct parse_constant
{
    std::expected<ParsedConstant, ParseError> operator()(std::string_view input) const;
};

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

struct parse_check_command
{
    std::expected<ParsedCheckCommand, ParseError> operator()(std::string_view input) const;
};

struct parse_program
{
    std::expected<ParsedProgram, ParseError> operator()(std::string_view input) const;
};
