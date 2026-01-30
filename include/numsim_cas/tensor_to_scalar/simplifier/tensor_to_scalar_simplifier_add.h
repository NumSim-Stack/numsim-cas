#ifndef TENSOR_TO_SCALAR_SIMPLIFIER_ADD_H
#define TENSOR_TO_SCALAR_SIMPLIFIER_ADD_H

#include "../../functions.h"
#include "../operators/tensor_to_scalar/tensor_to_scalar_add.h"
#include "../operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_mul.h"
#include "../tensor_to_scalar_expression.h"

namespace numsim::cas {
namespace tensor_to_scalar_detail {
namespace simplifier {
template <typename ExprLHS, typename ExprRHS> struct add_default {
  using expr_type = expression_holder<tensor_to_scalar_expression>;

  add_default(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  template <typename Expr>
  [[nodiscard]] constexpr inline auto operator()(Expr const &) noexcept {
    return get_default();
  }

  // tensor_to_scalar + tensor_scalar_with_scalar_add -->
  // tensor_scalar_with_scalar_add
  [[nodiscard]] constexpr inline auto
  operator()(tensor_to_scalar_with_scalar_add const &rhs) noexcept {
    return make_expression<tensor_to_scalar_with_scalar_add>(
        rhs.expr_lhs(), rhs.expr_rhs() + m_lhs);
  }

protected:
  [[nodiscard]] constexpr inline auto get_default() noexcept {
    if (m_rhs == m_lhs) {
      return get_default_same();
    }
    return get_default_imp();
  }

  [[nodiscard]] constexpr inline auto get_default_same() noexcept {
    auto coef{make_expression<scalar_constant>(2)};
    return make_expression<tensor_to_scalar_with_scalar_mul>(std::move(coef),
                                                             m_rhs);
  }

  [[nodiscard]] constexpr inline auto get_default_imp() noexcept {
    auto add_new{make_expression<tensor_to_scalar_add>()};
    auto &add{add_new.template get<tensor_to_scalar_add>()};
    add.push_back(m_lhs);
    add.push_back(m_rhs);
    return std::move(add_new);
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

template <typename ExprLHS, typename ExprRHS>
class n_ary_add final : public add_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<tensor_to_scalar_expression>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  // using base::get_coefficient;

  n_ary_add(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<tensor_to_scalar_add>()} {}

  // merge two expression
  auto operator()(tensor_to_scalar_add const &rhs) {
    auto expr{make_expression<tensor_to_scalar_add>()};
    auto &add{expr.template get<tensor_to_scalar_add>()};
    merge_add(lhs, rhs, add);
    return expr;
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_to_scalar_add const &lhs;
};

// tensor_scalar_with_scalar_add + tensor_to_scalar -->
// tensor_to_scalar_with_scalar_add tensor_to_scalar +
// tensor_to_scalar_with_scalar_add --> tensor_to_scalar_with_scalar_add
// tensor_scalar_with_scalar_add + tensor_scalar_with_scalar_add -->
// tensor_to_scalar_with_scalar_add
template <typename ExprLHS, typename ExprRHS>
class wrapper_tensor_to_scalar_add_add final
    : public add_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<tensor_to_scalar_expression>;
  using base = add_default<ExprLHS, ExprRHS>;

  wrapper_tensor_to_scalar_add_add(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<tensor_to_scalar_with_scalar_add>()} {}

  // tensor_scalar_with_scalar_add + tensor_scalar_with_scalar_add -->
  // tensor_scalar_with_scalar_add
  constexpr inline expr_type
  operator()(tensor_to_scalar_with_scalar_add const &rhs) {
    return make_expression<tensor_to_scalar_with_scalar_add>(
        lhs.expr_lhs() + rhs.expr_lhs(), lhs.expr_rhs() + rhs.expr_rhs());
  }

  // tensor_scalar_with_scalar_add + tensor_to_scalar -->
  // tensor_scalar_with_scalar_add
  template <typename Expr> constexpr inline expr_type operator()(Expr const &) {
    return make_expression<tensor_to_scalar_with_scalar_add>(
        lhs.expr_lhs(), m_rhs + lhs.expr_rhs());
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_to_scalar_with_scalar_add const &lhs;
};

template <typename ExprLHS, typename ExprRHS> struct add_base {
  using expr_type = expression_holder<tensor_to_scalar_expression>;

  add_base(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  template <typename Type> constexpr inline expr_type operator()(Type const &) {
    auto &expr_rhs{*m_rhs};
    return visit(add_default<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                               std::forward<ExprRHS>(m_rhs)),
                 expr_rhs);
  }

  constexpr inline expr_type operator()(tensor_to_scalar_add const &) {
    auto &expr_rhs{*m_rhs};
    return visit(n_ary_add<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                             std::forward<ExprRHS>(m_rhs)),
                 expr_rhs);
  }

  // tensor_scalar_with_scalar_add + tensor_scalar -->
  // tensor_scalar_with_scalar_add
  constexpr inline expr_type
  operator()(tensor_to_scalar_with_scalar_add const &) {
    auto &expr_rhs{*m_rhs};
    return visit(
        wrapper_tensor_to_scalar_add_add<ExprLHS, ExprRHS>(
            std::forward<ExprLHS>(m_lhs), std::forward<ExprRHS>(m_rhs)),
        expr_rhs);
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};
} // namespace simplifier
} // namespace tensor_to_scalar_detail
} // namespace numsim::cas

#endif // TENSOR_SCALAR_SIMPLIFIER_ADD_H
