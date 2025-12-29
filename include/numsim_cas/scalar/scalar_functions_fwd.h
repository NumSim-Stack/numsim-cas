#ifndef SCALAR_FUNCTIONS_FWD_H
#define SCALAR_FUNCTIONS_FWD_H

#include "../numsim_cas_type_traits.h"
#include "../printer_base.h"
#include "scalar_expression.h"

namespace numsim::cas {

template <typename ValueType>
inline auto diff(expression_holder<scalar_expression<ValueType>> const &expr,
                 expression_holder<scalar_expression<ValueType>> const &arg);

template <typename ExprTypeLHS, typename ExprTypeRHS>
constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_scalar_add_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

template <typename ExprTypeLHS, typename ExprTypeRHS>
constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_scalar_sub_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

template <typename ExprTypeLHS, typename ExprTypeRHS>
constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_scalar_div_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

template <typename ExprTypeLHS, typename ExprTypeRHS>
constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_scalar_mul_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

template <typename ValueType>
bool contains_symbol(
    expression_holder<scalar_expression<ValueType>> const &expr);

template <typename ValueType, typename StreamType>
constexpr inline void
print(StreamType &out,
      expression_holder<scalar_expression<ValueType>> const &expr,
      Precedence precedence = Precedence::None);

} // namespace numsim::cas

#endif // SCALAR_FUNCTIONS_FWD_H
