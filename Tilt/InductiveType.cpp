#include <algorithm>
#include <expected>
#include <format>
#include <memory>
#include <ranges>
#include <string>

#include "Expression.h"
#include "InductiveType.h"

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

    Recursor recursor = create_recursor(std::string{ p.identifier.identifier }, constructors, 1);

    return InductiveType
    {
        .name = std::string{p.identifier.identifier},
        .type = std::move(*type),
        .constructors = std::move(constructors),
        .rec = std::move(recursor)
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

namespace
{

NamedExpression create_minor_premise(std::string const& type_name, Constructor const& c)
{
    std::vector<NamedExpression> function_terms = get_function_args(c.type)
        | std::views::transform([](NamedExpressionView&& x) { return NamedExpression(x); })
        | std::ranges::to<std::vector<NamedExpression>>();

    Expression constructor_instance = [&c, &type_name, &function_terms]()
        {
            std::vector<Expression> terms;
            terms.emplace_back(Identifier{ {type_name, c.name} });
            terms.append_range(function_terms | std::views::transform([](NamedExpression const& x) { return Identifier{{x.name}}; }));
            return create_function_application_from_terms(terms);
        }();

    function_terms.emplace_back(
        "",
        FunctionApplication
        {
            .function = std::make_unique<Expression>(Identifier{{"motive"}}),
            .argument = std::make_unique<Expression>(std::move(constructor_instance))
        }
    );

    return NamedExpression
    {
        c.name,
        create_function_from_terms(function_terms)
    };
}
}

Recursor create_recursor(std::string const& name, std::vector<Constructor> const& constructors, int motive_universe)
{
    NamedExpression motive = NamedExpression{
        "motive",
        Function
        {
            .param_name = "_",
            .param_type = std::make_unique<Expression>(Identifier{{name}}),
            .return_type = std::make_unique<Expression>(Universe{motive_universe}),
        }
    };

    std::vector<NamedExpression> minor_premises = constructors
        | std::views::transform([&name](Constructor const& c) { return create_minor_premise(name, c); })
        | std::ranges::to<std::vector<NamedExpression>>();

    NamedExpression major_premise = NamedExpression{ "t", Identifier{{name}} };

    NamedExpression return_type = NamedExpression{
        "",
        FunctionApplication
        {
            .function = std::make_unique<Expression>(Identifier{{"motive"}}),
            .argument = std::make_unique<Expression>(Identifier{{"t"}}),
        }
    };

    std::vector<NamedExpression> terms;
    terms.reserve(minor_premises.size() + 3);

    terms.emplace_back(std::move(motive));
    std::ranges::move(minor_premises, std::back_inserter(terms));
    terms.emplace_back(std::move(major_premise));
    terms.emplace_back(std::move(return_type));

    return Recursor{ create_function_from_terms(terms) };
}

Constructor clone(Constructor const& c)
{
    return Constructor{.name=c.name, .type=clone(c.type)};
}

Recursor clone(Recursor const& rec)
{
    return Recursor{ clone(rec.type) };
}

std::string to_pretty_string(InductiveType const& p)
{
    std::string output = std::format("inductive {} : {} where", p.name, to_pretty_string(p.type));
    for (Constructor const& c : p.constructors)
    {
        output += "\n";
        output += to_pretty_string(c);
    }
    return output;
}

std::string to_pretty_string(Constructor const& c)
{
    return std::format("{} : {}", c.name, to_pretty_string(c.type));
}

std::string to_pretty_string(Recursor const& rec)
{
    return to_pretty_string(rec.type);
}