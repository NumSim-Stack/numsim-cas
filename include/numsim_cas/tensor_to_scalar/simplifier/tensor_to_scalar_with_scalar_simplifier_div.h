#ifndef TENSOR_TO_SCALAR_WITH_SCALAR_SIMPLIFIER_DIV_H
#define TENSOR_TO_SCALAR_WITH_SCALAR_SIMPLIFIER_DIV_H

#include "../operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_div.h"
#include "../tensor_to_scalar_expression.h"

namespace numsim::cas {
namespace tensor_to_scalar_with_scalar_detail {
namespace simplifier {

template <typename ExprLHS, typename ExprRHS> struct div_default {
  using expr_type = expression_holder<tensor_to_scalar_expression>;

  div_default(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  // scalar / tensor_to_scalar --> scalar_with_tensor_to_scalar_div
  constexpr inline expr_type operator()(tensor_to_scalar_expression const &) {
    return make_expression<scalar_with_tensor_to_scalar_div>(
        std::forward<ExprLHS>(m_lhs), std::forward<ExprRHS>(m_rhs));
  }

  // tensor_to_scalar / scalar --> tensor_to_scalar_with_scalar_div
  constexpr inline expr_type operator()(scalar_expression const &) {
    return make_expression<tensor_to_scalar_with_scalar_div>(
        std::forward<ExprLHS>(m_lhs), std::forward<ExprRHS>(m_rhs));
  }

  //  constexpr inline expr_type operator()(scalar_constant const &
  //  rhs) {
  //    if(rhs() == static_cast(1)){
  //      return std::forward<ExprLHS>(m_lhs);
  //    }else{
  //      return make_expression<tensor_to_scalar_with_scalar_div>(
  //          std::forward<ExprLHS>(m_lhs), std::forward<ExprRHS>(m_rhs));
  //    }
  //  }

  constexpr inline expr_type operator()(scalar_one const &) {
    return std::forward<ExprLHS>(m_lhs);
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

// tensor_to_scalar_with_scalar_div / scalar
template <typename ExprLHS, typename ExprRHS>
class wrapper_tensor_to_scalar_div_div final
    : public div_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<tensor_to_scalar_expression>;
  using base = div_default<ExprLHS, ExprRHS>;

  wrapper_tensor_to_scalar_div_div(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        m_data(m_lhs.template get<tensor_to_scalar_with_scalar_div>()) {}

  // tensor_to_scalar_with_scalar_div / rhs_scalar
  // (tensor_to_scalar / lhs_scalar) / rhs_scalar
  // tensor_to_scalar / (rhs_scalar * lhs_scalar)
  constexpr inline expr_type operator()(scalar_expression const &) {
    auto scalar{m_data.expr_rhs() * std::forward<ExprRHS>(m_rhs)};
    if (is_same<scalar_constant>(scalar)) {
      if (scalar.template get<scalar_constant>()() == static_cast(1)) {
        return m_data.expr_lhs();
      }
    }
    return m_data.expr_lhs() / scalar;
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_to_scalar_with_scalar_div const &m_data;
};

// scalar_with_tensor_to_scalar_div / scalar
template <typename ExprLHS, typename ExprRHS>
class wrapper_scalar_div_div final : public div_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<tensor_to_scalar_expression>;
  using base = div_default<ExprLHS, ExprRHS>;

  wrapper_scalar_div_div(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        m_data(m_lhs.template get<scalar_with_tensor_to_scalar_div>()) {}

  // lhs_scalar_with_tensor_to_scalar_div / rhs_scalar
  // (lhs_scalar / tensor_to_scalar) / rhs_scalar
  // (lhs_scalar / rhs_scalar) / tensor_to_scalar
  constexpr inline expr_type operator()(scalar_expression const &) {
    return (m_data.expr_lhs() / std::forward<ExprRHS>(m_rhs)) /
           m_data.expr_rhs();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_with_tensor_to_scalar_div const &m_data;
};

template <typename ExprLHS, typename ExprRHS> struct div_base {
  using expr_type = expression_holder<tensor_to_scalar_expression>;

  div_base(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  template <
      typename Expr,
      std::enable_if_t<!std::is_same_v<tensor_to_scalar_with_scalar_div, Expr>,
                       bool> = true,
      std::enable_if_t<!std::is_same_v<scalar_with_tensor_to_scalar_div, Expr>,
                       bool> = true>
  constexpr inline expr_type operator()(Expr const &) {
    auto &expr_rhs{*m_rhs};
    return visit(div_default<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                               std::forward<ExprRHS>(m_rhs)),
                 expr_rhs);
  }

  constexpr inline expr_type
  operator()(tensor_to_scalar_with_scalar_div const &) {
    auto &expr_rhs{*m_rhs};
    return visit(
        wrapper_tensor_to_scalar_div_div<ExprLHS, ExprRHS>(
            std::forward<ExprLHS>(m_lhs), std::forward<ExprRHS>(m_rhs)),
        expr_rhs);
  }

  constexpr inline expr_type
  operator()(scalar_with_tensor_to_scalar_div const &) {
    auto &expr_rhs{*m_rhs};
    return visit(
        wrapper_scalar_div_div<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                                 std::forward<ExprRHS>(m_rhs)),
        expr_rhs);
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};
} // namespace simplifier
} // namespace tensor_to_scalar_with_scalar_detail
} // namespace numsim::cas
#endif // TENSOR_TO_SCALAR_WITH_SCALAR_SIMPLIFIER_DIV_H
