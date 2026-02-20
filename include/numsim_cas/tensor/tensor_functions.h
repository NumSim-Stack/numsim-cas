#ifndef NUMSIM_CAS_TENSOR_FUNCTIONS_H
#define NUMSIM_CAS_TENSOR_FUNCTIONS_H

#include <cstdlib>
#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/tensor/data/tensor_data_make_imp.h>
#include <numsim_cas/tensor/projector_algebra.h>
#include <numsim_cas/tensor/projection_tensor.h>
#include <numsim_cas/tensor/sequence.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/tensor_zero.h>
#include <numsim_cas/tensor/visitors/tensor_printer.h>
#include <vector>

namespace numsim::cas {

template <typename T>
constexpr inline auto make_tensor_data(std::size_t dim, std::size_t rank) {
  return make_tensor_data_imp<T>().evaluate(dim, rank);
}

template <typename Expr>
[[nodiscard]] inline expression_holder<tensor_expression> dev(Expr &&expr) {
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
  return inner_product(P_devi(d), sequence{3, 4},
                       std::forward<Expr>(expr), sequence{1, 2});
}

template <typename Expr>
[[nodiscard]] inline expression_holder<tensor_expression> sym(Expr &&expr) {
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
  return inner_product(P_sym(d), sequence{3, 4},
                       std::forward<Expr>(expr), sequence{1, 2});
}

template <typename Expr>
[[nodiscard]] inline expression_holder<tensor_expression> vol(Expr &&expr) {
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
  return inner_product(P_vol(d), sequence{3, 4},
                       std::forward<Expr>(expr), sequence{1, 2});
}

template <typename Expr>
[[nodiscard]] inline expression_holder<tensor_expression> skew(Expr &&expr) {
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
  return inner_product(P_skew(d), sequence{3, 4},
                       std::forward<Expr>(expr), sequence{1, 2});
}

template <typename ExprLHS, typename ExprRHS>
[[nodiscard]] constexpr inline auto inner_product(ExprLHS &&lhs, sequence &&lhs_indices,
                                    ExprRHS &&rhs, sequence &&rhs_indices) {
  return make_expression<inner_product_wrapper>(
      std::forward<ExprLHS>(lhs), std::move(lhs_indices),
      std::forward<ExprRHS>(rhs), std::move(rhs_indices));
}

template <typename ExprLHS, typename ExprRHS>
[[nodiscard]] constexpr inline auto inner_product(ExprLHS &&lhs, sequence const &lhs_indices,
                                    ExprRHS &&rhs,
                                    sequence const &rhs_indices) {
  return make_expression<inner_product_wrapper>(
      std::forward<ExprLHS>(lhs), lhs_indices, std::forward<ExprRHS>(rhs),
      rhs_indices);
}

template <typename ExprLHS, typename ExprRHS>
[[nodiscard]] constexpr inline auto otimes(ExprLHS &&lhs, ExprRHS &&rhs) {
  sequence lhs_indices(lhs.get().rank()), rhs_indices(rhs.get().rank());
  std::iota(std::begin(lhs_indices), std::end(lhs_indices),
            std::size_t{0});
  std::iota(std::begin(rhs_indices), std::end(rhs_indices),
            lhs_indices.size());
  return make_expression<outer_product_wrapper>(
      std::forward<ExprLHS>(lhs), std::move(lhs_indices),
      std::forward<ExprRHS>(rhs), std::move(rhs_indices));
}

template <typename ExprLHS, typename ExprRHS>
[[nodiscard]] constexpr inline auto otimes(ExprLHS &&lhs, sequence &&lhs_indices,
                             ExprRHS &&rhs, sequence &&rhs_indices) {
  return make_expression<outer_product_wrapper>(
      std::forward<ExprLHS>(lhs), std::move(lhs_indices),
      std::forward<ExprRHS>(rhs), std::move(rhs_indices));
}

template <typename ExprLHS, typename ExprRHS>
[[nodiscard]] constexpr inline auto otimesu(ExprLHS &&lhs, ExprRHS &&rhs) {
  return make_expression<outer_product_wrapper>(
      std::forward<ExprLHS>(lhs), std::move(sequence{1, 3}),
      std::forward<ExprRHS>(rhs), std::move(sequence{2, 4}));
}

template <typename ExprLHS, typename ExprRHS>
[[nodiscard]] constexpr inline auto otimesl(ExprLHS &&lhs, ExprRHS &&rhs) {
  return make_expression<outer_product_wrapper>(
      std::forward<ExprLHS>(lhs), std::move(sequence{1, 4}),
      std::forward<ExprRHS>(rhs), std::move(sequence{2, 3}));
}

template <typename Expr>
[[nodiscard]] constexpr inline auto permute_indices(Expr &&expr, sequence &&indices) {
  if (is_same<basis_change_imp>(expr)) {
    auto &tensor{expr.template get<basis_change_imp>()};
    const auto &t_indices{tensor.indices()};
    sequence new_order(t_indices.size());
    for (std::size_t i{0}; i < t_indices.size(); ++i) {
      new_order[i] = t_indices[indices[i]];
    }
    return make_expression<basis_change_imp>(tensor.expr(),
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

    // Permute: new[i] = old[perm[i]]  (all 0-based)
    for (std::size_t i{0}; i < indices.size(); ++i) {
      indices_new[i] = indices_old[indices[i]];
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
  return make_expression<basis_change_imp>(std::forward<Expr>(expr),
                                           std::move(indices));
}

template <typename Expr> [[nodiscard]] constexpr inline auto trans(Expr &&expr) {
  return make_expression<basis_change_imp>(std::forward<Expr>(expr),
                                           sequence{2, 1});
}

template <typename Expr> [[nodiscard]] constexpr inline auto inv(Expr &&expr) {
  return make_expression<tensor_inv>(std::forward<Expr>(expr));
}

} // namespace numsim::cas

#endif // NUMSIM_CAS_TENSOR_FUNCTIONS_H
