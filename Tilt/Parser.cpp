#include "Parser.h"
#include "StringLiteral.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <tuple>
#include <vector>
#include <variant>

std::expected<ParsedRawIdentifier, ParseError> parse_raw_identifier::operator()(std::string_view input) const
{
    auto it = std::find_if_not(input.begin(), input.end(), is_identifier);
    size_t identifier_end = std::distance(input.begin(), it);
    if (identifier_end)
        return ParsedRawIdentifier{
            .identifier = input.substr(0, identifier_end),
            .remainder = input.substr(identifier_end)
        };
    if (input.size() == 0)
        return std::unexpected("Expected identifier but reached end of input");
    return std::unexpected("Invalid identifier: '" + std::string{ input.substr(0, 1) } + "'.");
}

std::expected<ParsedIdentifier, ParseError> parse_identifier::operator()(std::string_view input) const
{
    return begin_parse(input)
        .and_then(immediate<parse_raw_identifier>)
        .and_then([](ParsedRawIdentifier p) -> std::expected<ParsedIdentifier, ParseError>
            {
                if (p.identifier == "where")
                    return std::unexpected("Unexpected use of reserved word 'where'. Expected identifier.");
                if (p.identifier == "Type")
                    return std::unexpected("Unexpected use of reserved word 'Type'. Expected identifier.");
                if (p.identifier == "Prop")
                    return std::unexpected("Unexpected use of reserved word 'Prop'. Expected identifier.");
                if (p.identifier == "Sort")
                    return std::unexpected("Unexpected use of reserved word 'Sort'. Expected identifier.");
                return ParsedIdentifier{ .identifier = p.identifier, .remainder = p.remainder };
            });
}

std::expected<ParsedDigits, ParseError> parse_digits::operator()(std::string_view input) const
{
    return begin_parse(input)
        .and_then(immediate<parse_raw_identifier>)
        .and_then([](ParsedRawIdentifier p) -> std::expected<ParsedDigits, ParseError>
            {
                std::string s{ p.identifier };
                if (!std::all_of(s.begin(), s.end(), [](char c) { return std::isdigit(c); }))
                    return std::unexpected("Unexpected term '" + s + "'. Expected natural number.");
                return ParsedDigits{ .digits = std::stoi(s), .remainder = p.remainder };
            });
}

std::expected<ParsedHierarchicalIdentifier, ParseError> parse_hierarchical_identifier::operator()(std::string_view input) const
{
    std::optional<ParsedIdentifier> first_identifier;
    return begin_parse(input)
        .and_then(immediate<parse_identifier>)
        .and_then(effect([&first_identifier](ParsedIdentifier parsed) { first_identifier = parsed; }))
        .and_then(immediate<zero_or_more<compose<parse_dot, parse_identifier>>>)
        .and_then([](Parsed auto&& parsed) -> std::expected<std::remove_cvref_t<decltype(parsed)>, ParseError>
        {
                // If there is a trailing dot with no identifier component, return error
                if (begin_parse(parsed.remainder).and_then(immediate<parse_dot>))
                    return std::unexpected("Invalid field notation : Identifier or numeral expected after '.'");
                return parsed;
        })
        .transform([first_identifier](Parsed auto&& parsed)
        {
                std::vector<ParsedIdentifier> components;
                components.reserve(parsed.value.size() + 1);
                components.push_back(*first_identifier);
                for (ParsedCompose<ParsedDot, ParsedIdentifier> parsed_compose : parsed.value)
                {
                    components.push_back(std::get<ParsedIdentifier>(parsed_compose.value));
                }
                return ParsedHierarchicalIdentifier{.components=std::move(components), .remainder = parsed.remainder};
        });
}

std::expected<ParsedControl, ParseError> parse_control::operator()(std::string_view input) const
{
    auto it = std::find_if_not(input.begin(), input.end(), is_control);
    size_t operator_end = std::distance(input.begin(), it);
    std::string_view control = input.substr(0, operator_end);
    if (operator_end != 0)
        return ParsedControl{ .control = control, .remainder = input.substr(operator_end) };
    return std::unexpected("Invalid operator: " + std::string{ next_token(input) });
}


namespace
{
template <Parsed P, StringLiteral Operator>
std::expected<P, ParseError> parse_operator_helper(std::string_view input)
{
    return begin_parse(input)
        .and_then(immediate<parse_control>)
        .transform_error([](ParseError const& e) {return e + " Expected operator '" + Operator.value + "'"; })
        .and_then([](ParsedControl p) -> std::expected<P, ParseError>
        {
                if (p.control == Operator.value)
                    return P{ .remainder = p.remainder };
                return std::unexpected("unexpected token: '" + std::string{ p.control } + "'. Expected operator '" + Operator.value + "'");
        });
}
}


