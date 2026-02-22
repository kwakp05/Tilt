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

