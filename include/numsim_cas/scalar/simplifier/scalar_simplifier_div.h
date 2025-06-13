#ifndef SCALAR_SIMPLIFIER_DIV_H
#define SCALAR_SIMPLIFIER_DIV_H

#include "../../operators.h"
#include "../scalar_div.h"
#include "../scalar_expression.h"
#include "../scalar_std.h"
#include <type_traits>

namespace numsim::cas {
namespace scalar_detail {
namespace simplifier {

template <typename ExprLHS, typename ExprRHS> struct div_default {
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;

  div_default(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  template <typename Expr> constexpr inline expr_type operator()(Expr const &) {
    return get_default();
  }

  // expr / 1 --> expr
  constexpr inline expr_type operator()(scalar_one<value_type> const &) {
    return std::forward<ExprLHS>(m_lhs);
  }

  constexpr inline expr_type
  operator()(scalar_pow<value_type> const &rhs) {
    if(m_lhs.get().hash_value() == rhs.expr_lhs().get().hash_value()){
      return make_expression<scalar_pow<value_type>>(rhs.expr_lhs(), get_scalar_one<value_type>() - rhs.expr_rhs());
    }
    return get_default();
  }

protected:

  auto get_default() {
    if(m_lhs.get().hash_value() == m_rhs.get().hash_value()){
      return get_scalar_one<value_type>();
    }
    return make_expression<scalar_div<value_type>>(m_lhs, m_rhs);
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

template <typename ExprLHS, typename ExprRHS>
struct scalar_div_simplifier final : public div_default<ExprLHS, ExprRHS> {
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;
  using base = div_default<ExprLHS, ExprRHS>;
  using base::operator();

  scalar_div_simplifier(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        m_expr(m_lhs.template get<scalar_div<value_type>>()) {}

  // (a/b)/(c/d) --> a*d/(b*c)
  constexpr inline expr_type operator()(scalar_div<value_type> const &rhs) {
    return make_expression<scalar_div<value_type>>(
        m_expr.expr_lhs() * rhs.expr_rhs(), m_expr.expr_rhs() * rhs.expr_lhs());
  }

  // a/b/c --> a/(b*c)
  template <typename Expr> constexpr inline expr_type operator()(Expr const &) {
    return make_expression<scalar_div<value_type>>(m_expr.expr_lhs(),
                                                   m_expr.expr_rhs() * m_rhs);
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_div<value_type> const &m_expr;
};

template <typename ExprLHS, typename ExprRHS>
struct constant_div_simplifier final : public div_default<ExprLHS, ExprRHS> {
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;
  using base = div_default<ExprLHS, ExprRHS>;
  using base::operator();

  constant_div_simplifier(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        m_expr(m_lhs.template get<scalar_constant<value_type>>()) {}

  constexpr inline expr_type
  operator()(scalar_constant<value_type> const &rhs) {
    return make_expression<scalar_constant<value_type>>(m_expr() / rhs());
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_constant<value_type> const &m_expr;
};

template <typename ExprLHS, typename ExprRHS>
struct pow_div_simplifier final : public div_default<ExprLHS, ExprRHS> {
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;
  using base = div_default<ExprLHS, ExprRHS>;
  using base::operator();

  pow_div_simplifier(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        m_expr(m_lhs.template get<scalar_pow<value_type>>()) {}


  constexpr inline expr_type
  operator()(scalar<value_type> const &rhs) {
    if(rhs.hash_value() == m_expr.expr_lhs().get().hash_value()){
      const auto expo{m_expr.expr_rhs() - get_scalar_one<value_type>()};
      if(is_same<scalar_constant<value_type>>(expo)){
        if(expo.template get<scalar_constant<value_type>>()() == 1){
          return m_rhs;
        }
      }
      return make_expression<scalar_pow<value_type>>(m_expr.expr_lhs(), expo);
    }
    return base::get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_pow<value_type> const &m_expr;
};

template <typename ExprLHS, typename ExprRHS> struct div_base {
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;

  div_base(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  template <typename Type> constexpr inline expr_type operator()(Type const &) {
    auto &expr_rhs{*m_rhs};
    return visit(div_default<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                               std::forward<ExprRHS>(m_rhs)),
                 expr_rhs);
  }

  constexpr inline expr_type operator()(scalar_div<value_type> const &) {
    auto &expr_rhs{*m_rhs};
    return visit(
        scalar_div_simplifier<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                                std::forward<ExprRHS>(m_rhs)),
        expr_rhs);
  }

  constexpr inline expr_type operator()(scalar_pow<value_type> const &) {
    auto &expr_rhs{*m_rhs};
    return visit(
        pow_div_simplifier<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                                std::forward<ExprRHS>(m_rhs)),
        expr_rhs);
  }

  template <typename ValueType = value_type,
            std::enable_if_t<std::is_floating_point_v<ValueType>, bool> = true>
  constexpr inline expr_type operator()(scalar_constant<value_type> const &) {
    auto &expr_rhs{*m_rhs};
    return visit(
        constant_div_simplifier<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                                  std::forward<ExprRHS>(m_rhs)),
        expr_rhs);
  }

private:
  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

} // namespace simplifier
} // namespace scalar_detail
} // namespace numsim::cas

#endif // SCALAR_SIMPLIFIER_DIV_H
