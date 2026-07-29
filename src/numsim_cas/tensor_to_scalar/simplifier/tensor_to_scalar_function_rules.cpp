#include <numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_function_rules.h>

#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_functions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/tensor/tensor_assume.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_operators.h>

#include <ranges>

// Bodies mirror the previous inline folds in tensor_to_scalar_functions.cpp
// exactly; #420 extracts them into named, testable rules. The two *_of_trans
// rules are new (transpose-invariance gaps). The det terminal node + its PD/PSD
// annotation stays in det() — it is not an early-return fold.

namespace numsim::cas::t2s_rules {

// ── dot / dot_product ────────────────────────────────────────────────
std::optional<result> try_dot_product_zero(tensor_holder const &lhs,
                                           tensor_holder const &rhs) {
  if (is_same<tensor_zero>(lhs) || is_same<tensor_zero>(rhs))
    return make_expression<tensor_to_scalar_zero>();
  return {};
}
std::optional<result> try_dot_zero(tensor_holder const &e) {
  if (is_same<tensor_zero>(e))
    return make_expression<tensor_to_scalar_zero>();
  return {};
}

// ── trace ────────────────────────────────────────────────────────────
std::optional<result> try_trace_zero(tensor_holder const &e) {
  if (is_same<tensor_zero>(e))
    return make_expression<tensor_to_scalar_zero>();
  return {};
}
std::optional<result> try_trace_identity(tensor_holder const &e) {
  // tr(I) = dim. Rank-2 asserted by the caller, so any identity_tensor here
  // is the rank-2 Kronecker delta (#188 unified kronecker_delta).
  if (is_same<identity_tensor>(e)) {
    auto dim = e.get().dim();
    return make_expression<tensor_to_scalar_scalar_wrapper>(
        make_expression<scalar_constant>(static_cast<int>(dim)));
  }
  return {};
}
std::optional<result> try_trace_of_trans(tensor_holder const &e) {
  // tr(Aᵀ) = tr(A). trans() builds permute_indices_wrapper{2,1}; match the
  // index sequence so a non-transpose permutation is not mis-simplified.
  if (is_same<permute_indices_wrapper>(e)) {
    auto const &perm = e.get<permute_indices_wrapper>();
    if (perm.indices() == sequence{2, 1})
      return trace(perm.expr());
  }
  return {};
}
std::optional<result> try_trace_scalar_mul(tensor_holder const &e) {
  if (is_same<tensor_scalar_mul>(e)) {
    auto const &sm = e.get<tensor_scalar_mul>();
    return sm.expr_lhs() * trace(sm.expr_rhs());
  }
  return {};
}
std::optional<result> try_trace_add(tensor_holder const &e) {
  if (is_same<tensor_add>(e)) {
    auto const &add = e.get<tensor_add>();
    result r;
    if (add.coeff().is_valid())
      r = trace(add.coeff());
    for (auto const &child : add.symbol_map() | std::views::values) {
      if (r.is_valid())
        r = r + trace(child);
      else
        r = trace(child);
    }
    return r;
  }
  return {};
}

// ── norm ─────────────────────────────────────────────────────────────
std::optional<result> try_norm_zero(tensor_holder const &e) {
  if (is_same<tensor_zero>(e))
    return make_expression<tensor_to_scalar_zero>();
  return {};
}
std::optional<result> try_norm_of_trans(tensor_holder const &e) {
  // ‖Aᵀ‖ = ‖A‖ — the Frobenius norm is transpose-invariant.
  if (is_same<permute_indices_wrapper>(e)) {
    auto const &perm = e.get<permute_indices_wrapper>();
    if (perm.indices() == sequence{2, 1})
      return norm(perm.expr());
  }
  return {};
}
std::optional<result> try_norm_scalar_mul(tensor_holder const &e) {
  if (is_same<tensor_scalar_mul>(e)) {
    auto const &sm = e.get<tensor_scalar_mul>();
    return abs(sm.expr_lhs()) * norm(sm.expr_rhs());
  }
  return {};
}

// ── det ──────────────────────────────────────────────────────────────
std::optional<result> try_det_zero(tensor_holder const &e) {
  if (is_same<tensor_zero>(e))
    return make_expression<tensor_to_scalar_zero>();
  return {};
}
std::optional<result> try_det_identity(tensor_holder const &e) {
  if (is_same<identity_tensor>(e))
    return make_expression<tensor_to_scalar_one>();
  return {};
}
std::optional<result> try_det_chirality(tensor_holder const &e) {
  // det of an orthogonal tensor is ±1, resolved by chirality (#269):
  // proper → +1, improper → −1, bare orthogonal → NO fold (sign unknown).
  if (is_proper_rotation(e))
    return make_expression<tensor_to_scalar_one>();
  if (is_improper_rotation(e))
    return -make_expression<tensor_to_scalar_one>();
  return {};
}
std::optional<result> try_det_inv(tensor_holder const &e) {
  // det(A⁻¹) = 1/det(A), routed through t2s div → canonical pow(det(A),-1).
  if (is_same<tensor_inv>(e)) {
    auto const &inner = e.get<tensor_inv>().expr();
    return make_expression<tensor_to_scalar_one>() / det(inner);
  }
  return {};
}
std::optional<result> try_det_trans(tensor_holder const &e) {
  if (is_same<permute_indices_wrapper>(e)) {
    auto const &perm = e.get<permute_indices_wrapper>();
    if (perm.indices() == sequence{2, 1})
      return det(perm.expr());
  }
  return {};
}
std::optional<result> try_det_outer_product(tensor_holder const &e) {
  // det(u ⊗ v) = 0 for dim ≥ 2 (rank-1 matrix). dim = 1 is the 1×1 scalar.
  if (is_same<outer_product_wrapper>(e) && e.get().dim() >= 2)
    return make_expression<tensor_to_scalar_zero>();
  return {};
}
std::optional<result> try_det_scalar_mul(tensor_holder const &e) {
  if (is_same<tensor_scalar_mul>(e)) {
    auto const &sm = e.get<tensor_scalar_mul>();
    auto dim = static_cast<int>(sm.expr_rhs().get().dim());
    auto dim_expr = make_expression<scalar_constant>(dim);
    return pow(sm.expr_lhs(), std::move(dim_expr)) * det(sm.expr_rhs());
  }
  return {};
}
std::optional<result> try_det_mul(tensor_holder const &e) {
  if (is_same<tensor_mul>(e)) {
    auto const &mul = e.get<tensor_mul>();
    result r;
    if (mul.coeff().is_valid())
      r = det(mul.coeff());
    for (auto const &child : mul.data()) {
      if (r.is_valid())
        r = r * det(child);
      else
        r = det(child);
    }
    return r;
  }
  return {};
}

} // namespace numsim::cas::t2s_rules
