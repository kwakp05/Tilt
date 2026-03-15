#include <algorithm>
#include <concepts>
#include <expected>
#include <functional>
#include <iostream>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "utf8.h"

#include "ParserUtils.h"
#include "VariantUtils.h"


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

struct ParsedIdentifier
{
    std::string_view identifier;
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
    ParsedType type;
    std::string_view remainder;
};

struct ParsedInductiveType
{
    ParsedIdentifier identifier;
    std::vector<ParsedConstructor> constructors;
    std::string_view remainder;
};

template <class Func>
auto effect(Func const& func)
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
    std::expected<ParsedIdentifier, ParseError> operator()(std::string_view input) const
    {
        auto it = std::find_if_not(input.begin(), input.end(), is_identifier);
        size_t identifier_end = std::distance(input.begin(), it);
        if (identifier_end)
            return ParsedIdentifier{
                .identifier = input.substr(0, identifier_end),
                .remainder = input.substr(identifier_end)
            };
        if (input.size() == 0)
            return std::unexpected("Expected identifier but reached end of input");
        return std::unexpected("Invalid identifier: " + std::string{ input.substr(0, 1) });
    }
};

struct parse_control
{
    std::expected<ParsedControl, ParseError> operator()(std::string_view input) const
    {
        auto it = std::find_if_not(input.begin(), input.end(), is_control);
        size_t operator_end = std::distance(input.begin(), it);
        std::string_view control = input.substr(0, operator_end);
        if (operator_end != 0)
            return ParsedControl{ .control = control, .remainder = input.substr(operator_end) };
        return std::unexpected("Invalid operator: " + std::string{ next_token(input) });
    }
};

struct parse_operator
{
    std::expected<ParsedOperator, ParseError> operator()(std::string_view input) const
    {
        return begin_parse(input)
            .and_then(immediate<parse_control>)
            .and_then([](ParsedControl p) -> std::expected<ParsedOperator, ParseError>
            {
                    if (p.control == "->")
                        return ParsedOperator{ .value = ParsedFunctionOperator{.remainder = p.remainder}, .remainder = p.remainder };
                    return std::unexpected("Invalid operator: " + std::string{ p.control });
            });
    }
};

struct parse_open_paren
{
    std::expected<ParsedOpenParen, ParseError> operator()(std::string_view input) const
    {
        if (input.empty())
            return std::unexpected("Expected '(' but reached end of input");
        if (input.front() == '(')
            return ParsedOpenParen{ .remainder = input.substr(1) };
        return std::unexpected("Expected '(' but got " + input.front());
    }
};

struct parse_closed_paren
{
    std::expected<ParsedClosedParen, ParseError> operator()(std::string_view input) const
    {
        if (input.empty())
            return std::unexpected("Expected ')' but reached end of input");
        if (input.front() == ')')
            return ParsedClosedParen{ .remainder = input.substr(1) };
        return std::unexpected("Expected ')' but got " + input.front());
    }
};

struct parse_colon
{
    std::expected<ParsedColon, ParseError> operator()(std::string_view input) const
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
};

struct parse_vertical_bar
{
    std::expected<ParsedVerticalBar, ParseError> operator()(std::string_view input) const
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
};

struct parse_command_name
{
    std::expected<ParsedCommandName, ParseError> operator()(std::string_view input) const
    {
        return begin_parse(input)
            .and_then(immediate<parse_identifier>)
            .and_then([](ParsedIdentifier x) -> std::expected<ParsedCommandName, ParseError>
            {
                    if (x.identifier == "check")
                        return ParsedCommandName{ .value = ParsedCheckCommand{.remainder = x.remainder }, .remainder=x.remainder };
                    else if (x.identifier == "eval")
                        return ParsedCommandName{ .value = ParsedEvalCommand{.remainder = x.remainder }, .remainder=x.remainder};
                    else
                        return std::unexpected("Invalid command: " + std::string(x.identifier));
            });
    }
};

