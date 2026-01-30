#ifndef TENSOR_WITH_SCALAR_SIMPLIFIER_DIV_H
#define TENSOR_WITH_SCALAR_SIMPLIFIER_DIV_H

#include "../../scalar/scalar_expression.h"
#include "../tensor_expression.h"
#include <type_traits>

namespace numsim::cas {
namespace tensor_with_scalar_detail {
namespace simplifier {

template <typename ExprLHS, typename ExprRHS> struct div_default {
  using expr_type = expression_holder<tensor_expression>;

  div_default(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  template <typename Expr> constexpr inline expr_type operator()(Expr const &) {
    return get_default();
  }

  // expr / 1 --> expr
  constexpr inline expr_type operator()(scalar_one const &) {
    return std::forward<ExprLHS>(m_lhs);
  }

  constexpr inline expr_type operator()(scalar_constant const &rhs) {
    if (rhs() == static_cast(1)) {
      return std::forward<ExprLHS>(m_lhs);
    }
    return get_default();
  }

protected:
  auto get_default() {
    auto div_new{make_expression<tensor_scalar_div>(
        std::forward<ExprLHS>(m_lhs), std::forward<ExprRHS>(m_rhs))};
    return std::move(div_new);
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

template <typename ExprLHS, typename ExprRHS>
struct tensor_div_simplifier final : public div_default<ExprLHS, ExprRHS> {
  using expr_type = expression_holder<tensor_expression>;
  using base = div_default<ExprLHS, ExprRHS>;
  using base::operator();

  tensor_div_simplifier(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        m_expr(m_lhs.template get<tensor_scalar_div>()) {}

  // (A/b)/(c/d) --> A*d/(b*c)
  constexpr inline expr_type operator()(tensor_scalar_div const &rhs) {
    return make_expression<tensor_scalar_div>(
        m_expr.expr_lhs() * rhs.expr_rhs(), m_expr.expr_rhs() * rhs.expr_lhs());
  }

  // a/b/c --> a/(b*c)
  template <
      typename Expr,
      std::enable_if_t<std::is_base_of_v<scalar_expression, Expr>, bool> = true>
  constexpr inline expr_type operator()(Expr const &) {
    return make_expression<tensor_scalar_div>(m_expr.expr_lhs(),
                                              m_expr.expr_rhs() * m_rhs);
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_scalar_div const &m_expr;
};

template <typename ExprLHS, typename ExprRHS> struct div_base {
  using expr_type = expression_holder<tensor_expression>;

  div_base(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  // lhs should always be tensor_expression
  template <
      typename Expr,
      std::enable_if_t<std::is_base_of_v<tensor_expression, Expr>, bool> = true>
  constexpr inline expr_type operator()(Expr const &) {
    auto &expr_rhs{*m_rhs};
    return visit(div_default<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                               std::forward<ExprRHS>(m_rhs)),
                 expr_rhs);
  }

  constexpr inline expr_type operator()(tensor_scalar_div const &) {
    auto &expr_rhs{*m_rhs};
    return visit(
        tensor_div_simplifier<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                                std::forward<ExprRHS>(m_rhs)),
        expr_rhs);
  }

private:
  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

} // namespace simplifier
} // namespace tensor_with_scalar_detail
} // namespace numsim::cas

#endif // TENSOR_WITH_SCALAR_SIMPLIFIER_DIV_H
