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
    auto project_name = [](auto&& identifier_variant) { return std::visit([](auto&& x) { return x.name; }, identifier_variant); };
    auto matches = [&identifier](auto const& x) { return x == identifier; };
    if (auto result = std::ranges::find_last_if(extra_identifiers, matches, project_name); !result.empty())
        return std::visit([](auto&& x) -> ValueType { return x; }, *result.begin());

    return wrapped.scope_find(identifier)
        .transform([](IdentifierReferenceType t)
            {
                return std::visit([](auto&& x) -> ValueType { return std::cref(x); }, t);
            });
}

IdentifierMapWrapper::BoundIdentifierGuard IdentifierMapWrapper::emplace_bound_identifier(std::string_view name, Expression&& exp)
{
    return BoundIdentifierGuard{ *this, std::string{name}, std::move(exp)};
}

IdentifierMapWrapper::BoundIdentifierGuard::BoundIdentifierGuard(IdentifierMapWrapper& identifiers, std::string_view name, Expression&& exp)
    : identifiers(identifiers)
{
    identifiers.extra_identifiers.emplace_back(std::in_place_type<BoundIdentifier>, std::string{ name }, std::move(exp));
}

IdentifierMapWrapper::BoundIdentifierGuard::~BoundIdentifierGuard()
{
    identifiers.extra_identifiers.pop_back();
}

IdentifierMapWrapper::SubstitutedIdentifierGuard IdentifierMapWrapper::emplace_substituted_identifier(std::string_view name, Expression&& exp)
{
    return SubstitutedIdentifierGuard{ *this, std::string{name}, std::move(exp)};
}

IdentifierMapWrapper::SubstitutedIdentifierGuard::SubstitutedIdentifierGuard(IdentifierMapWrapper& identifiers, std::string_view name, Expression&& exp)
    : identifiers(identifiers)
{
    identifiers.extra_identifiers.emplace_back(std::in_place_type<SubstitutedIdentifier>, std::string{ name }, std::move(exp));
}

IdentifierMapWrapper::SubstitutedIdentifierGuard::~SubstitutedIdentifierGuard()
{
    identifiers.extra_identifiers.pop_back();
}