std::expected<ParsedOperatorFunction, ParseError> parse_operator_function::operator()(std::string_view input) const
{
    return parse_operator_helper<ParsedOperatorFunction, "->">(input);
}


std::expected<ParsedOpenParen, ParseError> parse_open_paren::operator()(std::string_view input) const
{
    if (input.empty())
        return std::unexpected("Expected '(' but reached end of input");
    if (input.front() == '(')
        return ParsedOpenParen{ .remainder = input.substr(1) };
    return std::unexpected("Expected '(' but got " + input.front());
}


std::expected<ParsedClosedParen, ParseError> parse_closed_paren::operator()(std::string_view input) const
{
    if (input.empty())
        return std::unexpected("Expected ')' but reached end of input");
    if (input.front() == ')')
        return ParsedClosedParen{ .remainder = input.substr(1) };
    return std::unexpected("Expected ')' but got " + input.front());
}


std::expected<ParsedColon, ParseError> parse_colon::operator()(std::string_view input) const
{
    return begin_parse(input)
        .and_then(immediate<parse_control>)
        .and_then([](ParsedControl p) -> std::expected<ParsedColon, ParseError>
        {
                if (p.control == ":")
                    return ParsedColon{ .remainder = p.remainder };
                return std::unexpected("Expected ':' but got " + std::string{ p.control });
        });
}

std::expected<ParsedDot, ParseError> parse_dot::operator()(std::string_view input) const
{
    return begin_parse(input)
        .and_then(immediate<parse_control>)
        .and_then([](ParsedControl p) -> std::expected<ParsedDot, ParseError>
        {
                if (p.control == ".")
                    return ParsedDot{ .remainder = p.remainder };
                return std::unexpected("Expected '.' but got " + std::string{ p.control });
        });
}


std::expected<ParsedVerticalBar, ParseError> parse_vertical_bar::operator()(std::string_view input) const
{
    return begin_parse(input)
        .and_then(immediate<parse_control>)
        .and_then([](ParsedControl p) -> std::expected<ParsedVerticalBar, ParseError>
        {
                if (p.control == "|")
                    return ParsedVerticalBar{ .remainder = p.remainder };
                return std::unexpected("Expected ':' but got " + std::string{ p.control });
        });
}


std::expected<ParsedExpression, ParseError> parse_expression::operator()(std::string_view input) const
{
    ParsedExpression result;

    /*
    * Things that can be in an expression:
    * (dependent) function ((ident : term) -> term)
    * function abstraction (fun (ident : term) => term)
    * function application (term term)
    *
    * Expression parsing starts in partial_state.
    *
    * partial_state = Incomplete parse, expect identifier
    * full_state = Complete parse, expect identifier or operator
    * error_state = Terminate with failure. Input does not form a valid expression
    * done_state = Terminate with success. Input forms a valid expression
    *
    * partial_state + identifier => full_state
    * partial_state + '->' => error_state
    * partial_state + '=>' => error_state
    * partial_state + '(' => partial_state
    * partial_state + ')' => error_state
    * partial_state + ':' => error_state
    * partial_state + * => error_state
    *
    * full_state + identifier => full_state
    * full_state + '->' => partial_state
    * full_state + '=>' => partial_state
    * full_state + '(' => partial_state
    * full_state + ')' => full_state
    * full_state + ':' => partial_state
    * full_state + * => done_state
    */

    struct partial_state {};
    struct full_state {};
    struct error_state
    {
        std::string error;
    };
    struct done_state {};
    using StateType = std::variant<partial_state, full_state, error_state, done_state>;
    StateType state{ partial_state{} };

    while (!std::holds_alternative<error_state>(state) && !std::holds_alternative<done_state>(state))
    {
        state = std::visit([&input, &result](auto&& x) -> StateType
        {
                using T = std::remove_cvref_t<decltype(x)>;
                if constexpr (std::is_same_v<T, partial_state>)
                {
                    auto res = begin_parse(input)
                        .and_then(next<any<
                            "Expected term",
                            parse_hierarchical_identifier,
                            parse_universe_type,
                            parse_universe_prop,
                            parse_universe_sort,
                            parse_open_paren>>)
                        .transform([&input, &result](Parsed auto&& x)
                        {
                                return std::visit([&input, &result](Parsed auto&& p) -> StateType
                                {
                                        using T = std::remove_cvref_t<decltype(p)>;

                                        result.tokens.push_back(p);
                                        input = result.remainder = p.remainder;

                                        if constexpr (std::is_same_v<T, ParsedOpenParen>)
                                            return partial_state{};
                                        else
                                            return full_state{};

                                }, x.value);
                        })
                        .transform_error([](ParseError e)
                        {
                                return error_state{ e };
                        });

                    if (res)
                        return res.value();
                    return res.error();
                }
                else if constexpr (std::is_same_v<T, full_state>)
                {
                    auto res = begin_parse(input)
                        .and_then(next<any<
                            "Expected operator or identifier",
                            parse_operator_function,
                            parse_hierarchical_identifier,
                            parse_universe_type,
                            parse_universe_prop,
                            parse_universe_sort,
                            parse_colon,
                            parse_closed_paren>>)
                        .transform([&input, &result](Parsed auto&& parsed_any) -> StateType
                        {
                                return std::visit([&input, &result](Parsed auto&& parsed) -> StateType
                                {
                                        using T = std::remove_cvref_t<decltype(parsed)>;

                                        result.tokens.push_back(parsed);
                                        input = result.remainder = parsed.remainder;

                                        if constexpr (std::is_same_v<T, ParsedOperatorFunction> || std::is_same_v<T, ParsedColon>)
                                            return partial_state{};
                                        else
                                            return full_state{};
                                }, parsed_any.value);
                        });

                    return res.value_or(done_state{});
                }
                else if constexpr (std::is_same_v<T, error_state>)
                    return error_state{};
                else if constexpr (std::is_same_v<T, done_state>)
                    return done_state{};
                else
                    static_assert(false, "should be unreachable");
        }, state);
    }

    return std::visit([&result](auto&& arg) -> std::expected<ParsedExpression, ParseError>
    {
            using T = std::remove_cvref_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, partial_state>)
                return std::unexpected("BUG: unexpected expression parsing failure. partial_state");
            else if constexpr (std::is_same_v<T, full_state>)
                return std::unexpected("BUG: unexpected expression parsing failure. full_state");
            else if constexpr (std::is_same_v<T, error_state>)
                return std::unexpected("bad expression: " + arg.error);
            else if constexpr (std::is_same_v<T, done_state>)
            {
                return result;
            }
            else
                static_assert(false, "should be unreachable");
    }, state);
}


