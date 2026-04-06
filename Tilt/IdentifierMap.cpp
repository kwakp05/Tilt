#include <expected>
#include <string>
#include <utility>

#include "IdentifierMap.h"

void IdentifierMap::insert(std::string identifier, IdentifierValueType value)
{
    identifiers.emplace(std::move(identifier), std::move(value));
}

bool IdentifierMap::contains(std::string const& identifier) const
{
    return identifiers.contains(identifier);
}

IdentifierValueType const* IdentifierMap::scope_find(std::string const& identifier) const
{
    if (auto it = identifiers.find(identifier); it != identifiers.end())
        return &it->second;
    return nullptr;
}
