#ifndef SCALAR_BINARY_SIMPLIFY_FWD_H
#define SCALAR_BINARY_SIMPLIFY_FWD_H

#include <numsim_cas/numsim_cas_type_traits.h>

namespace numsim::cas {
// template <typename ExprTypeLHS, typename ExprTypeRHS>
// result_expression_t<ExprTypeLHS, ExprTypeRHS>
// binary_scalar_add_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

// template <typename ExprTypeLHS, typename ExprTypeRHS>
// result_expression_t<ExprTypeLHS, ExprTypeRHS>
// binary_scalar_sub_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

// template <typename ExprTypeLHS, typename ExprTypeRHS>
// result_expression_t<ExprTypeLHS, ExprTypeRHS>
// binary_scalar_div_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

// template <typename ExprTypeLHS, typename ExprTypeRHS>
// result_expression_t<ExprTypeLHS, ExprTypeRHS>
// binary_scalar_mul_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

expression_holder<scalar_expression>
binary_scalar_pow_simplify(expression_holder<scalar_expression> lhs,
                           expression_holder<scalar_expression> rhs);
} // namespace numsim::cas

#endif // SCALAR_BINARY_SIMPLIFY_FWD_H
