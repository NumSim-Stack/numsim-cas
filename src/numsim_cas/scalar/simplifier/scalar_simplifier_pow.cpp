#include <numsim_cas/core/operators.h>
#include <numsim_cas/scalar/scalar_definitions.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/simplifier/scalar_simplifier_pow.h>

namespace numsim::cas::simplifier {

pow_pow::pow_pow(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      m_lhs_node{base::m_lhs.template get<scalar_pow>()} {}

/// pow(pow(x,a),b) --> pow(x,a*b)
/// TODO? pow(pow(x,-a), -b) --> pow(x,-a*b) only when x,a,b>0
template <typename Expr>
pow_pow::expr_holder_t pow_pow::dispatch(Expr const &) {
  return pow(m_lhs_node.expr_lhs(), m_lhs_node.expr_rhs() * m_rhs);
}

pow_pow::expr_holder_t pow_pow::dispatch(scalar_negative const &) {
  return pow(m_lhs_node.expr_lhs(), m_lhs_node.expr_rhs() * m_rhs);
}

mul_pow::mul_pow(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      m_lhs_node{base::m_lhs.template get<scalar_mul>()} {}

// pow(scalar_mul, -rhs)
mul_pow::expr_holder_t mul_pow::dispatch(scalar_negative const &rhs) {
  auto pos{m_lhs_node.hash_map().find(rhs.expr())};
  auto mul_expr{make_expression<scalar_mul>(m_lhs_node)};
  auto &mul{mul_expr.template get<scalar_mul>()};
  // x*y*z / x --> y*z
  if (pos != m_lhs_node.hash_map().end()) {
    mul.hash_map().erase(rhs.expr());
    return mul_expr;
  }

  // pow(x*y*pow(z,base), rhs) --> pow(x*y, rhs) * pos(z,base*rhs)
  const auto pows{get_all<scalar_pow>(m_lhs_node)};
  if (!pows.empty()) {
    expr_holder_t result;
    for (const auto &expr : pows) {
      const auto &pow_expr{expr.get<scalar_pow>()};
      mul.hash_map().erase(expr);
      auto pow_n{pow(pow_expr.expr_lhs(), pow_expr.expr_rhs() * m_rhs)};
      if (!result.is_valid()) {
        result = std::move(pow_n);
      } else {
        result = result * std::move(pow_n);
      }
    }
    return pow(mul_expr, m_rhs) * result;
  }

  return get_default();
}

template <typename Expr>
mul_pow::expr_holder_t mul_pow::dispatch([[maybe_unused]] Expr const &rhs) {
  auto mul_expr{make_expression<scalar_mul>(m_lhs_node)};
  auto &mul{mul_expr.template get<scalar_mul>()};

  // pow(x*y*pow(z,base), rhs) --> pow(x*y, rhs) * pos(z,base*rhs)
  const auto pows{get_all<scalar_pow>(m_lhs_node)};
  if (!pows.empty()) {
    expr_holder_t result;
    for (const auto &expr : pows) {
      const auto &pow_expr{expr.get<scalar_pow>()};
      mul.hash_map().erase(expr);
      auto pow_n{pow(pow_expr.expr_lhs(), pow_expr.expr_rhs() * m_rhs)};
      if (!result.is_valid()) {
        result = std::move(pow_n);
      } else {
        result = result * std::move(pow_n);
      }
    }
    return pow(mul_expr, m_rhs) * result;
  }

  return get_default();
}

pow_base::pow_base(expr_holder_t lhs, expr_holder_t rhs)
    : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

pow_base::expr_holder_t pow_base::dispatch(scalar_pow const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  pow_pow visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

pow_base::expr_holder_t pow_base::dispatch(scalar_mul const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  mul_pow visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

} // namespace numsim::cas::simplifier
