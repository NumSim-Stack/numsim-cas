#ifndef TENSOR_TO_SCALAR_FUNCTIONS_H
#define TENSOR_TO_SCALAR_FUNCTIONS_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/tensor/sequence.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>

namespace numsim::cas {

template <typename ExprLHS, typename ExprRHS>
[[nodiscard]] inline expression_holder<tensor_to_scalar_expression>
dot_product(ExprLHS &&lhs, sequence &&lhs_indices, ExprRHS &&rhs,
            sequence &&rhs_indices) {
  assert(call_tensor::rank(lhs) == lhs_indices.size() ||
         call_tensor::rank(rhs) == rhs_indices.size());
  // tensor_to_scalar_expression
  if (is_same<tensor_zero>(lhs) || is_same<tensor_zero>(rhs)) {
    return make_expression<tensor_to_scalar_zero>();
  }

  return make_expression<tensor_inner_product_to_scalar>(
      std::forward<ExprLHS>(lhs), std::move(lhs_indices),
      std::forward<ExprRHS>(rhs), std::move(rhs_indices));
}

template <typename Expr>
[[nodiscard]] inline expression_holder<tensor_to_scalar_expression>
dot(Expr &&expr) {
  return make_expression<tensor_dot>(std::forward<Expr>(expr));
}

[[nodiscard]] inline expression_holder<tensor_to_scalar_expression>
trace(expression_holder<tensor_expression> const &expr) {
  assert(expr.get().rank() == 2);
  return make_expression<tensor_trace>(expr);
}

[[nodiscard]] inline expression_holder<tensor_to_scalar_expression>
norm(expression_holder<tensor_expression> const &expr) {
  assert(expr.get().rank() == 2);
  return make_expression<tensor_norm>(expr);
}

[[nodiscard]] inline expression_holder<tensor_to_scalar_expression>
det(expression_holder<tensor_expression> const &expr) {
  assert(expr.get().rank() == 2);
  return make_expression<tensor_det>(expr);
}

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_FUNCTIONS_H
