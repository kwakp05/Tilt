#pragma once

#include <type_traits>
#include <variant>


template <class T>
struct is_variant
{
	static constexpr bool value = false;
};

template <class... Types>
struct is_variant<std::variant<Types...>>
{
	static constexpr bool value = true;
};

template <class T>
concept Variant = is_variant<T>::value;

template <class T>
inline constexpr bool is_variant_v = is_variant<T>::value;

static_assert(is_variant_v<std::variant<int, double>>);
static_assert(!is_variant_v<std::tuple<int, double>>);


namespace
{
template <class V1, class V2>
struct combine_variants_impl;

template <class... Types1, class... Types2>
struct combine_variants_impl<std::variant<Types1...>, std::variant<Types2...>>
{
	using type = std::variant<Types1..., Types2...>;
};

template <class V1, class V2>
using combine_variants_impl_t = combine_variants_impl<V1, V2>::type;

static_assert(std::is_same_v<combine_variants_impl_t<std::variant<int, double>, std::variant<float, char>>, std::variant<int, double, float, char>>);
}


template <class... Variants>
struct combine_variants;

template <class V>
struct combine_variants<V>
{
	using type = V;
};

template <class ResultVariant, class InputVariant, class... Variants>
struct combine_variants<ResultVariant, InputVariant, Variants...>
{
	using type = combine_variants<combine_variants_impl_t<ResultVariant, InputVariant>, Variants...>::type;
};

template <class... Variants>
using combine_variants_t = combine_variants<Variants...>::type;


static_assert(std::is_same_v<
	combine_variants_t<std::variant<int, double>>,
	std::variant<int, double>>
);

static_assert(std::is_same_v<
	combine_variants_t<std::variant<int, double>, std::variant<float, char>>,
	std::variant<int, double, float, char>>
);

static_assert(std::is_same_v<
	combine_variants_t<std::variant<int, double>, std::variant<short>, std::variant<float, char>>,
	std::variant<int, double, short, float, char>>
);
