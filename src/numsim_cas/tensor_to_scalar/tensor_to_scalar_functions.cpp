#include <numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_function_rules.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_functions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/cas_error.h>
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

  if (auto r = t2s_rules::try_dot_product_zero(lhs, rhs))
    return *r;

  return make_expression<tensor_inner_product_to_scalar>(
      lhs, std::move(lhs_indices), rhs, std::move(rhs_indices));
}

expression_holder<tensor_to_scalar_expression>
dot(expression_holder<tensor_expression> const &expr) {
  if (auto r = t2s_rules::try_dot_zero(expr))
    return *r;

  return make_expression<tensor_dot>(expr);
}

expression_holder<tensor_to_scalar_expression>
trace(expression_holder<tensor_expression> const &expr) {
  assert(expr.get().rank() == 2);

  if (auto r = t2s_rules::try_trace_zero(expr))
    return *r;
  if (auto r = t2s_rules::try_trace_identity(expr))
    return *r;
  if (auto r = t2s_rules::try_trace_of_trans(expr))
    return *r;
  if (auto r = t2s_rules::try_trace_scalar_mul(expr))
    return *r;
  if (auto r = t2s_rules::try_trace_add(expr))
    return *r;

  return make_expression<tensor_trace>(expr);
}

expression_holder<tensor_to_scalar_expression>
norm(expression_holder<tensor_expression> const &expr) {
  assert(expr.get().rank() == 2);

  if (auto r = t2s_rules::try_norm_zero(expr))
    return *r;
  if (auto r = t2s_rules::try_norm_of_trans(expr))
    return *r;
  if (auto r = t2s_rules::try_norm_scalar_mul(expr))
    return *r;

  return make_expression<tensor_norm>(expr);
}

expression_holder<tensor_to_scalar_expression>
det(expression_holder<tensor_expression> const &expr) {
  assert(expr.get().rank() == 2);

  if (auto r = t2s_rules::try_det_zero(expr))
    return *r;
  if (auto r = t2s_rules::try_det_identity(expr))
    return *r;
  if (auto r = t2s_rules::try_det_chirality(expr))
    return *r;
  if (auto r = t2s_rules::try_det_inv(expr))
    return *r;
  if (auto r = t2s_rules::try_det_trans(expr))
    return *r;
  if (auto r = t2s_rules::try_det_outer_product(expr))
    return *r;
  if (auto r = t2s_rules::try_det_scalar_mul(expr))
    return *r;
  if (auto r = t2s_rules::try_det_mul(expr))
    return *r;

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
