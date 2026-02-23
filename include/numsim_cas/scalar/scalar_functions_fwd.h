#ifndef SCALAR_FUNCTIONS_FWD_H
#define SCALAR_FUNCTIONS_FWD_H

#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

// template <typename ExprTypeLHS, typename ExprTypeRHS>
// constexpr result_expression_t<ExprTypeLHS, ExprTypeRHS>
// binary_scalar_add_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

// template <typename ExprTypeLHS, typename ExprTypeRHS>
// constexpr result_expression_t<ExprTypeLHS, ExprTypeRHS>
// binary_scalar_sub_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

// template <typename ExprTypeLHS, typename ExprTypeRHS>
// constexpr result_expression_t<ExprTypeLHS, ExprTypeRHS>
// binary_scalar_div_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

// template <typename ExprTypeLHS, typename ExprTypeRHS>
// constexpr result_expression_t<ExprTypeLHS, ExprTypeRHS>
// binary_scalar_mul_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

bool contains_symbol(expression_holder<scalar_expression> const &expr);

// template <typename StreamType>
// void
// print(StreamType &out,
//       expression_holder<scalar_expression> const &expr,
//       Precedence precedence);

} // namespace numsim::cas

#endif // SCALAR_FUNCTIONS_FWD_H
