#include <algorithm>
#include <expected>
#include <format>
#include <memory>
#include <ranges>
#include <string>

#include "Expression.h"
#include "InductiveType.h"

Constructor const* InductiveType::scope_find(std::string const& identifier) const
{
    if (auto result = std::ranges::find(constructors, identifier, &Constructor::name); result != constructors.end())
        return std::to_address(result);
    return nullptr;
}

std::expected<InductiveType, std::string> create_inductive_type(ParsedInductiveType p)
{
    auto type = create_expression(p.type);
    if (!type)
        return std::unexpected(type.error());

    std::vector<Constructor> constructors;
    constructors.reserve(p.constructors.size());

    for (ParsedConstructor const& c : p.constructors)
    {
        auto res = create_constructor(c);
        if (!res)
            return std::unexpected(res.error());
        constructors.push_back(std::move(*res));
    }

    return InductiveType
    {
        .name = std::string{p.identifier.identifier},
        .type = std::move(*type),
        .constructors = std::move(constructors)
    };
}

std::expected<Constructor, std::string> create_constructor(ParsedConstructor p)
{
    return create_expression(p.type)
        .transform([&p](Expression&& exp)
        {
                return Constructor
                {
                    .name = std::string{p.identifier.identifier},
                    .type = std::move(exp)
                };
        });
}

std::string to_pretty_string(InductiveType const& p)
{
    std::string output = std::format("inductive {} : {} where", p.name, to_pretty_string(p.type));
    for (Constructor const& c : p.constructors)
    {
        output += std::format("\n| {} : {}", c.name, to_pretty_string(c.type));
    }
    return output;
}