std::expected<ParsedHash, ParseError> parse_hash::operator()(std::string_view input) const
{
    if (input.empty())
        return std::unexpected("empty");
    if (input.front() == '#')
        return ParsedHash{ .remainder=input.substr(1) };
    return std::unexpected("Expected hash");
}


std::expected<ParsedAssignment, ParseError> parse_assignment::operator()(std::string_view input) const
{
    return begin_parse(input)
        .and_then(immediate <parse_control>)
        .and_then([](ParsedControl p) -> std::expected<ParsedAssignment, ParseError>
        {
                if (p.control == ":=")
                    return ParsedAssignment{ .remainder = p.remainder };
                return std::unexpected("Expected assignment. Got: " + std::string{ p.control });
        });
}


std::expected<ParsedUniverseType, ParseError> parse_universe_type::operator()(std::string_view input) const
{
    return begin_parse(input)
        .and_then(immediate<parse_keyword_type>)
        .and_then(next<parse_digits>)
        .transform([](ParsedDigits p)
            {
                return ParsedUniverseType{ .level = p.digits, .remainder = p.remainder };
            });
}


std::expected<ParsedUniverseProp, ParseError> parse_universe_prop::operator()(std::string_view input) const
{
    return begin_parse(input)
        .and_then(immediate<parse_keyword_prop>)
        .transform([](ParsedKeywordProp p) { return ParsedUniverseProp{ .remainder = p.remainder }; });
}


std::expected<ParsedUniverseSort, ParseError> parse_universe_sort::operator()(std::string_view input) const
{
    return begin_parse(input)
        .and_then(immediate<parse_keyword_sort>)
        .and_then(next<parse_digits>)
        .transform([](ParsedDigits p)
            {
                return ParsedUniverseSort{ .level = p.digits, .remainder = p.remainder };
            });
}


std::expected<ParsedType, ParseError> parse_type_name::operator()(std::string_view input) const
{
    return parse_identifier{}(input).and_then(
        [](ParsedIdentifier x) -> std::expected<ParsedType, ParseError> {
            return ParsedType{ .name = x.identifier, .remainder = x.remainder };
        });
}


std::expected<ParsedNatLiteral, ParseError> parse_nat_literal::operator()(std::string_view input) const
{
    auto it = std::find_if_not(input.begin(), input.end(), is_digit);
    size_t literal_end = std::distance(input.begin(), it);
    if (literal_end == 0)
        return std::unexpected("Expected Nat literal");
    return ParsedNatLiteral{
        .literal = input.substr(0, literal_end),
        .remainder = input.substr(literal_end)
    };
}


