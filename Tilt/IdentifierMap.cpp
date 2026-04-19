#include <expected>
#include <functional>
#include <string>
#include <utility>
#include <variant>

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
