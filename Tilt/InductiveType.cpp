#include <ranges>

#include "Expression.h"
#include "InductiveType.h"

InductiveType create_inductive_type(ParsedInductiveType p)
{
    return InductiveType
    {
        .name = std::string{p.identifier.identifier},
        .type = create_expression(p.type),
        .constructors = p.constructors
            | std::views::transform([](ParsedConstructor parsed_constructor) { return create_constructor(parsed_constructor); })
            | std::ranges::to<std::vector<Constructor>>()
    };
}

Constructor create_constructor(ParsedConstructor p)
{
    return Constructor
    {
        .name = std::string{p.identifier.identifier},
        .type = create_expression(p.type)
    };
}