std::expected<ParsedBoolLiteral, ParseError> parse_bool_literal::operator()(std::string_view input) const
{
    auto it = std::find_if_not(input.begin(), input.end(), is_alpha);
    size_t literal_end = std::distance(input.begin(), it);
    if (literal_end == 0)
        return std::unexpected("Expected Bool literal");
    auto literal = input.substr(0, literal_end);
    if (literal == "true" || literal == "false")
        return ParsedBoolLiteral{
            .literal = input.substr(0, literal_end),
            .remainder = input.substr(literal_end)
        };
    return std::unexpected("invalid bool literal: " + std::string(literal));
}

namespace
{
template <Parsed P, StringLiteral Keyword>
std::expected<P, ParseError> parse_keyword_helper(std::string_view input)
{
    return begin_parse(input)
        .and_then(immediate<parse_raw_identifier>)
        .transform_error([](ParseError const& e) {return e + " Expected keyword '" + Keyword.value + "'"; })
        .and_then([](ParsedRawIdentifier p) -> std::expected<P, ParseError>
        {
                if (p.identifier == Keyword.value)
                    return P{ .remainder = p.remainder };
                return std::unexpected("unexpected identifier: '" + std::string{ p.identifier } + "'. Expected keyword '" + Keyword.value + "'");
        });
}
}


std::expected<ParsedKeywordCheck, ParseError> parse_keyword_check::operator()(std::string_view input) const
{
    return parse_keyword_helper<ParsedKeywordCheck, "check">(input);
}


std::expected<ParsedKeywordDef, ParseError> parse_keyword_def::operator()(std::string_view input) const
{
    return parse_keyword_helper<ParsedKeywordDef, "def">(input);
}


std::expected<ParsedKeywordAxiom, ParseError> parse_keyword_axiom::operator()(std::string_view input) const
{
    return parse_keyword_helper<ParsedKeywordAxiom, "axiom">(input);
}


std::expected<ParsedKeywordTheorem, ParseError> parse_keyword_theorem::operator()(std::string_view input) const
{
    return parse_keyword_helper<ParsedKeywordTheorem, "theorem">(input);
}


std::expected<ParsedKeywordInductive, ParseError> parse_keyword_inductive::operator()(std::string_view input) const
{
    return parse_keyword_helper<ParsedKeywordInductive, "inductive">(input);
}


std::expected<ParsedKeywordWhere, ParseError> parse_keyword_where::operator()(std::string_view input) const
{
    return parse_keyword_helper<ParsedKeywordWhere, "where">(input);
}


std::expected<ParsedKeywordType, ParseError> parse_keyword_type::operator()(std::string_view input) const
{
    return parse_keyword_helper<ParsedKeywordType, "Type">(input);
}


std::expected<ParsedKeywordProp, ParseError> parse_keyword_prop::operator()(std::string_view input) const
{
    return parse_keyword_helper<ParsedKeywordProp, "Prop">(input);
}


std::expected<ParsedKeywordSort, ParseError> parse_keyword_sort::operator()(std::string_view input) const
{
    return parse_keyword_helper<ParsedKeywordSort, "Sort">(input);
}


std::expected<ParsedConstant, ParseError> parse_constant::operator()(std::string_view input) const
{
    std::string name;
    std::string type;
    auto result = begin_parse(input)
        .and_then(next<parse_keyword_def>)
        .and_then(next<parse_identifier>)
        .and_then(effect([&name](ParsedIdentifier x) {name = x.identifier; }))
        .and_then(next<parse_colon>)
        .and_then(next<parse_type_name>)
        .and_then(effect([&type](ParsedType x) {type = x.name; }))
        .and_then(next<parse_assignment>)
        .and_then([&type](auto x) -> std::expected<std::variant<ParsedNatLiteral, ParsedBoolLiteral>, ParseError>
        {
                if (type == "Nat")
                {
                    return next<parse_nat_literal>(x);
                }
                else if (type == "Bool")
                {
                    return next<parse_bool_literal>(x);

                }
                else
                {
                    return std::unexpected("Unexpected type " + type);
                }
        })
        .and_then([&name](auto x) -> std::expected<ParsedConstant, ParseError>
        {
                return std::visit([&name](auto arg) -> ParsedConstant
                {
                        using T = std::decay_t<decltype(arg)>;
                        if constexpr (std::is_same_v<T, ParsedNatLiteral>)
                            return ParsedConstant{ .value = NatConstant{
                                .name = name,
                                .value = std::stoi(std::string(arg.literal))
                        }, .remainder=arg.remainder };
                        else if constexpr (std::is_same_v<T, ParsedBoolLiteral>)
                            return ParsedConstant{ .value = BoolConstant{
                                .name = name,
                                .value = arg.literal == "true"
                        }, .remainder = arg.remainder };
                        else
                            static_assert(false, "should be unreachable");

                }, x);
        });

    return result;
}


