#ifndef NUMSIM_CAS_TENSOR_FUNCTIONS_H
#define NUMSIM_CAS_TENSOR_FUNCTIONS_H

#include <cstdlib>
#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/tensor/data/tensor_data_make_imp.h>
#include <numsim_cas/tensor/identity_tensor.h>
#include <numsim_cas/tensor/projection_tensor.h>
#include <numsim_cas/tensor/projector_algebra.h>
#include <numsim_cas/tensor/sequence.h>
#include <numsim_cas/tensor/skew_classification.h>
#include <numsim_cas/tensor/tensor_assume.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/tensor_zero.h>
#include <numsim_cas/tensor/visitors/tensor_printer.h>
#include <optional>
#include <vector>

namespace numsim::cas {

template <typename T>
constexpr inline auto make_tensor_data(std::size_t dim, std::size_t rank) {
  return make_tensor_data_imp<T>().evaluate(dim, rank);
}

template <tensor_expr_holder Expr>
[[nodiscard]] inline expression_holder<tensor_expression> dev(Expr &&expr) {
  // Rank gate: deviatoric projection is defined for rank-2 tensors only.
  // Reject other ranks at construction so silently-invalid expressions
  // can't reach the evaluator. Closes #53.
  if (expr.get().rank() != 2)
    throw invalid_expression_error(
        "dev: only rank-2 tensors are supported (got rank " +
        std::to_string(expr.get().rank()) + ")");
  if (is_same<identity_tensor>(expr) && expr.get().rank() == 2)
    return make_expression<tensor_zero>(expr.get().dim(), expr.get().rank());
  if (auto const &sp = expr.get().space()) {
    if (auto rule = contraction_rule(ProjKind::Dev, classify_space(*sp))) {
      if (*rule == ContractionRule::Idempotent ||
          *rule == ContractionRule::RhsSubspace)
        return expr;
      if (*rule == ContractionRule::Zero)
        return make_expression<tensor_zero>(expr.get().dim(),
                                            expr.get().rank());
      // LhsSubspace: projector is stricter — proceed to normal projection
    }
  }
  if (auto simp = try_simplify_projector_contraction(ProjKind::Dev, expr)) {
    if (simp->rule == ContractionRule::Zero)
      return make_expression<tensor_zero>(simp->dim, expr.get().rank());
    return apply_projection(simp->result_kind, simp->argument);
  }
  if (is_same<tensor_scalar_mul>(expr)) {
    auto const &sm = expr.template get<tensor_scalar_mul>();
    return sm.expr_lhs() * dev(sm.expr_rhs());
  }
  if (is_same<tensor_negative>(expr)) {
    auto const &neg = expr.template get<tensor_negative>();
    return -dev(neg.expr());
  }
  if (is_same<tensor_zero>(expr))
    return make_expression<tensor_zero>(expr.get().dim(), expr.get().rank());
  auto d = expr.get().dim();
  auto result = inner_product(P_devi(d), sequence{3, 4},
                              std::forward<Expr>(expr), sequence{1, 2});
  result.data()->set_space(space_for_kind(ProjKind::Dev));
  return result;
}

template <tensor_expr_holder Expr>
[[nodiscard]] inline expression_holder<tensor_expression> sym(Expr &&expr) {
  if (expr.get().rank() != 2)
    throw invalid_expression_error(
        "sym: only rank-2 tensors are supported (got rank " +
        std::to_string(expr.get().rank()) + ")");
  if (is_same<identity_tensor>(expr) && expr.get().rank() == 2)
    return expr;
  if (auto const &sp = expr.get().space()) {
    if (auto rule = contraction_rule(ProjKind::Sym, classify_space(*sp))) {
      if (*rule == ContractionRule::Idempotent ||
          *rule == ContractionRule::RhsSubspace)
        return expr;
      if (*rule == ContractionRule::Zero)
        return make_expression<tensor_zero>(expr.get().dim(),
                                            expr.get().rank());
    }
  }
  if (auto simp = try_simplify_projector_contraction(ProjKind::Sym, expr)) {
    if (simp->rule == ContractionRule::Zero)
      return make_expression<tensor_zero>(simp->dim, expr.get().rank());
    return apply_projection(simp->result_kind, simp->argument);
  }
  if (is_same<tensor_scalar_mul>(expr)) {
    auto const &sm = expr.template get<tensor_scalar_mul>();
    return sm.expr_lhs() * sym(sm.expr_rhs());
  }
  if (is_same<tensor_negative>(expr)) {
    auto const &neg = expr.template get<tensor_negative>();
    return -sym(neg.expr());
  }
  if (is_same<tensor_zero>(expr))
    return make_expression<tensor_zero>(expr.get().dim(), expr.get().rank());
  auto d = expr.get().dim();
  auto result = inner_product(P_sym(d), sequence{3, 4},
                              std::forward<Expr>(expr), sequence{1, 2});
  result.data()->set_space(space_for_kind(ProjKind::Sym));
  return result;
}

template <tensor_expr_holder Expr>
[[nodiscard]] inline expression_holder<tensor_expression> vol(Expr &&expr) {
  if (expr.get().rank() != 2)
    throw invalid_expression_error(
        "vol: only rank-2 tensors are supported (got rank " +
        std::to_string(expr.get().rank()) + ")");
  if (is_same<identity_tensor>(expr) && expr.get().rank() == 2)
    return expr;
  if (auto const &sp = expr.get().space()) {
    if (auto rule = contraction_rule(ProjKind::Vol, classify_space(*sp))) {
      if (*rule == ContractionRule::Idempotent ||
          *rule == ContractionRule::RhsSubspace)
        return expr;
      if (*rule == ContractionRule::Zero)
        return make_expression<tensor_zero>(expr.get().dim(),
                                            expr.get().rank());
    }
  }
  if (auto simp = try_simplify_projector_contraction(ProjKind::Vol, expr)) {
    if (simp->rule == ContractionRule::Zero)
      return make_expression<tensor_zero>(simp->dim, expr.get().rank());
    return apply_projection(simp->result_kind, simp->argument);
  }
  if (is_same<tensor_scalar_mul>(expr)) {
    auto const &sm = expr.template get<tensor_scalar_mul>();
    return sm.expr_lhs() * vol(sm.expr_rhs());
  }
  if (is_same<tensor_negative>(expr)) {
    auto const &neg = expr.template get<tensor_negative>();
    return -vol(neg.expr());
  }
  if (is_same<tensor_zero>(expr))
    return make_expression<tensor_zero>(expr.get().dim(), expr.get().rank());
  auto d = expr.get().dim();
  auto result = inner_product(P_vol(d), sequence{3, 4},
                              std::forward<Expr>(expr), sequence{1, 2});
  result.data()->set_space(space_for_kind(ProjKind::Vol));
  return result;
}

template <tensor_expr_holder Expr>
[[nodiscard]] inline expression_holder<tensor_expression> skew(Expr &&expr) {
  if (expr.get().rank() != 2)
    throw invalid_expression_error(
        "skew: only rank-2 tensors are supported (got rank " +
        std::to_string(expr.get().rank()) + ")");
  if (is_same<identity_tensor>(expr) && expr.get().rank() == 2)
    return make_expression<tensor_zero>(expr.get().dim(), expr.get().rank());
  if (auto const &sp = expr.get().space()) {
    if (auto rule = contraction_rule(ProjKind::Skew, classify_space(*sp))) {
      if (*rule == ContractionRule::Idempotent ||
          *rule == ContractionRule::RhsSubspace)
        return expr;
      if (*rule == ContractionRule::Zero)
        return make_expression<tensor_zero>(expr.get().dim(),
                                            expr.get().rank());
    }
  }
  if (auto simp = try_simplify_projector_contraction(ProjKind::Skew, expr)) {
    if (simp->rule == ContractionRule::Zero)
      return make_expression<tensor_zero>(simp->dim, expr.get().rank());
    return apply_projection(simp->result_kind, simp->argument);
  }
  if (is_same<tensor_scalar_mul>(expr)) {
    auto const &sm = expr.template get<tensor_scalar_mul>();
    return sm.expr_lhs() * skew(sm.expr_rhs());
  }
  if (is_same<tensor_negative>(expr)) {
    auto const &neg = expr.template get<tensor_negative>();
    return -skew(neg.expr());
  }
  if (is_same<tensor_zero>(expr))
    return make_expression<tensor_zero>(expr.get().dim(), expr.get().rank());
  auto d = expr.get().dim();
  auto result = inner_product(P_skew(d), sequence{3, 4},
                              std::forward<Expr>(expr), sequence{1, 2});
  result.data()->set_space(space_for_kind(ProjKind::Skew));
  return result;
}

namespace detail_ip {
/// If RHS is a rank-2 projector and both index sets are {1,2},
/// normalize X:{1,2} P:{1,2} → sym/dev/vol/skew(X).
template <tensor_expr_holder ExprLHS, tensor_expr_holder ExprRHS>
inline std::optional<expression_holder<tensor_expression>>
try_normalize_reversed_projector(ExprLHS &&lhs, sequence const &lhs_indices,
                                 ExprRHS &&rhs, sequence const &rhs_indices) {
  if (!is_same<tensor_projector>(rhs))
    return std::nullopt;
  if (lhs.get().rank() != 2)
    return std::nullopt;
  auto const &proj = rhs.template get<tensor_projector>();
  if (proj.acts_on_rank() != 2)
    return std::nullopt;
  if (lhs_indices != sequence{1, 2} || rhs_indices != sequence{1, 2})
    return std::nullopt;
  auto kind = classify(proj);
  switch (kind) {
  case ProjKind::Sym:
    return sym(std::forward<ExprLHS>(lhs));
  case ProjKind::Skew:
    return skew(std::forward<ExprLHS>(lhs));
  case ProjKind::Vol:
    return vol(std::forward<ExprLHS>(lhs));
  case ProjKind::Dev:
    return dev(std::forward<ExprLHS>(lhs));
  default:
    return std::nullopt;
  }
}
} // namespace detail_ip

template <tensor_expr_holder ExprLHS, tensor_expr_holder ExprRHS>
[[nodiscard]] inline auto inner_product(ExprLHS &&lhs, sequence &&lhs_indices,
                                        ExprRHS &&rhs, sequence &&rhs_indices) {
  if (is_same<tensor_zero>(lhs) || is_same<tensor_zero>(rhs)) {
    const auto rank{lhs.get().rank() + rhs.get().rank() - lhs_indices.size() -
                    rhs_indices.size()};
    return make_expression<tensor_zero>(lhs.get().dim(), rank);
  }

  if (auto norm = detail_ip::try_normalize_reversed_projector(lhs, lhs_indices,
                                                              rhs, rhs_indices))
    return std::move(*norm);
  return make_expression<inner_product_wrapper>(
      std::forward<ExprLHS>(lhs), std::move(lhs_indices),
      std::forward<ExprRHS>(rhs), std::move(rhs_indices));
}

template <tensor_expr_holder ExprLHS, tensor_expr_holder ExprRHS>
[[nodiscard]] inline auto
inner_product(ExprLHS &&lhs, sequence const &lhs_indices, ExprRHS &&rhs,
              sequence const &rhs_indices) {

  if (is_same<tensor_zero>(lhs) || is_same<tensor_zero>(rhs)) {
    const auto rank{lhs.get().rank() + rhs.get().rank() - lhs_indices.size() -
                    rhs_indices.size()};
    return make_expression<tensor_zero>(lhs.get().dim(), rank);
  }

  if (auto norm = detail_ip::try_normalize_reversed_projector(lhs, lhs_indices,
                                                              rhs, rhs_indices))
    return std::move(*norm);
  return make_expression<inner_product_wrapper>(
      std::forward<ExprLHS>(lhs), lhs_indices, std::forward<ExprRHS>(rhs),
      rhs_indices);
}

template <tensor_expr_holder ExprLHS, tensor_expr_holder ExprRHS>
[[nodiscard]] constexpr inline auto otimes(ExprLHS &&lhs, ExprRHS &&rhs) {
  if (is_same<tensor_zero>(lhs) || is_same<tensor_zero>(rhs))
    return make_expression<tensor_zero>(lhs.get().dim(),
                                        lhs.get().rank() + rhs.get().rank());
  sequence lhs_indices(lhs.get().rank()), rhs_indices(rhs.get().rank());
  std::iota(std::begin(lhs_indices), std::end(lhs_indices), std::size_t{0});
  std::iota(std::begin(rhs_indices), std::end(rhs_indices), lhs_indices.size());
  return make_expression<outer_product_wrapper>(
      std::forward<ExprLHS>(lhs), std::move(lhs_indices),
      std::forward<ExprRHS>(rhs), std::move(rhs_indices));
}

template <tensor_expr_holder ExprLHS, tensor_expr_holder ExprRHS>
[[nodiscard]] constexpr inline auto
otimes(ExprLHS &&lhs, sequence &&lhs_indices, ExprRHS &&rhs,
       sequence &&rhs_indices) {
  if (is_same<tensor_zero>(lhs) || is_same<tensor_zero>(rhs))
    return make_expression<tensor_zero>(lhs.get().dim(),
                                        lhs.get().rank() + rhs.get().rank());
  return make_expression<outer_product_wrapper>(
      std::forward<ExprLHS>(lhs), std::move(lhs_indices),
      std::forward<ExprRHS>(rhs), std::move(rhs_indices));
}

template <tensor_expr_holder ExprLHS, tensor_expr_holder ExprRHS>
[[nodiscard]] constexpr inline auto otimesu(ExprLHS &&lhs, ExprRHS &&rhs) {
  if (is_same<tensor_zero>(lhs) || is_same<tensor_zero>(rhs))
    return make_expression<tensor_zero>(lhs.get().dim(),
                                        lhs.get().rank() + rhs.get().rank());
  return make_expression<outer_product_wrapper>(
      std::forward<ExprLHS>(lhs), std::move(sequence{1, 3}),
      std::forward<ExprRHS>(rhs), std::move(sequence{2, 4}));
}

template <tensor_expr_holder ExprLHS, tensor_expr_holder ExprRHS>
[[nodiscard]] constexpr inline auto otimesl(ExprLHS &&lhs, ExprRHS &&rhs) {
  if (is_same<tensor_zero>(lhs) || is_same<tensor_zero>(rhs))
    return make_expression<tensor_zero>(lhs.get().dim(),
                                        lhs.get().rank() + rhs.get().rank());
  return make_expression<outer_product_wrapper>(
      std::forward<ExprLHS>(lhs), std::move(sequence{1, 4}),
      std::forward<ExprRHS>(rhs), std::move(sequence{2, 3}));
}

template <tensor_expr_holder Expr>
[[nodiscard]] constexpr inline auto permute_indices(Expr &&expr,
                                                    sequence &&indices) {
  // Size gate: the permutation must cover exactly the tensor's index
  // positions. A mismatch would either drop or invent indices in the
  // resulting expression — silently wrong.
  if (indices.size() != expr.get().rank())
    throw invalid_expression_error(
        "permute_indices: indices size (" + std::to_string(indices.size()) +
        ") must equal tensor rank (" + std::to_string(expr.get().rank()) + ")");
  // For symmetric rank-2 tensors, any permutation of two indices is identity
  if (expr.get().rank() == 2) {
    if (auto const &sp = expr.get().space()) {
      if (std::holds_alternative<Symmetric>(sp->perm))
        return std::forward<Expr>(expr);
    }
  }
  if (is_same<tensor_zero>(expr))
    return make_expression<tensor_zero>(expr.get().dim(), expr.get().rank());
  if (is_same<permute_indices_wrapper>(expr)) {
    auto &tensor{expr.template get<permute_indices_wrapper>()};
    const auto &t_indices{tensor.indices()};
    sequence new_order(t_indices.size());
    for (std::size_t i{0}; i < t_indices.size(); ++i) {
      new_order[i] = indices[t_indices[i]];
    }
    return make_expression<permute_indices_wrapper>(tensor.expr(),
                                                    std::move(new_order));
  }

  if (is_same<outer_product_wrapper>(expr)) {
    auto &tensor{expr.template get<outer_product_wrapper>()};
    const auto &indices_lhs{tensor.indices_lhs()};
    const auto &indices_rhs{tensor.indices_rhs()};
    sequence indices_old;
    indices_old.reserve(indices_lhs.size() + indices_rhs.size());
    indices_old.insert(indices_old.end(), indices_lhs.begin(),
                       indices_lhs.end());
    indices_old.insert(indices_old.end(), indices_rhs.begin(),
                       indices_rhs.end());
    sequence indices_new(indices_lhs.size() + indices_rhs.size());
    sequence indices_new_lhs, indices_new_rhs;
    indices_new_lhs.reserve(indices_lhs.size());
    indices_new_rhs.reserve(indices_rhs.size());

    // Permute: new[k] = perm[old[k]]  (all 0-based)
    // Same composition as permute_indices_wrapper folding above.
    for (std::size_t k{0}; k < indices_old.size(); ++k) {
      indices_new[k] = indices[indices_old[k]];
    }

    indices_new_lhs.insert(indices_new_lhs.begin(), indices_new.begin(),
                           indices_new.begin() + indices_lhs.size());
    indices_new_rhs.insert(indices_new_rhs.begin(),
                           indices_new.begin() + indices_lhs.size(),
                           indices_new.end());
    return otimes(tensor.expr_lhs(), std::move(indices_new_lhs),
                  tensor.expr_rhs(), std::move(indices_new_rhs));
  }

  // outer_product_wrapper
  return make_expression<permute_indices_wrapper>(std::forward<Expr>(expr),
                                                  std::move(indices));
}

template <tensor_expr_holder Expr>
[[nodiscard]] constexpr inline auto trans(Expr &&expr) {
  // trans is rank-2 only (it's the {2,1} permutation). The internal
  // permute_indices_wrapper this builds also assumes rank 2.
  if (expr.get().rank() != 2)
    throw invalid_expression_error(
        "trans: only rank-2 tensors are supported (got rank " +
        std::to_string(expr.get().rank()) + ")");
  // trans(X) = X when X is symmetric (or any symmetric subspace)
  // trans(X) = -X when X is skew-symmetric
  if (auto const &sp = expr.get().space()) {
    if (std::holds_alternative<Symmetric>(sp->perm))
      return std::forward<Expr>(expr);
    if (std::holds_alternative<Skew>(sp->perm))
      return -std::forward<Expr>(expr);
  }
  if (is_same<tensor_zero>(expr))
    return make_expression<tensor_zero>(expr.get().dim(), expr.get().rank());
  if (is_same<permute_indices_wrapper>(expr)) {
    auto const &bc = expr.template get<permute_indices_wrapper>();
    if (bc.indices() == sequence{2, 1})
      return bc.expr();
  }
  return make_expression<permute_indices_wrapper>(std::forward<Expr>(expr),
                                                  sequence{2, 1});
}

template <tensor_expr_holder Expr>
[[nodiscard]] constexpr inline auto inv(Expr &&expr) {
  // inv(inv(A)) → A — collapse at any rank. By the time we see a
  // tensor_inv the inner is already valid (would have been rejected at
  // its own construction by the guards below if not).
  if (is_same<tensor_inv>(expr))
    return expr.template get<tensor_inv>().expr();
  // identity_tensor is self-inverse at any even rank (the minor identity
  // is its own inverse under the appropriate contraction). Short-circuit
  // *before* the rank check below so the rank-4 minor identity stays
  // valid. (The former kronecker_delta short-circuit is gone — that node
  // was unified into identity_tensor in #188.)
  if (is_same<identity_tensor>(expr))
    return std::forward<Expr>(expr);
  // Singular: inv(0) is undefined. Closes #187.
  if (is_same<tensor_zero>(expr))
    throw invalid_expression_error("inv: operand is the zero tensor "
                                   "(singular)");
  // Rank gate (#248): rank-2 is unconditionally supported; rank-4 is
  // supported with two evaluator routes selected by the operand's space
  // annotation:
  //
  //   * Minor / MinorMajor → tmech::inv<sequence<1,2>, sequence<3,4>>
  //       The minor-symmetric inverse — the dominant case in continuum
  //       mechanics (algorithmic-tangent rule C_ijkl = ∂a_ij/∂b_kl):
  //         (A^{-1})_ijmn · A_mnkl = 1/2 (I_ik I_jl + I_il I_jk)
  //
  //   * Any other annotation (Skew, Sym_r, Anti_r, ...) OR no annotation
  //     → tmech::invf (fully anisotropic):
  //         (A^{-1})_ijmn · A_mnkl = I_ij · I_kl
  //
  // The dispatch lives in the evaluator (operator()(tensor_inv) inspects
  // the operand's space). The factory just gates rank; the AST node
  // remains a single tensor_inv either way.
  //
  // Other ranks (3, 5, ...) keep the rejection — they have no canonical
  // inverse routine. Supersedes #192's rank-2-only gate.
  if (expr.get().rank() != 2 && expr.get().rank() != 4)
    throw invalid_expression_error(
        "inv: only rank-2 and rank-4 tensors are supported (got rank " +
        std::to_string(expr.get().rank()) + ")");
  // inv(orthogonal R) = trans(R). Closes one half of #246. The
  // orthogonality annotation only makes sense at rank-2 (R^T R = I for
  // square R) and trans() is rank-2 only — gate on both. Re-annotate
  // the result as orthogonal so downstream queries / further folds see
  // it (trans() doesn't propagate the algebra annotation through its
  // general path). Same post-construction annotation pattern as
  // tensor_operators.h's `trans(A) + (-A) → Skew` recognition.
  if (expr.get().rank() == 2 && is_orthogonal(expr)) {
    auto result = trans(std::forward<Expr>(expr));
    result.data()->tensor_algebra_assumptions().insert(orthogonal{});
    return result;
  }
  // inv(α·A) = inv(A) / α. The scalar pulls out of the inverse as its
  // reciprocal. Recurse so e.g. inv(α·inv(A)) folds via tensor_inv → A,
  // not just one layer. (Placed after the rank gate so rank > 2 inputs
  // produce the dedicated "rank not supported" error rather than
  // recursing into a deeper inv call.) Closes #71.
  if (is_same<tensor_scalar_mul>(expr)) {
    auto const &sm = expr.template get<tensor_scalar_mul>();
    return inv(sm.expr_rhs()) / sm.expr_lhs();
  }
  // A skew-symmetric matrix in odd dimensions is singular (det = 0) by
  // the determinant theorem det(-A^T) = (-1)^n det(A). The theorem is
  // RANK-2 SPECIFIC — for rank-4 a "skew" annotation doesn't carry the
  // same algebraic consequence (rank-4 skew has different geometry and
  // the 9x9 unfolding's determinant isn't related to (-1)^n det(A)).
  // Restrict the guard to rank-2 so rank-4 skew passes through to the
  // invf evaluator branch. contains_skew_factor catches both direct
  // skew tensors and products like B * skew(A).
  if (expr.get().rank() == 2 && expr.get().dim() % 2 != 0 &&
      contains_skew_factor(expr)) {
    throw invalid_expression_error(
        "inv: operand contains a skew-symmetric factor in odd dimensions "
        "(singular)");
  }
  return make_expression<tensor_inv>(std::forward<Expr>(expr));
}

} // namespace numsim::cas

#endif // NUMSIM_CAS_TENSOR_FUNCTIONS_H
