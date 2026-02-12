#ifndef TENSOR_SCALAR_SIMPLIFIER_SUB_H
#define TENSOR_SCALAR_SIMPLIFIER_SUB_H

#include "../operators/tensor_to_scalar/tensor_to_scalar_sub.h"
#include "../tensor_to_scalar_expression.h"

namespace numsim::cas {
namespace tensor_to_scalar_detail {
namespace simplifier {

template <typename ExprLHS, typename ExprRHS>
struct sub_default : public tensor_to_scalar_visitor_return_expr_t {
  using expr_type = expression_holder<tensor_to_scalar_expression>;

  sub_default(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  auto get_default() {
    auto add_expr{make_expression<tensor_to_scalar_add>()};
    auto &add{add_expr.template get<tensor_to_scalar_add>()};
    add.push_back(m_lhs);
    add.push_back(-m_rhs);
    return std::move(add_expr);
  }

  template <typename Expr> constexpr inline expr_type operator()(Expr const &) {
    return get_default();
  }

  ExprLHS m_lhs;
  ExprRHS m_rhs;
};

template <typename ExprLHS, typename ExprRHS> struct sub_base {
  using expr_type = expression_holder<tensor_to_scalar_expression>;

  sub_base(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  template <typename Type> constexpr inline expr_type operator()(Type const &) {
    auto &expr_rhs{*m_rhs};
    return visit(sub_default<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                               std::forward<ExprRHS>(m_rhs)),
                 expr_rhs);
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

} // namespace simplifier
} // namespace tensor_to_scalar_detail
} // namespace numsim::cas

#endif // TENSOR_SCALAR_SIMPLIFIER_SUB_H
