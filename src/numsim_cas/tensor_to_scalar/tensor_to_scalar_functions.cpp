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

  // trace(I) = dim. The asserted rank-2 input means any identity_tensor
  // reaching here is the Kronecker delta (rank-2 identity).
  if (is_same<identity_tensor>(expr)) {
    auto dim = expr.get().dim();
    return make_expression<tensor_to_scalar_scalar_wrapper>(
        make_expression<scalar_constant>(static_cast<int>(dim)));
  }

  // trace(identity_tensor at rank 2) = dim. The general identity_tensor
  // exists as a separate node from kronecker_delta because it carries a
  // rank parameter for differentiation results; here it can only be rank
  // 2 (enforced by the function's leading assert).
  if (is_same<identity_tensor>(expr)) {
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

  // det(I) = 1 at rank 2 (the asserted rank-2 input means any
  // identity_tensor reaching here is the rank-2 Kronecker delta).
  if (is_same<identity_tensor>(expr))
    return make_expression<tensor_to_scalar_one>();

  // det(identity_tensor at rank 2) = 1. Like trace above, identity_tensor
  // and kronecker_delta are distinct nodes; both fold at rank 2.
  if (is_same<identity_tensor>(expr))
    return make_expression<tensor_to_scalar_one>();

  // det(inv(A)) = 1/det(A). Routes through the t2s div operator which
  // composes via pow(rhs, -1) — produces canonical pow(det(A), -1).
  if (is_same<tensor_inv>(expr)) {
    auto const &inner = expr.get<tensor_inv>().expr();
    return make_expression<tensor_to_scalar_one>() / det(inner);
  }

  // det(trans(A)) = det(A). trans() builds permute_indices_wrapper with
  // sequence{2, 1}; det is rank-2 only (asserted), so any
  // permute_indices_wrapper reaching here is necessarily the transpose.
  // Still match on the index sequence so a future caller passing a
  // non-transpose permutation doesn't get a wrong simplification.
  if (is_same<permute_indices_wrapper>(expr)) {
    auto const &perm = expr.get<permute_indices_wrapper>();
    if (perm.indices() == sequence{2, 1})
      return det(perm.expr());
  }

  // det(u ⊗ v) = 0 for dim ≥ 2. The outer product u ⊗ v of two rank-1
  // tensors is a rank-2 matrix of rank 1 (linear-algebra sense), so its
  // determinant is zero for any n×n with n ≥ 2. For dim = 1 the matrix
  // is the 1×1 scalar u₀·v₀ — don't fold.
  if (is_same<outer_product_wrapper>(expr) && expr.get().dim() >= 2)
    return make_expression<tensor_to_scalar_zero>();

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

  return make_expression<tensor_det>(expr);
}

} // namespace numsim::cas
