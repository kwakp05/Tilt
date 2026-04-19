#include <expected>
#include <format>
#include <string>
#include <utility>

#include "Constant.h"
#include "Expression.h"

std::expected<Constant, std::string> create_constant(ParsedConstant p)
{
    auto type = create_expression(p.type);
    if (!type)
        return std::unexpected(type.error());

    auto value = create_expression(p.value);
    if (!value)
        return std::unexpected(value.error());

    return Constant{ .name = std::string{p.identifier}, .type = std::move(*type), .value = std::move(*value) };
}

std::string to_pretty_string(Constant const& c)
{
    return std::format("def {} : {} := {}", c.name, to_pretty_string(c.type), to_pretty_string(c.value));
}
