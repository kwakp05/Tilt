#pragma once


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
        return Parsed<std::variant_alternative_t<I, T>>&& is_parsed_variant<T, I + 1>();
}

template <typename T>
concept SingleParseResult = std::same_as<T, std::expected<typename T::value_type, typename T::error_type>>
    && Parsed<typename T::value_type>
    && std::same_as<typename T::error_type, ParseError>;

template <typename T>
concept MultiParseResult = std::same_as<T, std::expected<typename T::value_type, typename T::error_type>>
    && is_parsed_variant<typename T::value_type>()
    && std::same_as<typename T::error_type, ParseError>;

template <typename T>
concept SingleParser = std::invocable<T, std::string_view> && SingleParseResult<std::invoke_result_t<T, std::string_view>>;

template <typename T>
concept MultiParser = std::invocable<T, std::string_view>&& MultiParseResult<std::invoke_result_t<T, std::string_view>>;

template <typename T>
concept Parser = SingleParser<T> || MultiParser<T>;

auto lift_parser(Parser auto const& parser)
{
    if constexpr (MultiParser<parser>)
        return parser;
    else
    {
        return [parser](std::string_view input)
        {
                return parser(input).transform([](Parsed auto result)
                {
                        return std::variant<decltype(result)>{result};
                });
        };
    }
}

template<MultiParser P>
struct get_variant_return
{
    using type = std::invoke_result_t<P, std::string_view>;
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
constexpr auto next = next_impl<Func>{};
