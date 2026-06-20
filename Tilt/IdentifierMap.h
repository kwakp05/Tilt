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

struct BoundArgument
{
    std::string name;
    Expression type;
};

class IdentifierMapWrapper
{
public:
    using ValueType = combine_variants_t<IdentifierReferenceType, std::variant<std::reference_wrapper<BoundArgument const>>>;

    explicit IdentifierMapWrapper(IdentifierMap const& wrapped);

    std::optional<ValueType> scope_find(std::string const& identifier) const;

    class BoundArgumentGuard
    {
    public:
        BoundArgumentGuard(IdentifierMapWrapper& identifiers, std::string_view name, Expression&& exp);

        ~BoundArgumentGuard();

        BoundArgumentGuard(BoundArgumentGuard const& other) = delete;
        BoundArgumentGuard(BoundArgumentGuard&& other) = delete;
        BoundArgumentGuard& operator=(BoundArgumentGuard const& other) = delete;
        BoundArgumentGuard& operator=(BoundArgumentGuard&& other) = delete;

    private:
        IdentifierMapWrapper& identifiers;
    };

    [[nodiscard]] BoundArgumentGuard emplace_bound_argument(std::string_view name, Expression&& exp);

private:
    IdentifierMap const& wrapped;
    std::vector<BoundArgument> bound_arguments;

    friend class BoundArgumentGuard;
};
