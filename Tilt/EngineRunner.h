#pragma once

#include <iostream>
#include <print>
#include <string>
#include <type_traits>
#include <utility>

#include "Engine.h"
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

private:
    Engine engine;
};
