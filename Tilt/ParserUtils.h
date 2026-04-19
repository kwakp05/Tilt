#pragma once

#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "Parsed.h"
#include "StringLiteral.h"
#include "VariantUtils.h"

template <Parser P>
struct get_parser_value
{
	using type = std::invoke_result_t<P, std::string_view>::value_type;
};

template <Parser P>
using get_parser_value_t = get_parser_value<P>::type;

inline constexpr bool is_whitespace(char c)
{
    return c == ' ' || c == '\n';
}

inline constexpr bool is_crlf(char c1, char c2)
{
    return c1 == '\r' && c2 == '\n';
}

inline constexpr bool is_alpha(char c)
{
    return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z';
}

inline constexpr bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

inline constexpr bool is_identifier(char c)
{
    return is_alpha(c) || is_digit(c) || c == '_';
}

inline constexpr bool is_control(char c)
{
    return c == '-' || c == '>' || c == ':' || c == '=' || c == '|' || c == '.';
}

inline std::string_view consume_whitespace(std::string_view input)
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
inline auto effect(Func const& func)
{
    return [&func](auto&& x) {
        using T = decltype(x);
        if constexpr (std::is_void_v<std::invoke_result_t<Func, T>>)
        {
            func(x);
            return std::expected<std::remove_cvref_t<T>, ParseError>{std::forward<T>(x)};
        }
        else
        {
            auto output = func(std::forward<T>(x));
            return output;
        }
    };
}

struct ParsedDummy
{
    std::string_view remainder;
};

inline std::expected<ParsedDummy, ParseError> begin_parse(std::string_view input)
{
    return ParsedDummy{ .remainder = input };
}

namespace
{
std::string_view next_token_impl(std::string_view input, auto&& filter)
{
    std::size_t i = 0;
    while (true)
    {
        if (i < input.size() && filter(input[i]))
            i++;
        else
            break;
    }
    return input.substr(0, i);
}
}

inline std::string_view next_token(std::string_view input)
{
    input = consume_whitespace(input);
    if (input.empty())
        return {};
    char c = input.front();
    if (is_identifier(c))
        return next_token_impl(input, [](char c) { return is_identifier(c); });
    if (is_control(c))
        return next_token_impl(input, [](char c) { return is_control(c); });
    return input.substr(0, 1);
}

namespace
{
template <class Func>
struct next_impl
{
    auto operator()(Parsed auto&& val) const
    {
		return Func{}(consume_whitespace(val.remainder));
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

template <Parsed... Args>
struct ParsedAny
{
    std::variant<Args...> value;
    std::string_view remainder;
};

template <StringLiteral Error, Parser... Parsers>
struct any
{
    using ParsedType = ParsedAny<get_parser_value_t<Parsers>...>;
    using ReturnType = std::expected<ParsedType, ParseError>;

	ReturnType operator()(std::string_view input) const
	{
		std::optional<ParsedType> result{};

		auto apply_parser = [&result, input]<Parser P>()
		{
			auto output = P{}(input);
            if (output.has_value())
            {
                result = ParsedType{ .value = *output, .remainder = output->remainder };
            }
			return output.has_value();
		};

		(... || apply_parser.template operator()<Parsers>());

        return result
            .transform([](ParsedType p) { return ReturnType{ p }; })
            .value_or(std::unexpected("unexpected token '" + std::string{ next_token(input) } + "'; " + std::string{Error.value}));
	}
};

template <Parsed P>
struct ParsedZeroOrMore
{
    std::vector<P> value;
    std::string_view remainder;
};

template<class T>
struct is_parsed_zero_or_more : std::false_type {};

template<class... Ts>
struct is_parsed_zero_or_more<ParsedZeroOrMore<Ts...>> : std::true_type {};

template<class T>
constexpr bool is_parsed_zero_or_more_v = is_parsed_zero_or_more<T>::value;

template <Parser P>
struct zero_or_more
{
    using ParsedSubType = get_parser_value_t<P>;
    using ParsedType = ParsedZeroOrMore<ParsedSubType>;
    using ReturnType = std::expected<ParsedType, ParseError>;

    ReturnType operator()(std::string_view input) const
    {
        std::vector<ParsedSubType> value;
        auto parser = P{};
        while (true)
        {
            auto res = begin_parse(input)
                .and_then(next<P>)
                .transform([&value, &input](ParsedSubType p)
                {
                        value.push_back(p);
                        input = p.remainder;
                        return p;
                });

            if (!res)
                break;
        }
        return ParsedType{ .value = std::move(value), .remainder = input };
    }
};

template <Parsed... P>
struct ParsedCompose
{
    std::tuple<P...> value;
    std::string_view remainder;
};

template <Parser... Parsers>
struct compose
{
    using ParsedType = ParsedCompose<get_parser_value_t<Parsers>...>;
    using ReturnType = std::expected<ParsedType, ParseError>;

    ReturnType operator()(std::string_view input) const
    {
        std::tuple<std::optional<get_parser_value_t<Parsers>>...> tup;
        std::optional<ParseError> error;

        auto apply_parser = [&input, &tup, &error]<Parser P>()
        {
            auto res = begin_parse(input)
                .and_then(immediate<P>)
                .and_then(effect([&input, &tup](Parsed auto&& parsed)
                {
                        input = parsed.remainder;
                        std::get<std::optional<get_parser_value_t<P>>>(tup) = parsed;
                }))
                .transform_error([&error](ParseError e)
                {
                        error = e;
                        return e;
                });

            return res.has_value();
        };

        if ((... && apply_parser.template operator()<Parsers>()))
        {
            return ParsedType{
                .value = std::apply([](auto const&... parsed_args)
                    {
                        return std::make_tuple((*parsed_args)...);

                    }, tup),
                .remainder = input
            };
        }

        return std::unexpected(*error);
    }
};

template<class T>
struct is_parsed_compose : std::false_type {};

template<class... Ts>
struct is_parsed_compose<ParsedCompose<Ts...>> : std::true_type {};

template<class T>
constexpr bool is_parsed_compose_v = is_parsed_compose<T>::value;

template <Parsed P>
struct ParsedMaybe
{
    std::optional<P> value;
    std::string_view remainder;
};

template <Parser P>
struct maybe
{
    using ParsedSubType = get_parser_value_t<P>;

    std::expected<ParsedMaybe<ParsedSubType>, ParseError> operator()(std::string_view input) const
    {
        std::string_view remainder = input;
        std::optional<ParsedSubType> value;

        auto res = begin_parse(input).and_then(immediate<P>);

        if (res)
        {
            remainder = res->remainder;
            value = std::move(*res);
        }

        return ParsedMaybe{.value=std::move(value), .remainder=remainder};
    }
};