struct parse_expression
{
    std::expected<ParsedExpression, ParseError> operator()(std::string_view input) const
    {
        ParsedExpression result;

        /*
        * Expression parsing starts in partial_state.
        *
        * partial_state = Incomplete parse, expect identifier
        * full_state = Complete parse, expect identifier or operator
        * error_state = Terminate with failure. Input does not form a valid expression
        * done_state = Terminate with success. Input forms a valid expression
        *
        * partial_state + identifier => full_state
        * partial_state + operator => error_state
        * partial_state + '(' => partial_state
        * partial_state + ')' => error_state
        * partial_state + ':' => error_state
        * partial_state + * => error_state
        *
        * full_state + identifier => full_state
        * full_state + operator => partial_state
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
                            .and_then(next<any<"Expected term", parse_identifier, parse_open_paren>>)
                            .transform([&input, &result](Parsed auto&& x)
                            {
                                    return std::visit([&input, &result](Parsed auto&& p) -> StateType
                                    {
                                            using T = std::remove_cvref_t<decltype(p)>;

                                            result.tokens.push_back(p);
                                            input = result.remainder = p.remainder;

                                            if constexpr (std::is_same_v<T, ParsedIdentifier>)
                                                return full_state{};
                                            else if constexpr (std::is_same_v<T, ParsedOpenParen>)
                                                return partial_state{};
                                            else
                                                static_assert(false, "UNREACHABLE");

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
                            .and_then(next<any<"Expected operator or identifier", parse_operator, parse_identifier, parse_colon, parse_closed_paren>>)
                            .transform(effect([&input, &result](Parsed auto&& parsed_any) -> StateType
                                {
                                    return std::visit([&input, &result](Parsed auto&& parsed) -> StateType
                                        {
                                            using T = std::remove_cvref_t<decltype(parsed)>;

                                            result.tokens.push_back(parsed);
                                            input = result.remainder = parsed.remainder;

                                            if constexpr (std::is_same_v<T, ParsedIdentifier> || std::is_same_v<T, ParsedClosedParen>)
                                                return full_state{};
                                            else
                                                return partial_state{};
                                    }, parsed_any.value);
                            }));

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
};

struct parse_hash
{
    std::expected<ParsedHash, ParseError> operator()(std::string_view input) const
    {
        if (input.empty())
            return std::unexpected("empty");
        if (input.front() == '#')
            return ParsedHash{ .remainder=input.substr(1) };
        return std::unexpected("Expected hash");
    }
};

struct parse_assignment
{
    std::expected<ParsedAssignment, ParseError> operator()(std::string_view input) const
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
};

struct parse_type_name
{
    std::expected<ParsedType, ParseError> operator()(std::string_view input) const
    {
        return parse_identifier{}(input).and_then(
            [](ParsedIdentifier x) -> std::expected<ParsedType, ParseError> {
                return ParsedType{ .name = x.identifier, .remainder = x.remainder };
            });
    }
};

struct parse_nat_literal
{
    std::expected<ParsedNatLiteral, ParseError> operator()(std::string_view input) const
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
};

struct parse_bool_literal
{
    std::expected<ParsedBoolLiteral, ParseError> operator()(std::string_view input) const
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
};

struct parse_keyword
{
    std::expected<ParsedKeyword, ParseError> operator()(std::string_view input) const
    {
        return begin_parse(input)
            .and_then(immediate<parse_identifier>)
            .and_then([](ParsedIdentifier p) -> std::expected<ParsedKeyword, ParseError>
            {
                    auto helper = [rem=p.remainder]<Parsed P>()
                    {
                        return P{ .remainder = rem };
                    };

                    using KeywordVariant = decltype(ParsedKeyword::value);
                    std::optional<KeywordVariant> val;
                    if (p.identifier == "def")
                        val = helper.operator()<ParsedKeywordDef>();
                    else if (p.identifier == "axiom")
                        val = helper.operator()<ParsedKeywordAxiom>();
                    else if (p.identifier == "theorem")
                        val = helper.operator()<ParsedKeywordTheorem>();
                    else if (p.identifier == "inductive")
                        val = helper.operator()<ParsedKeywordInductive>();
                    else if (p.identifier == "where")
                        val = helper.operator()<ParsedKeywordWhere>();

                    return val
                        .transform([p](KeywordVariant v) -> std::expected<ParsedKeyword, ParseError>
                        {
                                return ParsedKeyword{.value=v, .identifier=p.identifier, .remainder=p.remainder};
                        })
                        .value_or(std::unexpected("unexpected identifier: " + std::string{ p.identifier } + ". Expected keyword"));
            });
    }
};

struct parse_keyword_def
{
    std::expected<ParsedKeywordDef, ParseError> operator()(std::string_view input) const
    {
        return begin_parse(input)
            .and_then(immediate<parse_keyword>)
            .and_then([](ParsedKeyword p)
            {
                    return std::visit([identifier=p.identifier](Parsed auto&& keyword) -> std::expected<ParsedKeywordDef, ParseError>
                    {
                            using T = std::remove_cvref_t<decltype(keyword)>;
                            if constexpr (std::is_same_v<T, ParsedKeywordDef>)
                                return keyword;
                            return std::unexpected("unexpected keyword: " + std::string{ identifier } + ". Expected def");
                    }, p.value);
            });
    }
};

struct parse_keyword_axiom
{
    std::expected<ParsedKeywordAxiom, ParseError> operator()(std::string_view input) const
    {
        return begin_parse(input)
            .and_then(immediate<parse_keyword>)
            .and_then([](ParsedKeyword p)
            {
                    return std::visit([identifier=p.identifier](Parsed auto&& keyword) -> std::expected<ParsedKeywordAxiom, ParseError>
                    {
                            using T = std::remove_cvref_t<decltype(keyword)>;
                            if constexpr (std::is_same_v<T, ParsedKeywordAxiom>)
                                return keyword;
                            return std::unexpected("unexpected keyword: " + std::string{ identifier } + ". Expected def");
                    }, p.value);
            });
    }
};

struct parse_keyword_theorem
{
    std::expected<ParsedKeywordTheorem, ParseError> operator()(std::string_view input) const
    {
        return begin_parse(input)
            .and_then(immediate<parse_keyword>)
            .and_then([](ParsedKeyword p)
            {
                    return std::visit([identifier=p.identifier](Parsed auto&& keyword) -> std::expected<ParsedKeywordTheorem, ParseError>
                    {
                            using T = std::remove_cvref_t<decltype(keyword)>;
                            if constexpr (std::is_same_v<T, ParsedKeywordTheorem>)
                                return keyword;
                            return std::unexpected("unexpected keyword: " + std::string{ identifier } + ". Expected theorem");
                    }, p.value);
            });
    }
};

struct parse_keyword_inductive
{
    std::expected<ParsedKeywordInductive, ParseError> operator()(std::string_view input) const
    {
        return begin_parse(input)
            .and_then(immediate<parse_keyword>)
            .and_then([](ParsedKeyword p)
            {
                    return std::visit([identifier=p.identifier](Parsed auto&& keyword) -> std::expected<ParsedKeywordInductive, ParseError>
                    {
                            using T = std::remove_cvref_t<decltype(keyword)>;
                            if constexpr (std::is_same_v<T, ParsedKeywordInductive>)
                                return keyword;
                            return std::unexpected("unexpected keyword: " + std::string{ identifier } + ". Expected inductive");
                    }, p.value);
            });
    }
};

struct parse_keyword_where
{
    std::expected<ParsedKeywordWhere, ParseError> operator()(std::string_view input) const
    {
        return begin_parse(input)
            .and_then(immediate<parse_keyword>)
            .and_then([](ParsedKeyword p)
            {
                    return std::visit([identifier=p.identifier](Parsed auto&& keyword) -> std::expected<ParsedKeywordWhere, ParseError>
                    {
                            using T = std::remove_cvref_t<decltype(keyword)>;
                            if constexpr (std::is_same_v<T, ParsedKeywordWhere>)
                                return keyword;
                            return std::unexpected("unexpected keyword: " + std::string{ identifier } + ". Expected where");
                    }, p.value);
            });
    }
};

struct parse_constant
{
    std::expected<ParsedConstant, ParseError> operator()(std::string_view input) const
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
};

std::expected<Command, ParseError> parse_command(std::string_view input)
{

    std::string name;
    std::string type;
    auto result = begin_parse(input)
        .and_then(next<parse_hash>)
        .and_then(immediate<parse_command_name>)
        .and_then(next<parse_expression>);
    return std::unexpected("HI");
}

struct parse_axiom
{
    std::expected<ParsedAxiom, ParseError> operator()(std::string_view input) const
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
};

struct parse_theorem
{
    std::expected<ParsedTheorem, ParseError> operator()(std::string_view input) const
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
};

struct parse_constructor
{
    std::expected<ParsedConstructor, ParseError> operator()(std::string_view input) const
    {
        std::optional<ParsedIdentifier> parsed_identifier;
        std::optional<ParsedType> parsed_type;

        return begin_parse(input)
            .and_then(immediate<parse_vertical_bar>)
            .and_then(next<parse_identifier>)
            .and_then(effect([&parsed_identifier](ParsedIdentifier p) { parsed_identifier = p; }))
            .and_then(next<parse_colon>)
            .and_then(next<parse_type_name>)
            .and_then(effect([&parsed_type](ParsedType p) { parsed_type = p; }))
            .transform([parsed_identifier, parsed_type](Parsed auto&& p) { return ParsedConstructor{.identifier=*parsed_identifier, .type=*parsed_type, .remainder=p.remainder}; });
    }
};

struct parse_inductive_type
{
    std::expected<ParsedInductiveType, ParseError> operator()(std::string_view input) const
    {
        std::optional<ParsedIdentifier> parsed_identifier;
        std::optional<std::vector<ParsedConstructor>> parsed_constructors;

        return begin_parse(input)
            .and_then(immediate<parse_keyword_inductive>)
            .and_then(next<parse_identifier>)
            .and_then(effect([&parsed_identifier](ParsedIdentifier p) { parsed_identifier = p; }))
            .and_then(next<parse_keyword_where>)
            .and_then(next<zero_or_more<parse_constructor>>)
            .and_then(effect([&parsed_constructors](ParsedZeroOrMore<ParsedConstructor> p) { parsed_constructors = p.value; }))
            .transform([parsed_identifier, &parsed_constructors](Parsed auto&& p)
            {
                    return ParsedInductiveType{.identifier=*parsed_identifier, .constructors=*parsed_constructors, .remainder=p.remainder};
            });
    }
};

void print_parsed(Parsed auto&& parsed)
{
    using T = std::remove_cvref_t<decltype(parsed)>;
    if constexpr (std::is_same_v<T, ParsedExpression>)
    {
        std::cout << "ParsedExpression" << "\n";
        for (auto&& var : parsed.tokens)
        {
            std::visit([](Parsed auto&& p)
            {
                    using P = std::remove_cvref_t<decltype(p)>;
                    std::cout << "    " << parsed_to_name<decltype(p)>() << "\n";
                    if constexpr (std::is_same_v<P, ParsedIdentifier>)
                        std::cout << "        " << p.identifier << "\n";
            }, var);
        }
    }
    else if constexpr (std::is_same_v<T, ParsedAxiom>)
    {
        std::cout << "ParsedAxiom" << "\n";
        std::cout << "    " << parsed.identifier << "\n";
        print_parsed(parsed.type);
    }
    else if constexpr (std::is_same_v<T, ParsedTheorem>)
    {
        std::cout << "ParsedTheorem" << "\n";
        std::cout << "    " << parsed.identifier << "\n";
        print_parsed(parsed.type);
        print_parsed(parsed.value);
    }
    else if constexpr (std::is_same_v<T, ParsedInductiveType>)
    {
        std::cout << "ParsedInductiveType" << "\n";
        std::cout << "Name: " << parsed.identifier.identifier << "\n";
        for (size_t i = 0; i < parsed.constructors.size(); i++)
        {
            std::cout << "Constructor #" << i << "\n";
            print_parsed(parsed.constructors[i]);
        }
    }
    else if constexpr (is_parsed_zero_or_more_v<T>)
    {
        std::cout << "ParsedZeroOrMore (Size: " << parsed.value.size() << ")\n";
        for (size_t i = 0; i < parsed.value.size(); i++)
        {
            std::cout << "INDEX " << i << "\n";
            print_parsed(parsed.value[i]);
        }
    }
    else if constexpr (std::is_same_v<T, ParsedConstant>)
    {
        std::cout << "ParsedConstant\n";
        std::visit([](auto&& arg)
        {
                std::cout << arg.name << "\n";
                std::cout << arg.type << "\n";
                std::cout << arg.value << "\n";
        }, parsed.value);
    }
    else if constexpr (std::is_same_v<T, ParsedConstructor>)
    {
        std::cout << "ParsedConstructor\n";
        std::cout << "Identifier: " << parsed.identifier.identifier << "\n";
        std::cout << "Type: " << parsed.type.name << "\n";
    }
    else
        static_assert(false, "UNREACHABLE");

}

template <class P>
constexpr std::string parsed_to_name()
{
    using T = std::remove_cvref_t<P>;
    static_assert(Parsed<T>);

    if constexpr (std::is_same_v<T, ParsedExpression>)
        return "ParsedExpression";
    else if constexpr (std::is_same_v<T, ParsedFunctionOperator>)
        return "ParsedFunctionOperator";
    else if constexpr (std::is_same_v<T, ParsedIdentifier>)
        return "ParsedIdentifier";
    else if constexpr (std::is_same_v<T, ParsedOpenParen>)
        return "ParsedOpenParen";
    else if constexpr (std::is_same_v<T, ParsedClosedParen>)
        return "ParsedClosedParen";
    else if constexpr (std::is_same_v<T, ParsedColon>)
        return "ParsedColon";
    else if constexpr (std::is_same_v<T, ParsedOperator>)
        return "ParsedOperator";
    else
        static_assert(false, "UNREACHABLE");
}

int main()
{
    std::string input = "def hello : Nat := 12938\n";
    std::string input2 = "#check hello";
    std::string input3 = "identifier check";
    auto x = begin_parse(input).and_then(immediate<parse_constant>);

    if (x.has_value())
    {
        std::visit([](auto&& arg)
            {
                std::cout << arg.name << "\n";
                std::cout << arg.type << "\n";
                std::cout << arg.value << "\n";
            }, x->value);
    }
    else
        std::cout << "FAIL " << x.error() << "\n";

    auto y = parse_command(input2);

    std::string input4 = "Nat -> List Nat -> Nat";
    std::cout << "\nPARSING EXPRESSION " << input4 << "\n";
    auto z2 = begin_parse(input4).and_then(next<parse_expression>);
    if (z2)
    {

        print_parsed(z2.value());
    }
    else
    {
        std::cout << "failed expression: " << z2.error() << "\n";
    }

    {
        std::string input = "axiom mynum : Nat -> Nat -> False";
        std::cout << "\nPARSING AXIOM " << input << "\n";
        auto res = begin_parse(input)
            .and_then(immediate<parse_axiom>);
        if (res)
            print_parsed(res.value());
        else
            std::cout << "FAIL " << res.error() << "\n";
    }

    {
        std::string input = "theorem myt : (p : Prop) -> p -> p := fun x : Prop => fun y : x => (y : x)";
        std::cout << "\nPARSING THEOREM " << input << "\n";
        auto res = begin_parse(input)
            .and_then(immediate<parse_theorem>);
        if (res)
            print_parsed(res.value());
        else
            std::cout << "FAIL " << res.error() << "\n";
    }

    {
        std::string input = "def Hello : Nat := 3\ndef World : Bool := false";
        std::cout << "\nPARSING MULTIPLE CONSTANT " << input << "\n";
        auto res = begin_parse(input)
            .and_then(immediate<zero_or_more<parse_constant>>);
        if (res)
            print_parsed(res.value());
        else
            std::cout << "FAIL " << res.error() << "\n";
    }

    {
        std::string input = "inductive Weekday where\n"
            "| sunday : Weekday\n"
            "| monday : Weekday\n"
            "| tuesday : Weekday\n"
            "| wednesday : Weekday\n"
            "| thursday : Weekday\n"
            "| friday : Weekday\n"
            "| saturday : Weekday\n";
        std::cout << "\nPARSING INDUCTIVE TYPE " << input << "\n";
        auto parsed_inductive_type = begin_parse(input).and_then(immediate<parse_inductive_type>);
        if (parsed_inductive_type)
            print_parsed(parsed_inductive_type.value());
        else
            std::cout << "FAIL " << parsed_inductive_type.error() << "\n";

    }

}
