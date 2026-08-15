#include <algorithm>
#include <concepts>
#include <expected>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <print>
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

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::println("Usage: {} <input-file>", argv[0]);
        return 1;
    }

    run_file(argv[1]);
    return 0;
}

