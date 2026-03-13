#pragma once

#include <algorithm>
#include <cstddef>

template <std::size_t N>
struct StringLiteral
{
    char value[N];

    constexpr StringLiteral(const char (&str)[N])
    {
        std::copy_n(str, N, value);
    }
};
