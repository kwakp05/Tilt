#pragma once

#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

#include "Constant.h"
#include "InductiveType.h"
#include "VariantUtils.h"

using IdentifierValueType = std::variant<InductiveType, Constructor, Constant, Recursor>;
using IdentifierReferenceType = std::variant<
    std::reference_wrapper<InductiveType const>,
    std::reference_wrapper<Constructor const>,
    std::reference_wrapper<Constant const>,
    std::reference_wrapper<Recursor const>
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

struct BoundIdentifier
{
    std::string name;
    Expression type;
};

struct SubstitutedIdentifier
{
    std::string name;
    Expression value;
};

class IdentifierMapWrapper
{
public:
    using ValueType = combine_variants_t<
        IdentifierReferenceType,
        std::variant<std::reference_wrapper<BoundIdentifier const>, std::reference_wrapper<SubstitutedIdentifier const>>
    >;

    explicit IdentifierMapWrapper(IdentifierMap const& wrapped);

    std::optional<ValueType> scope_find(std::string const& identifier) const;

    class BoundIdentifierGuard
    {
    public:
        BoundIdentifierGuard(IdentifierMapWrapper& identifiers, std::string_view name, Expression&& exp);

        ~BoundIdentifierGuard();

        BoundIdentifierGuard(BoundIdentifierGuard const& other) = delete;
        BoundIdentifierGuard(BoundIdentifierGuard&& other) = delete;
        BoundIdentifierGuard& operator=(BoundIdentifierGuard const& other) = delete;
        BoundIdentifierGuard& operator=(BoundIdentifierGuard&& other) = delete;

    private:
        IdentifierMapWrapper& identifiers;
    };

    class SubstitutedIdentifierGuard
    {
    public:
        SubstitutedIdentifierGuard(IdentifierMapWrapper& identifiers, std::string_view name, Expression&& exp);

        ~SubstitutedIdentifierGuard();

        SubstitutedIdentifierGuard(SubstitutedIdentifierGuard const& other) = delete;
        SubstitutedIdentifierGuard(SubstitutedIdentifierGuard&& other) = delete;
        SubstitutedIdentifierGuard& operator=(SubstitutedIdentifierGuard const& other) = delete;
        SubstitutedIdentifierGuard& operator=(SubstitutedIdentifierGuard&& other) = delete;

    private:
        IdentifierMapWrapper& identifiers;
    };

    [[nodiscard]] BoundIdentifierGuard emplace_bound_identifier(std::string_view name, Expression&& exp);
    [[nodiscard]] SubstitutedIdentifierGuard emplace_substituted_identifier(std::string_view name, Expression&& exp);

private:
    IdentifierMap const& wrapped;
    std::vector<std::variant<BoundIdentifier, SubstitutedIdentifier>> extra_identifiers;

    friend class BoundIdentifierGuard;
    friend class SubstitutedIdentifierGuard;
};
