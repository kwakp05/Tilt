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

#include "utf8.h"

#include "ParserUtils.h"

//template<SingleParser P>
//struct get_variant_return
//{
//    using type = 
//};

template<Parser p, Parser... args>
struct get_return
{

};

auto do_any(Parser auto parser, Parser auto... parsers)
{
    if constexpr (sizeof...(parsers) == 0)
    {
        return lift_parser(parser);
    }
    else
    {
        return parser;
    }
}

enum class Keyword
{
    DEF
};

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

using Constant = std::variant<NatConstant, BoolConstant>;

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

struct ParsedDummy
{
    std::string_view remainder;
};

struct ParsedCheckCommand
{
    std::string_view remainder;
};

struct ParsedEvalCommand
{
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

struct ParsedFunctionOperator
{
    std::string_view remainder;
};

using ParsedOperator = std::variant<ParsedFunctionOperator>;

struct ParsedHash
{
    std::string_view remainder;
};

struct ParsedColon
{
    std::string_view remainder;
};

struct ParsedAssignment
{
    std::string_view remainder;
};

struct ParsedKeyword
{
    Keyword keyword;
    std::string_view remainder;
};

struct ParsedExpression
{
    std::string_view remainder;
};

constexpr bool is_whitespace(char c)
{
    return c == ' ' || c == '\n';
}

constexpr bool is_crlf(char c1, char c2)
{
    return c1 == '\r' && c2 == '\n';
}

constexpr bool is_alpha(char c)
{
    return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z';
}

constexpr bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

constexpr bool is_identifier(char c)
{
    return is_alpha(c) || is_digit(c) || c == '_';
}

constexpr bool is_control(char c)
{
    return c == '-' || c == '>';
}

std::string_view consume_whitespace(std::string_view input)
{
    std::size_t i = 0;
    while (true)
    {
        if (i < input.size() && is_whitespace(input[i]))
            i++;
        else if (i + 1 < input.size() && is_crlf(input[i], input[i + 1]))
            i += 2;
        else
            break;
    }
    return input.substr(i);
}

template <class Func>
auto next(Func const& func)
{
    return [&func](auto val) {
        using T = std::decay_t<decltype(val)>;
        std::string_view remainder;
        if constexpr (Parsed<T>)
            remainder = consume_whitespace(val.remainder);
        else
            remainder = std::visit([](auto& x)
            {
                    return consume_whitespace(x.remainder);
            }, val);
        return func(remainder);
    };
}

std::expected<ParsedIdentifier, ParseError> parse_identifier(std::string_view input)
{
    if (input.empty())
        return std::unexpected("empty");
    if (!is_identifier(input.front()))
        return std::unexpected("bad char");

    auto it = std::find_if_not(input.begin(), input.end(), is_identifier);
    size_t identifier_end = std::distance(input.begin(), it);
    return ParsedIdentifier{
        .identifier = input.substr(0, identifier_end),
        .remainder = input.substr(identifier_end)
    };
}

std::expected<ParsedOperator, ParseError> parse_operator(std::string_view input)
{
    auto it = std::find_if_not(input.begin(), input.end(), is_control);
    size_t operator_end = std::distance(input.begin(), it);
    std::string_view op = input.substr(0, operator_end);
    if (op == "->")
        return ParsedFunctionOperator{
            .remainder = input.substr(operator_end)
        };
    return std::unexpected("Invalid operator: " + std::string{ op });
}

std::expected<std::variant<ParsedCheckCommand, ParsedEvalCommand>, ParseError> parse_command_name(std::string_view input)
{
    return parse_identifier(input).and_then([](ParsedIdentifier x) -> std::expected<std::variant<ParsedCheckCommand, ParsedEvalCommand>, ParseError>
    {
            if (x.identifier == "check")
                return ParsedCheckCommand{ .remainder = x.remainder };
            else if (x.identifier == "eval")
                return ParsedEvalCommand{ .remainder = x.remainder };
            else
                return std::unexpected("Invalid command: " + std::string(x.identifier));
    });
}

std::expected<ParsedExpression, ParseError> parse_expression(std::string_view input)
{
    struct test1 {};
    struct test2 {};
    std::variant<test1, test2> state{ test1{} };
    while (true)
    {
        auto result = std::visit([input](auto x)
        {
                using T = std::decay_t<decltype(x)>;
                if constexpr (std::is_same_v<T, test1>)
                {
                    //return parse_identifier(input);
                    return 3;
                }
                else if constexpr (std::is_same_v<T, test2>)
                {
                    //return parse_identifier(input)
                    //    .or_else([input]() {return parse_identifier(input); });
                    return 2;
                }
                else
                    static_assert(false, "should be unreachable");
        }, state);
        break;

    }
    parse_identifier(input).and_then(next(parse_operator));

    return std::unexpected("how do i do this");
}

std::expected<ParsedHash, ParseError> parse_hash(std::string_view input)
{
    if (input.empty())
        return std::unexpected("empty");
    if (input.front() == '#')
        return ParsedHash{ .remainder=input.substr(1) };
    return std::unexpected("Expected hash");
}

std::expected<ParsedColon, ParseError> parse_colon(std::string_view input)
{
    if (input.empty())
        return std::unexpected("empty");
    if (input.front() == ':')
        return ParsedColon{ .remainder=input.substr(1) };
    return std::unexpected("Expected colon");
}

std::expected<ParsedAssignment, ParseError> parse_assignment(std::string_view input)
{
    if (input.size() < 2)
        return std::unexpected("not enough chars for assingmnent operator");
    if (input.starts_with(":="))
        return ParsedAssignment{ input.substr(2) };
    return std::unexpected("Expected assignment operator");
}

std::expected<ParsedType, ParseError> parse_type_name(std::string_view input)
{
    return parse_identifier(input).and_then(
        [](ParsedIdentifier x) -> std::expected<ParsedType, ParseError> {
            return ParsedType{ .name = x.identifier, .remainder = x.remainder };
        });
}

std::expected<ParsedNatLiteral, ParseError> parse_nat_literal(std::string_view input)
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

std::expected<ParsedBoolLiteral, ParseError> parse_bool_literal(std::string_view input)
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

template<Keyword k>
std::expected<ParsedKeyword, ParseError> expect_keyword(ParsedKeyword val)
{
    if (val.keyword == k)
        return val;
    return std::unexpected("wrong keyword");
}

template <class Func>
auto immediate(Func const& func)
{
    return [&func](auto val) {
        return func(val.remainder);
    };
}

std::expected<ParsedDummy, ParseError> begin_parse(std::string_view input)
{
    return ParsedDummy{ .remainder = input };
}

template <class Func>
auto effect(Func const& func)
{
    return [&func](auto x) -> std::expected<decltype(x), ParseError> {
        func(x);
        return x;
	};
}

std::expected<ParsedKeyword, ParseError> parse_keyword(std::string_view input)
{
    if (input.starts_with("def"))
    {
        return ParsedKeyword{ .keyword = Keyword::DEF, .remainder = input.substr(3) };
    }
    return std::unexpected("Invalid keyword");
}

std::expected<Constant, ParseError> parse_constant(std::string_view input)
{

    std::string name;
    std::string type;
    auto result = begin_parse(input)
        .and_then(next(parse_keyword))
        .and_then(expect_keyword<Keyword::DEF>)
        .and_then(next(parse_identifier))
        .and_then(effect([&name](ParsedIdentifier x) {name = x.identifier; }))
        .and_then(next(parse_colon))
        .and_then(next(parse_type_name))
        .and_then(effect([&type](ParsedType x) {type = x.name; }))
        .and_then(next(parse_assignment))
        .and_then([&type](auto x) -> std::expected<std::variant<ParsedNatLiteral, ParsedBoolLiteral>, ParseError>
        {
                if (type == "Nat")
                {
                    return next(parse_nat_literal)(x);
                }
                else if (type == "Bool")
                {
                    return next(parse_bool_literal)(x);

                }
                else
                {
                    return std::unexpected("Unexpected type " + type);
                }
        })
        .and_then([&name](auto x) -> std::expected<Constant, ParseError>
        {
                return std::visit([&name](auto arg) -> Constant
                {
                        using T = std::decay_t<decltype(arg)>;
                        if constexpr (std::is_same_v<T, ParsedNatLiteral>)
                            return NatConstant{
                                .name = name,
                                .value = std::stoi(std::string(arg.literal))
                        };
                        else if constexpr (std::is_same_v<T, ParsedBoolLiteral>)
                            return BoolConstant{
                                .name = name,
                                .value = arg.literal == "true"
                        };
                        else
                            static_assert(false, "should be unreachable");

                }, x);
        });

    return result;
}

std::expected<Command, ParseError> parse_command(std::string_view input)
{

    std::string name;
    std::string type;
    auto result = begin_parse(input)
        .and_then(next(parse_hash))
        .and_then(immediate(parse_command_name))
        .and_then(next(parse_expression));
    return std::unexpected("HI");
}

int main()
{
    std::string input = "def hello : Nat := 12938\n";
    std::string input2 = "#check hello";
    //std::string input = "def msodfij3 : Bool := false\n";
    auto x = parse_constant(input);
    auto y = parse_command(input2);
    if (x.has_value())
    {
        std::visit([](auto&& arg)
            {
                std::cout << arg.name << "\n";
                std::cout << arg.type << "\n";
                std::cout << arg.value << "\n";
            }, *x);
    }
    else
        std::cout << x.error() << "\n";
}
