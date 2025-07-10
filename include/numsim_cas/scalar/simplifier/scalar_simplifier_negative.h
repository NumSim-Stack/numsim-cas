#ifndef SCALAR_SIMPLIFIER_NEGATIVE_H
#define SCALAR_SIMPLIFIER_NEGATIVE_H

#include "../../numsim_cas_type_traits.h"
#include <utility>

namespace numsim::cas {
namespace simplifier {

template <typename Expr> class scalar_simplifier_negative {
public:
  scalar_simplifier_negative(Expr &&expr) : m_expr(std::forward<Expr>(expr)) {}

  template <typename NegExpr> auto operator()(NegExpr const &) {
    using value_type = typename NegExpr::value_type;
    return make_expression<scalar_negative<value_type>>(
        std::forward<Expr>(m_expr));
  }

  template <typename ValueType>
  auto operator()(scalar_negative<ValueType> const &expr) {
    return expr.expr();
  }

  template <typename ValueType>
  auto operator()(scalar_zero<ValueType> const &) {
    return std::forward<Expr>(m_expr);
  }

private:
  Expr &&m_expr;
};

} // namespace simplifier
} // namespace numsim::cas

#endif // SCALAR_SIMPLIFIER_NEGATIVE_H
