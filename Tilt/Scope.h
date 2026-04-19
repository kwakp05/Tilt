#pragma once

#include <string>
#include <type_traits>

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
