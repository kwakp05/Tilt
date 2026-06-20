#include <algorithm>
#include <expected>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include "Expression.h"
#include "IdentifierMap.h"

void IdentifierMap::insert(std::string identifier, IdentifierValueType value)
{
    identifiers.emplace(std::move(identifier), std::move(value));
}

bool IdentifierMap::contains(std::string const& identifier) const
{
    return identifiers.contains(identifier);
}

std::optional<IdentifierReferenceType> IdentifierMap::scope_find(std::string const& identifier) const
{
    if (auto it = identifiers.find(identifier); it != identifiers.end())
        return std::visit(
            [](auto const& x) -> IdentifierReferenceType { return std::cref(x); },
            it->second
        );
    return {};
}

IdentifierMapWrapper::IdentifierMapWrapper(IdentifierMap const& wrapped) : wrapped(wrapped) {}

std::optional<IdentifierMapWrapper::ValueType> IdentifierMapWrapper::scope_find(std::string const& identifier) const
{
    auto matches = [&identifier](auto const& x) { return x == identifier; };
    if (auto result = std::ranges::find_if(bound_arguments, matches, &BoundArgument::name); result != bound_arguments.end())
        return *result;

    return wrapped.scope_find(identifier)
        .transform([](IdentifierReferenceType t)
            {
                return std::visit([](auto&& x) -> ValueType { return std::cref(x); }, t);
            });
}

IdentifierMapWrapper::BoundArgumentGuard IdentifierMapWrapper::emplace_bound_argument(std::string_view name, Expression&& exp)
{
    return BoundArgumentGuard{ *this, std::string{name}, std::move(exp)};
}

IdentifierMapWrapper::BoundArgumentGuard::BoundArgumentGuard(IdentifierMapWrapper& identifiers, std::string_view name, Expression&& exp)
    : identifiers(identifiers)
{
    identifiers.bound_arguments.emplace_back(std::string{ name }, std::move(exp));
}

IdentifierMapWrapper::BoundArgumentGuard::~BoundArgumentGuard()
{
    identifiers.bound_arguments.pop_back();
}
