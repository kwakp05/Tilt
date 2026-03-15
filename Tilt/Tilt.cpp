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

#include "Parsed.h"
#include "Parser.h"
#include "ParserUtils.h"
#include "VariantUtils.h"


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
