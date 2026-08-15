#include <expected>
#include <fstream>
#include <print>
#include <sstream>
#include <string>

#include "EngineRunner.h"
#include "Parsed.h"
#include "Parser.h"
#include "ParserUtils.h"


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

