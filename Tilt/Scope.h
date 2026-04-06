#pragma once

#include <concepts>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <variant>

namespace
{
template <class T>
struct is_pointer_to_const : std::false_type {};

template<class T>
struct is_pointer_to_const<T const*> : std::true_type {};
}

template <class T>
concept ScopeReturn = is_pointer_to_const<T>::value;

template <class T>
concept Scope = requires(T const v, std::string const& identifier)
{
    { v.scope_find(identifier) } -> ScopeReturn;
};

namespace
{
template <class T>
struct is_variant : std::false_type {};

template <class... Types>
struct is_variant<std::variant<Types...>> : std::true_type {};

template <class T>
concept Variant = is_variant<T>::value;
}

template <class... Rs>
std::optional<std::variant<std::reference_wrapper<Rs const>...>> resolve_identifier(std::span<std::string const> identifier, auto const& scope)
{
    using VariantType = std::variant<std::reference_wrapper<Rs const>...>;
    using T = std::remove_cvref_t<decltype(scope)>;
    static_assert((std::is_same_v<Rs, T> || ...), "Type T must be in Rs");

    if (identifier.empty())
    {
        if constexpr (Variant<T>)
            return std::visit([](auto&& value) { return value; }, scope);
        else
            return scope;
    }

    if constexpr (Scope<T>)
    {
        if (auto ptr = scope.scope_find(identifier[0]))
            return resolve_identifier<Rs...>(identifier.subspan<1>(), *ptr);
    }

    return {};
}

template <class... Rs>
std::optional<std::variant<std::reference_wrapper<Rs const>...>> resolve_identifier(std::span<std::string const> identifier, Variant auto const& scope)
{
    return std::visit([identifier](auto&& x) { return resolve_identifier<Rs...>(identifier, x); }, scope);
}
