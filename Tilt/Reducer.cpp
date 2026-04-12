#include <memory>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "Expression.h"
#include "Reducer.h"

Expression delta_reduce(Expression const& target, std::vector<std::string> const& identifier, Expression const& constant)
{
    return std::visit([&constant, &identifier](auto&& exp) -> Expression
        {
            using T = std::remove_cvref_t<decltype(exp)>;
            if constexpr (std::is_same_v<T, Universe>)
                return exp;
            else if constexpr (std::is_same_v<T, Identifier>)
            {
                if (exp.components == identifier)
                    return clone(constant);
                else
                    return exp;
            }
            else if constexpr (std::is_same_v<T, Function>)
            {
                return Function{
                    .param_name=exp.param_name,
                    .param_type=std::make_unique<Expression>(delta_reduce(*exp.param_type, identifier, constant)),
                    .return_type=std::make_unique<Expression>(delta_reduce(*exp.return_type, identifier, constant)),
                };
            }
            else if constexpr (std::is_same_v<T, FunctionAbstraction>)
            {
                return FunctionAbstraction{
                    .param_name=exp.param_name,
                    .param_type=std::make_unique<Expression>(delta_reduce(*exp.param_type, identifier, constant)),
                    .return_type=std::make_unique<Expression>(delta_reduce(*exp.return_type, identifier, constant)),
                };
            }
            else if constexpr (std::is_same_v<T, FunctionApplication>)
            {
                return FunctionApplication{
                    .function=std::make_unique<Expression>(delta_reduce(*exp.function, identifier, constant)),
                    .argument=std::make_unique<Expression>(delta_reduce(*exp.argument, identifier, constant)),
                };
            }
            else
                static_assert(false, "UNREACHABLE");
        }, target);
}