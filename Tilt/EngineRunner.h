#pragma once

#include <iostream>
#include <print>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <utility>

#include "Engine.h"
#include "InductiveType.h"
#include "Parsed.h"

class SimpleEngineRunner
{
public:
    void process(Parsed auto p)
    {
        using R = decltype(engine.process(p));
        auto value_handler = []()
        {
                if constexpr (std::is_same_v<R, std::expected<std::string, Engine::ErrorType>>)
                    return [](std::string const& str) { std::println("info: {}", str); };
                else
                    return []() {};
        }();

        std::ignore = engine.process(p)
            .transform(value_handler)
            .transform_error([](Engine::ErrorType e) { std::println("{}", e); return e; });
    };

    void print_identifier(std::string const& identifier)
    {
        if (IdentifierValueType const* v = engine.scope_find(identifier))
        {
            std::visit([](auto const& value)
                {
                    using T = std::remove_cvref_t<decltype(value)>;
                    if constexpr (std::is_same_v<T, InductiveType>)
                        std::println("{}", to_pretty_string(value));
                    else
                        static_assert(false, "UNREACHABLE");
                }, *v);
        }
        else
        {
            throw std::out_of_range("invalid identifier " + identifier);
        }
    }

private:
    Engine engine;
};
