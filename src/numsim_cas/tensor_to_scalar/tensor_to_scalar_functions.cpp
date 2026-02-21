#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_functions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/tensor/tensor_definitions.h>

#include <cassert>

namespace numsim::cas {

expression_holder<tensor_to_scalar_expression> dot_product(
    expression_holder<tensor_expression> const &lhs, sequence &&lhs_indices,
    expression_holder<tensor_expression> const &rhs, sequence &&rhs_indices) {
  assert(call_tensor::rank(lhs) == lhs_indices.size() ||
         call_tensor::rank(rhs) == rhs_indices.size());

  if (is_same<tensor_zero>(lhs) || is_same<tensor_zero>(rhs))
    return make_expression<tensor_to_scalar_zero>();

  return make_expression<tensor_inner_product_to_scalar>(
      lhs, std::move(lhs_indices), rhs, std::move(rhs_indices));
}

expression_holder<tensor_to_scalar_expression>
dot(expression_holder<tensor_expression> const &expr) {
  if (is_same<tensor_zero>(expr))
    return make_expression<tensor_to_scalar_zero>();

  return make_expression<tensor_dot>(expr);
}

expression_holder<tensor_to_scalar_expression>
trace(expression_holder<tensor_expression> const &expr) {
  assert(expr.get().rank() == 2);

  if (is_same<tensor_zero>(expr))
    return make_expression<tensor_to_scalar_zero>();

  if (is_same<kronecker_delta>(expr)) {
    auto dim = expr.get().dim();
    return make_expression<tensor_to_scalar_scalar_wrapper>(
        make_expression<scalar_constant>(static_cast<int>(dim)));
  }

  if (is_same<tensor_scalar_mul>(expr)) {
    auto const &sm = expr.get<tensor_scalar_mul>();
    return sm.expr_lhs() * trace(sm.expr_rhs());
  }

  return make_expression<tensor_trace>(expr);
}

expression_holder<tensor_to_scalar_expression>
norm(expression_holder<tensor_expression> const &expr) {
  assert(expr.get().rank() == 2);

  if (is_same<tensor_zero>(expr))
    return make_expression<tensor_to_scalar_zero>();

  return make_expression<tensor_norm>(expr);
}

expression_holder<tensor_to_scalar_expression>
det(expression_holder<tensor_expression> const &expr) {
  assert(expr.get().rank() == 2);

  if (is_same<tensor_zero>(expr))
    return make_expression<tensor_to_scalar_zero>();

  if (is_same<kronecker_delta>(expr))
    return make_expression<tensor_to_scalar_one>();

  return make_expression<tensor_det>(expr);
}

} // namespace numsim::cas
