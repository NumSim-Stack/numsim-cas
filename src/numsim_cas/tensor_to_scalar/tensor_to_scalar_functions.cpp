#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_functions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/tensor/tensor_definitions.h>

#include <cassert>
#include <ranges>

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

  if (is_same<tensor_add>(expr)) {
    auto const &add = expr.get<tensor_add>();
    expression_holder<tensor_to_scalar_expression> result;
    if (add.coeff().is_valid())
      result = trace(add.coeff());
    for (auto const &child : add.symbol_map() | std::views::values) {
      if (result.is_valid())
        result = result + trace(child);
      else
        result = trace(child);
    }
    return result;
  }

  // trace(-A) -> -trace(A). Linearity through unary negation.
  if (is_same<tensor_negative>(expr)) {
    return -trace(expr.get<tensor_negative>().expr());
  }

  // trace(trans(A)) -> trace(A). Trace is invariant under transpose for
  // rank-2 tensors (basis_change_imp with indices {2,1} is the transpose).
  if (is_same<basis_change_imp>(expr)) {
    auto const &bc = expr.get<basis_change_imp>();
    if (bc.indices() == sequence{2, 1})
      return trace(bc.expr());
  }

  // trace(u ⊗ v) -> u · v. The trace assertion (rank == 2) means any
  // outer_product_wrapper reaching this point has rank-1 operands; their
  // inner product is u_i v_i.
  if (is_same<outer_product_wrapper>(expr)) {
    auto const &op = expr.get<outer_product_wrapper>();
    return dot_product(op.expr_lhs(), sequence{1}, op.expr_rhs(), sequence{1});
  }

  return make_expression<tensor_trace>(expr);
}

expression_holder<tensor_to_scalar_expression>
norm(expression_holder<tensor_expression> const &expr) {
  assert(expr.get().rank() == 2);

  if (is_same<tensor_zero>(expr))
    return make_expression<tensor_to_scalar_zero>();

  if (is_same<tensor_scalar_mul>(expr)) {
    auto const &sm = expr.get<tensor_scalar_mul>();
    return abs(sm.expr_lhs()) * norm(sm.expr_rhs());
  }

  return make_expression<tensor_norm>(expr);
}

expression_holder<tensor_to_scalar_expression>
det(expression_holder<tensor_expression> const &expr) {
  assert(expr.get().rank() == 2);

  if (is_same<tensor_zero>(expr))
    return make_expression<tensor_to_scalar_zero>();

  if (is_same<kronecker_delta>(expr))
    return make_expression<tensor_to_scalar_one>();

  if (is_same<tensor_scalar_mul>(expr)) {
    auto const &sm = expr.get<tensor_scalar_mul>();
    auto dim = static_cast<int>(sm.expr_rhs().get().dim());
    auto dim_expr = make_expression<scalar_constant>(dim);
    return pow(sm.expr_lhs(), std::move(dim_expr)) * det(sm.expr_rhs());
  }

  if (is_same<tensor_mul>(expr)) {
    auto const &mul = expr.get<tensor_mul>();
    expression_holder<tensor_to_scalar_expression> result;
    if (mul.coeff().is_valid())
      result = det(mul.coeff());
    for (auto const &child : mul.data()) {
      if (result.is_valid())
        result = result * det(child);
      else
        result = det(child);
    }
    return result;
  }

  // det(inv(A)) -> 1 / det(A).
  if (is_same<tensor_inv>(expr)) {
    auto const &inv_node = expr.get<tensor_inv>();
    return make_expression<tensor_to_scalar_one>() / det(inv_node.expr());
  }

  // det(trans(A)) -> det(A). basis_change_imp with indices {2,1} is
  // transpose for rank-2.
  if (is_same<basis_change_imp>(expr)) {
    auto const &bc = expr.get<basis_change_imp>();
    if (bc.indices() == sequence{2, 1})
      return det(bc.expr());
  }

  // det(otimes(u, v)) -> 0 when the outer product yields a rank-1 matrix
  // (rank-1 in the linear-algebra sense, i.e. a rank-2 tensor formed by
  // u_i * v_j). The outer product of two rank-1 vectors always satisfies
  // this; since we're inside det()'s rank-2 assertion, any outer product
  // here has rank-1 operands.
  if (is_same<outer_product_wrapper>(expr)) {
    return make_expression<tensor_to_scalar_zero>();
  }

  return make_expression<tensor_det>(expr);
}

} // namespace numsim::cas
