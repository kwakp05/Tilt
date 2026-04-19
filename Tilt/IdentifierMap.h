#pragma once

#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

#include "Constant.h"
#include "InductiveType.h"

using IdentifierValueType = std::variant<InductiveType, Constructor, Constant>;
using IdentifierReferenceType = std::variant<
    std::reference_wrapper<InductiveType const>,
    std::reference_wrapper<Constructor const>,
    std::reference_wrapper<Constant const>
>;

class IdentifierMap
{
public:
    void insert(std::string identifier, IdentifierValueType value);
    bool contains(std::string const& identifier) const;
    std::optional<IdentifierReferenceType> scope_find(std::string const& identifier) const;

private:
    std::unordered_map<std::string, IdentifierValueType> identifiers;
};
