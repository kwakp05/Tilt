#pragma once

#include <expected>
#include <string>
#include <unordered_map>
#include <variant>

#include "Constant.h"
#include "InductiveType.h"

using IdentifierValueType = std::variant<InductiveType, Constant>;

class IdentifierMap
{
public:
    void insert(std::string identifier, IdentifierValueType value);
    bool contains(std::string const& identifier) const;
    IdentifierValueType const* scope_find(std::string const& identifier) const;

private:
    std::unordered_map<std::string, IdentifierValueType> identifiers;
};
