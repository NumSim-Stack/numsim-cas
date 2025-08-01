#ifndef TENSOR_FUNCTIONS_FWD_H
#define TENSOR_FUNCTIONS_FWD_H

#include "../numsim_cas_type_traits.h"

namespace numsim::cas {

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_add_tensor_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_sub_tensor_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_mul_tensor_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_mul_tensor_with_scalar_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_div_tensor_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

} // namespace numsim::cas

#endif // TENSOR_FUNCTIONS_FWD_H
