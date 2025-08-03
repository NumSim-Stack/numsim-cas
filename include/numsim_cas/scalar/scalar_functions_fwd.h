#ifndef SCALAR_FUNCTIONS_FWD_H
#define SCALAR_FUNCTIONS_FWD_H

#include "../numsim_cas_type_traits.h"

namespace numsim::cas {

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

} // namespace numsim::cas

#endif // SCALAR_FUNCTIONS_FWD_H
