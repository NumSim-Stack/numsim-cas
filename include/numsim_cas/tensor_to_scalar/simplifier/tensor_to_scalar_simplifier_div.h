#ifndef TENSOR_TO_SCALAR_SIMPLIFIER_DIV_H
#define TENSOR_TO_SCALAR_SIMPLIFIER_DIV_H

#include "../operators/tensor_to_scalar/tensor_to_scalar_div.h"
#include "../tensor_to_scalar_expression.h"

namespace numsim::cas {
namespace tensor_to_scalar_detail {
namespace simplifier {

template <typename ExprLHS, typename ExprRHS> struct div_default {
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_to_scalar_expression<value_type>>;

  div_default(ExprLHS lhs, ExprRHS rhs) : m_lhs(lhs), m_rhs(rhs) {}

  auto get_default() {
    auto div_new{
        make_expression<tensor_to_scalar_div<value_type>>(m_lhs, m_rhs)};
    return std::move(div_new);
  }

  template <typename Expr> constexpr inline expr_type operator()(Expr const &) {
    return get_default();
  }

  ExprLHS m_lhs;
  ExprRHS m_rhs;
};

template <typename ExprLHS, typename ExprRHS>
class wrapper_tensor_to_scalar_div_div final
    : public div_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_to_scalar_expression<value_type>>;
  using base = div_default<ExprLHS, ExprRHS>;

  wrapper_tensor_to_scalar_div_div(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        m_data(
            m_lhs
                .template get<tensor_to_scalar_with_scalar_div<value_type>>()) {
  }

  //not needed.
  //see operator overload operator()
  //using base::operator();

  // lhs_tensor_to_scalar_with_scalar_div / rhs_tensor_to_scalar_with_scalar_div
  // (lhs_tensor_to_scalar / lhs_scalar) / (rhs_tensor_to_scalar / rhs_scalar)
  // (lhs_tensor_to_scalar * rhs_scalar) / (lhs_scalar * rhs_tensor_to_scalar)
  constexpr inline expr_type
  operator()(tensor_to_scalar_with_scalar_div<value_type> const &rhs) {
    return (rhs.expr_rhs() / m_data.expr_rhs()) *
           (m_data.expr_lhs() / rhs.expr_lhs());
  }

  // lhs_tensor_to_scalar_with_scalar_div / scalar_with_tensor_to_scalar_div
  // (lhs_tensor_to_scalar / lhs_scalar) / (rhs_scalar / rhs_tensor_to_scalar)
  // lhs_tensor_to_scalar * rhs_tensor_to_scalar / (lhs_scalar*rhs_scalar)
  constexpr inline expr_type
  operator()(scalar_with_tensor_to_scalar_div<value_type> const &rhs) {
    return m_data.expr_lhs() * rhs.expr_rhs() /
           (m_data.expr_rhs() * rhs.expr_lhs());
  }

  // lhs_tensor_to_scalar_with_scalar_div / rhs_tensor_to_scalar
  // (lhs_tensor_to_scalar / scalar) / rhs_tensor_to_scalar
  // (lhs_tensor_to_scalar / (scalar * rhs_tensor_to_scalar)
  constexpr inline expr_type
  operator()(tensor_to_scalar_expression<value_type> const &) {
    return m_data.expr_lhs() /
           (m_data.expr_rhs() * std::forward<ExprRHS>(m_rhs));
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_to_scalar_with_scalar_div<value_type> const &m_data;
};

template <typename ExprLHS, typename ExprRHS>
class wrapper_scalar_div_div final : public div_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_to_scalar_expression<value_type>>;
  using base = div_default<ExprLHS, ExprRHS>;

  wrapper_scalar_div_div(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        m_data(
            m_lhs
                .template get<scalar_with_tensor_to_scalar_div<value_type>>()) {
  }

  //using base::operator();

  // scalar_with_tensor_to_scalar_div / rhs_tensor_to_scalar
  // (scalar / lhs_tensor_to_scalar) / rhs_tensor_to_scalar
  // scalar / (lhs_tensor_to_scalar / rhs_tensor_to_scalar)
  constexpr inline expr_type
  operator()(tensor_to_scalar_expression<value_type> const &) {
    return m_data.expr_lhs() /
           (std::forward<ExprRHS>(m_rhs) * m_data.expr_rhs());
  }

  // scalar_with_tensor_to_scalar_div / scalar_with_tensor_to_scalar_div
  // (lhs_scalar / lhs_tensor_to_scalar) / (rhs_scalar / rhs_tensor_to_scalar)
  // (lhs_scalar * rhs_tensor_to_scalar) / (lhs_tensor_to_scalar * rhs_scalar)
  // (lhs_scalar / rhs_scalar) * (rhs_tensor_to_scalar / lhs_tensor_to_scalar)
  // a/b/(x/y) = a*y/b*x
  constexpr inline expr_type
  operator()(scalar_with_tensor_to_scalar_div<value_type> const &rhs) {
    return (m_data.expr_lhs() / rhs.expr_lhs()) *
           (rhs.expr_rhs() / m_data.expr_rhs());
  }

  // scalar_with_tensor_to_scalar_div / tensor_to_scalar_with_scalar_div
  // (lhs_scalar / lhs_tensor_to_scalar) / (rhs_tensor_to_scalar / rhs_scalar)
  // (lhs_scalar * rhs_scalar * ( lhs_tensor_to_scalar / rhs_tensor_to_scalar)
  constexpr inline expr_type
  operator()(tensor_to_scalar_with_scalar_div<value_type> const &rhs) {
    return (m_data.expr_lhs() * rhs.expr_rhs()) *
           (m_data.expr_rhs() / rhs.expr_lhs());
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_with_tensor_to_scalar_div<value_type> const &m_data;
};

template <typename ExprLHS, typename ExprRHS> struct div_base {
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_to_scalar_expression<value_type>>;

  div_base(ExprLHS lhs, ExprRHS rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  template <typename Type> constexpr inline expr_type operator()(Type const &) {
    auto &expr_rhs{*m_rhs};
    return visit(div_default<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                               std::forward<ExprRHS>(m_rhs)),
                 expr_rhs);
  }

  constexpr inline expr_type
  operator()(tensor_to_scalar_with_scalar_div<value_type> const &) {
    auto &expr_rhs{*m_rhs};
    return visit(
        wrapper_tensor_to_scalar_div_div<ExprLHS, ExprRHS>(
            std::forward<ExprLHS>(m_lhs), std::forward<ExprRHS>(m_rhs)),
        expr_rhs);
  }

  constexpr inline expr_type
  operator()(scalar_with_tensor_to_scalar_div<value_type> const &) {
    auto &expr_rhs{*m_rhs};
    return visit(
        wrapper_scalar_div_div<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                                 std::forward<ExprRHS>(m_rhs)),
        expr_rhs);
  }

  ExprLHS m_lhs;
  ExprRHS m_rhs;
};

} // namespace simplifier
} // namespace tensor_to_scalar_detail
} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_SIMPLIFIER_DIV_H
