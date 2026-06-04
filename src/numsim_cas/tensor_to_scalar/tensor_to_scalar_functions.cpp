#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_functions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/tensor/tensor_assume.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_operators.h>

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
  // reaching here is the rank-2 Kronecker delta (since #188 unified
  // kronecker_delta into identity_tensor).
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
  // identity_tensor reaching here is the rank-2 Kronecker delta;
  // kronecker_delta was unified into identity_tensor by #188).
  if (is_same<identity_tensor>(expr))
    return make_expression<tensor_to_scalar_one>();

  // det(orthogonal R) = +1 for proper rotations. The "orthogonal"
  // annotation in this codebase doesn't distinguish proper rotations
  // (det = +1) from improper ones / reflections (det = -1); the
  // common continuum-mechanics use case is proper rotation so we
  // default to +1. A future `chirality` sub-tag could refine this.
  // Closes one half of #246.
  if (is_orthogonal(expr))
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

  auto result = make_expression<tensor_det>(expr);
  // Propagate positivity from PD/PSD annotations on the input (#246
  // α-2d). PD ⇒ det > 0; PSD ⇒ det ≥ 0. Insert into the t2s result's
  // numeric_assumption_manager directly — the same one inherited from
  // the expression base class that scalar assume() writes to. Mirrors
  // scalar_assume.h's joint-insertion pattern.
  //
  // Limitation: this fires only on the terminal tensor_det node. The
  // earlier structural folds (det(α·A) → α^d·det(A), det(inv) →
  // 1/det(A), det(tensor_mul) → ∏det) compose results through t2s
  // mul/div/pow which do NOT yet propagate `positive` through the
  // operation. So `is_positive(det(α·PD))` is currently false even
  // though it should be true. See #260 (t2s op propagation) and #261
  // (t2s constants annotation) for the broader fix; this PR closes
  // only the terminal-leaf case.
  //
  // Branches are two if's rather than if/else if to be robust against
  // direct-manager callers who insert positive_definite{} alone (the
  // joint PD ⇒ PSD insertion comes from assume_positive_definite, not
  // from the manager itself).
  auto &a = result.data()->assumptions();
  if (is_positive_definite(expr)) {
    a.insert(positive{});
    a.insert(nonnegative{});
    a.insert(nonzero{});
    a.insert(real_tag{});
    a.set_inferred(); // forward-compat: a future t2s assumption
                      // propagator should treat these as already-known
                      // facts, not as candidates for re-derivation.
  }
  if (is_positive_semidefinite(expr)) {
    a.insert(nonnegative{});
    a.insert(real_tag{});
    a.set_inferred();
  }
  return result;
}

// ─── Principal invariants (#226 cheap deliverable) ─────────────────────
// I1(A) = tr(A); I2(A) = (tr(A)^2 - tr(A^2)) / 2; I3(A) = det(A).
// Compositions of existing primitives — no new AST nodes.

expression_holder<tensor_to_scalar_expression>
first_invariant(expression_holder<tensor_expression> const &expr) {
  return trace(expr);
}

expression_holder<tensor_to_scalar_expression>
second_invariant(expression_holder<tensor_expression> const &expr) {
  // For a rank-2 tensor A: I2 = (tr(A)^2 - tr(A·A)) / 2.
  // The A*A product uses the existing single-contraction tensor-tensor
  // mul_fn (rank 2*2 = 2). Zero-short-circuits and trace simplifiers
  // fire through the composition automatically.
  auto tr_A = trace(expr);
  auto tr_AA = trace(expr * expr);
  // (tr_A * tr_A - tr_AA) / 2. Use a scalar_constant(2) wrapped as t2s.
  auto two = make_expression<tensor_to_scalar_scalar_wrapper>(
      make_expression<scalar_constant>(2));
  return (tr_A * tr_A - tr_AA) / two;
}

expression_holder<tensor_to_scalar_expression>
third_invariant(expression_holder<tensor_expression> const &expr) {
  return det(expr);
}

} // namespace numsim::cas
