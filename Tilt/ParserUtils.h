#pragma once

#include <type_traits>

#include "VariantUtils.h"

using ParseError = std::string;

template <typename T>
concept Parsed = requires (T a)
{
    { a.remainder } -> std::same_as<std::string_view&>;
};

template<typename T, std::size_t I = 0>
consteval bool is_parsed_variant()
{
    if constexpr (I >= std::variant_size_v<T>)
        return true;
    else
        return Parsed<std::variant_alternative_t<I, T>> && is_parsed_variant<T, I + 1>();
}

template <typename T>
concept SingleParseResult = std::same_as<T, std::expected<typename T::value_type, typename T::error_type>>
    && Parsed<typename T::value_type>
    && std::same_as<typename T::error_type, ParseError>;

template <typename T>
concept MultiParseResult = std::same_as<T, std::expected<typename T::value_type, typename T::error_type>>
    && is_variant_v<typename T::value_type>
    && is_parsed_variant<typename T::value_type>()
    && std::same_as<typename T::error_type, ParseError>;

template <typename T>
concept SingleParser = std::invocable<T, std::string_view> && SingleParseResult<std::invoke_result_t<T, std::string_view>>;

template <typename T>
concept MultiParser = std::invocable<T, std::string_view>&& MultiParseResult<std::invoke_result_t<T, std::string_view>>;

template <typename T>
concept Parser = SingleParser<T> || MultiParser<T>;

template <Parser P>
struct get_variant_return;

template <SingleParser P>
struct get_variant_return<P>
{
	using type = std::variant<typename std::invoke_result_t<P, std::string_view>::value_type>;
};

template <MultiParser P>
struct get_variant_return<P>
{
	using type = std::invoke_result_t<P, std::string_view>::value_type;
};

template<Parser P>
using get_variant_return_t = get_variant_return<P>::type;

namespace
{
template <SingleParser P>
struct lift_parser_impl
{
	std::expected<get_variant_return_t<P>, ParseError> operator()(std::string_view input) const
	{
        return P{}(input).transform([](Parsed auto result)
        {
                return std::variant<decltype(result)>{result};
        });
	}
    static_assert(MultiParser<lift_parser_impl<P>>, "HI");
};
}

template <Parser P>
struct lift_parser {};

template <MultiParser P>
struct lift_parser<P>
{
    using type = P;
};

template <SingleParser P>
struct lift_parser<P>
{
    using type = lift_parser_impl<P>;
};

template <Parser P>
using lift_parser_t = lift_parser<P>::type;

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

namespace
{
template <class Func>
struct next_impl
{
    auto operator()(Parsed auto val) const
    {
		using T = std::decay_t<decltype(val)>;
		std::string_view remainder;
		if constexpr (Parsed<T>)
			remainder = consume_whitespace(val.remainder);
		else
			remainder = std::visit([](auto& x)
			{
					return consume_whitespace(x.remainder);
			}, val);
		return Func{}(remainder);
    }
};
}

template <class Func>
inline constexpr auto next = next_impl<Func>{};

namespace
{
template <class Func>
struct immediate_impl
{
    auto operator()(Parsed auto val) const
    {
		return Func{}(val.remainder);
    }
};
}

template <class Func>
inline constexpr auto immediate = immediate_impl<Func>{};

template <Parser... Parsers>
struct any
{
	using CombinedParsedType = combine_variants_t<get_variant_return_t<Parsers>...>;
    using ReturnType = std::expected<CombinedParsedType, ParseError>;

	std::expected<CombinedParsedType, ParseError> operator()(std::string_view input) const
	{
		std::optional<CombinedParsedType> result{};
		auto apply_parser = [&result, input]<Parser P>()
		{
			auto output = lift_parser_t<P>{}(input);
            if (output.has_value())
            {
                std::visit([&result](auto&& res)
                {
                        result = CombinedParsedType{res};
                }, *output);
            }
			return output.has_value();
		};

		(... || apply_parser.template operator()<Parsers>());

        return result
            .transform([](CombinedParsedType x) {return ReturnType{x}; })
            .value_or(std::unexpected("parse any failed"));
	}
};

