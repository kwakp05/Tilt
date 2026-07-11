#include <algorithm>
#include <concepts>
#include <expected>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "EngineRunner.h"
#include "Parsed.h"
#include "Parser.h"
#include "ParserUtils.h"
#include "VariantUtils.h"


void run_file(std::string const& path)
{
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();

    std::string text = buffer.str();
    std::expected<ParsedProgram, ParseError> program = begin_parse(text)
        .and_then(immediate<parse_program>);

    if (program)
    {
        SimpleEngineRunner engine;
        for (auto&& token: program->statements)
        {
            std::visit([&engine](auto&& statement)
                {
                    engine.process(statement);
                }, token);
        }
    }
    else
    {
        std::println("Failed to run file {}", path);
        std::println("{}", program.error());
    }
}


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
        std::cout << "Type:\n";
        print_parsed(parsed.type);
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
        std::println("{}", parsed.identifier.identifier);
        std::println("TYPE:");
        print_parsed(parsed.type);
        std::println("VALUE:");
        print_parsed(parsed.value);
    }
    else if constexpr (std::is_same_v<T, ParsedConstructor>)
    {
        std::cout << "ParsedConstructor\n";
        std::cout << "Identifier: " << parsed.identifier.identifier << "\n";
        std::cout << "Type:\n";
        print_parsed(parsed.type);
    }
    else if constexpr (std::is_same_v<T, ParsedHierarchicalIdentifier>)
    {
        std::cout << "ParsedHierarchicalIdentifier\n";
        for (auto&& p : parsed.components)
        {
            std::cout << p.identifier << " ";
        }
        std::cout << "\n";
    }
    else if constexpr (is_parsed_compose_v<T>)
    {
        std::cout << "ParsedCompose\n";
        std::apply([](auto&... parsed_args)
            {
                (..., print_parsed(parsed_args));
            }, parsed.value);
    }
    else if constexpr (std::is_same_v<T, ParsedHash>)
    {
        std::cout << "ParsedHash\n";
    }
    else if constexpr (std::is_same_v<T, ParsedIdentifier>)
    {
        std::cout << "ParsedIdentifier\n";
        std::cout << parsed.identifier << "\n";
    }
    else if constexpr (std::is_same_v<T, ParsedCheckCommand>)
    {
        std::cout << "ParsedCheckCommand\n";
        std::cout << "Expression:\n";
        print_parsed(parsed.expression);
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
    else if constexpr (std::is_same_v<T, ParsedOperatorFunction>)
        return "ParsedOperatorFunction";
    else if constexpr (std::is_same_v<T, ParsedOperatorFunctionAbstraction>)
        return "ParsedOperatorFunctionAbstraction";
    else if constexpr (std::is_same_v<T, ParsedIdentifier>)
        return "ParsedIdentifier";
    else if constexpr (std::is_same_v<T, ParsedOpenParen>)
        return "ParsedOpenParen";
    else if constexpr (std::is_same_v<T, ParsedClosedParen>)
        return "ParsedClosedParen";
    else if constexpr (std::is_same_v<T, ParsedColon>)
        return "ParsedColon";
    else if constexpr (std::is_same_v<T, ParsedHierarchicalIdentifier>)
        return "ParsedHierarchicalIdentifier";
    else if constexpr (std::is_same_v<T, ParsedUniverseType>)
        return "ParsedUniverseType";
    else if constexpr (std::is_same_v<T, ParsedUniverseProp>)
        return "ParsedUniverseProp";
    else if constexpr (std::is_same_v<T, ParsedUniverseSort>)
        return "ParsedUniverseSort";
    else if constexpr (std::is_same_v<T, ParsedKeywordFun>)
        return "ParsedKeywordFun";
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
        print_parsed(*x);
    }
    else
        std::cout << "FAIL " << x.error() << "\n";

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
        std::string input = "inductive Weekday : Type 0 where\n"
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
        {
            print_parsed(parsed_inductive_type.value());
            Engine engine;
            engine.process(parsed_inductive_type.value());
        }
        else
            std::cout << "FAIL " << parsed_inductive_type.error() << "\n";
    }

    {
        std::string input = "Weekday.sunday.monday";
        std::cout << "\nPARSING HIERARCHICAL IDENTIFIER " << input << "\n";
        auto parsed = begin_parse(input).and_then(immediate<parse_hierarchical_identifier>);
        if (parsed)
            print_parsed(parsed.value());
        else
            std::cout << "FAIL " << parsed.error() << "\n";
    }

    {
        std::string input = "#check";
        std::cout << "\nPARSING #check to test compose " << input << "\n";
        auto parsed = begin_parse(input).and_then(immediate<compose<parse_hash, parse_identifier>>);
        if (parsed)
            print_parsed(parsed.value());
        else
            std::cout << "FAIL " << parsed.error() << "\n";
    }

    {
        SimpleEngineRunner engine;
        std::string input = "inductive Nat : Type 0 where\n"
            "| zero : Nat\n"
            "| succ : (n : Nat) -> Nat\n";
        std::cout << "\nPARSING INDUCTIVE TYPE " << input << "\n";
        auto parsed_inductive_type = begin_parse(input).and_then(immediate<parse_inductive_type>);
        if (parsed_inductive_type)
        {
            print_parsed(parsed_inductive_type.value());
            engine.process(parsed_inductive_type.value());
            engine.print_identifier("Nat");
        }
        else
            std::cout << "FAIL " << parsed_inductive_type.error() << "\n";

        input = "#check Nat.succ";
        std::cout << "\nPARSING " << input << "\n";
        auto parsed_check_command = begin_parse(input).and_then(immediate<parse_check_command>);
        if (parsed_check_command)
        {
            print_parsed(parsed_check_command.value());
            engine.process(parsed_check_command.value());
        }
        else
            std::cout << "FAIL " << parsed_inductive_type.error() << "\n";

        input = "#check Nat.zero";
        std::cout << "\nPARSING " << input << "\n";
        parsed_check_command = begin_parse(input).and_then(immediate<parse_check_command>);
        if (parsed_check_command)
        {
            print_parsed(parsed_check_command.value());
            engine.process(parsed_check_command.value());
            std::println("{}", to_pretty_string(create_expression(parsed_check_command.value().expression).value()));
        }
        else
            std::cout << "FAIL " << parsed_inductive_type.error() << "\n";

        input = "#check Nat.succ Nat.zero";
        std::cout << "\nPARSING " << input << "\n";
        parsed_check_command = begin_parse(input).and_then(immediate<parse_check_command>);
        if (parsed_check_command)
        {
            print_parsed(parsed_check_command.value());
            engine.process(parsed_check_command.value());
            std::println("{}", to_pretty_string(create_expression(parsed_check_command.value().expression).value()));
        }
        else
            std::cout << "FAIL " << parsed_inductive_type.error() << "\n";

        input = "#check Nat.succ (Nat.succ Nat.zero)";
        std::cout << "\nPARSING " << input << "\n";
        parsed_check_command = begin_parse(input).and_then(immediate<parse_check_command>);
        if (parsed_check_command)
        {
            print_parsed(parsed_check_command.value());
            engine.process(parsed_check_command.value());
            std::println("{}", to_pretty_string(create_expression(parsed_check_command.value().expression).value()));
        }
        else
            std::cout << "FAIL " << parsed_inductive_type.error() << "\n";
    }

    run_file("code.txt");
}