std::expected<ParsedCheckCommand, ParseError> parse_check_command::operator()(std::string_view input) const
{
    return begin_parse(input)
        .and_then(immediate<parse_hash>)
        .and_then(immediate<parse_keyword_check>)
        .and_then(next<parse_expression>)
        .transform([](ParsedExpression p) { return ParsedCheckCommand{ .expression = p, .remainder = p.remainder }; });
}


std::expected<ParsedAxiom, ParseError> parse_axiom::operator()(std::string_view input) const
{
    std::optional<std::string_view> identifier;
    std::optional<ParsedExpression> type;

    return begin_parse(input)
        .and_then(immediate<parse_keyword_axiom>)
        .and_then(next<parse_identifier>)
        .and_then(effect([&identifier](ParsedIdentifier p) { identifier = p.identifier; }))
        .and_then(next<parse_colon>)
        .and_then(next<parse_expression>)
        .and_then(effect([&type](ParsedExpression p) { type = p; }))
        .transform([&identifier, &type](ParsedExpression p) { return ParsedAxiom{ .identifier = *identifier, .type = *type , .remainder=p.remainder}; });
}


std::expected<ParsedTheorem, ParseError> parse_theorem::operator()(std::string_view input) const
{
    std::optional<std::string_view> identifier;
    std::optional<ParsedExpression> type;
    std::optional<ParsedExpression> value;

    return begin_parse(input)
        .and_then(immediate<parse_keyword_theorem>)
        .and_then(next<parse_identifier>)
        .and_then(effect([&identifier](ParsedIdentifier p) { identifier = p.identifier; }))
        .and_then(next<parse_colon>)
        .and_then(next<parse_expression>)
        .and_then(effect([&type](ParsedExpression p) { type = p; }))
        .and_then(next<parse_assignment>)
        .and_then(next<parse_expression>)
        .and_then(effect([&value](ParsedExpression p) { value = p; }))
        .transform([&identifier, &type, &value](ParsedExpression p)
        {
                return ParsedTheorem{ .identifier = *identifier, .type = *type, .value = *value, .remainder = p.remainder};
        });
}


std::expected<ParsedConstructor, ParseError> parse_constructor::operator()(std::string_view input) const
{
    std::optional<ParsedIdentifier> parsed_identifier;
    std::optional<ParsedExpression> parsed_type;

    return begin_parse(input)
        .and_then(immediate<parse_vertical_bar>)
        .and_then(next<parse_identifier>)
        .and_then(effect([&parsed_identifier](ParsedIdentifier p) { parsed_identifier = p; }))
        .and_then(next<parse_colon>)
        .and_then(next<parse_expression>)
        .and_then(effect([&parsed_type](ParsedExpression p) { parsed_type = p; }))
        .transform([parsed_identifier, parsed_type](Parsed auto&& p) { return ParsedConstructor{.identifier=*parsed_identifier, .type=*parsed_type, .remainder=p.remainder}; });
}


std::expected<ParsedInductiveType, ParseError> parse_inductive_type::operator()(std::string_view input) const
{
    std::optional<ParsedIdentifier> parsed_identifier;
    std::optional<std::vector<ParsedConstructor>> parsed_constructors;
    std::optional<ParsedExpression> parsed_type;

    return begin_parse(input)
        .and_then(immediate<parse_keyword_inductive>)
        .and_then(next<parse_identifier>)
        .and_then(effect([&parsed_identifier](ParsedIdentifier p) { parsed_identifier = p; }))
        .and_then(next<parse_colon>)
        .and_then(next<parse_expression>)
        .and_then(effect([&parsed_type](ParsedExpression p) { parsed_type = p; }))
        .and_then(next<parse_keyword_where>)
        .and_then(next<zero_or_more<parse_constructor>>)
        .and_then(effect([&parsed_constructors](ParsedZeroOrMore<ParsedConstructor> p) { parsed_constructors = p.value; }))
        .transform([parsed_identifier, &parsed_constructors, &parsed_type](Parsed auto&& p)
        {
                return ParsedInductiveType{.identifier=*parsed_identifier, .type=*parsed_type, .constructors=*parsed_constructors, .remainder=p.remainder};
        });
}
