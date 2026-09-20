#include <numsim_cas/core/operators.h>
#include <numsim_cas/core/scalar_number.h>
#include <numsim_cas/scalar/scalar_definitions.h>
#include <numsim_cas/scalar/scalar_make_constant.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/scalar/simplifier/scalar_simplifier_pow.h>

namespace numsim::cas::simplifier {

namespace {

/// A provably integer exponent: a numeric integer, or a symbol carrying the
/// integer assumption.
bool provably_integer(expression_holder<scalar_expression> const &e) {
  return detail::numeric_integer_exponent<scalar_traits>(e) || is_integer(e);
}

/// (x^a)^b = x^(a*b) needs integer exponents, unless x is known nonnegative.
bool exponents_compose(expression_holder<scalar_expression> const &base,
                       expression_holder<scalar_expression> const &inner_exp,
                       expression_holder<scalar_expression> const &outer_exp) {
  if (is_nonnegative(base) || is_positive(base))
    return true;
  return provably_integer(inner_exp) && provably_integer(outer_exp);
}

} // namespace

pow_pow::pow_pow(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      m_lhs_node{base::m_lhs.template get<scalar_pow>()} {}

/// pow(pow(x,a),b) --> pow(x,a*b)
/// (The double-negative case pow(pow(x,-a),-b) = pow(x,ab) is covered by
/// the a*b exponent multiply above; closed #268.)
pow_pow::expr_holder_t pow_pow::compose() {
  if (!exponents_compose(m_lhs_node.expr_lhs(), m_lhs_node.expr_rhs(), m_rhs))
    return get_default();
  return pow(m_lhs_node.expr_lhs(), m_lhs_node.expr_rhs() * m_rhs);
}

template <typename Expr>
pow_pow::expr_holder_t pow_pow::dispatch(Expr const &) {
  return compose();
}

pow_pow::expr_holder_t pow_pow::dispatch(scalar_negative const &) {
  return compose();
}

mul_pow::mul_pow(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      m_lhs_node{base::m_lhs.template get<scalar_mul>()} {}

/// Each extracted factor composes its exponent with the outer one, so it
/// needs the same real-domain check as pow(pow(x,a),b).
bool mul_pow::factors_compose(std::vector<expr_holder_t> const &pows) const {
  for (auto const &entry : pows) {
    auto const &p{entry.get<scalar_pow>()};
    if (!exponents_compose(p.expr_lhs(), p.expr_rhs(), m_rhs))
      return false;
  }
  return true;
}

// pow(scalar_mul, -rhs)
mul_pow::expr_holder_t
mul_pow::dispatch([[maybe_unused]] scalar_negative const &rhs) {
  const bool int_exp{try_int_constant(m_rhs).has_value()};
  auto mul_expr{make_expression<scalar_mul>(m_lhs_node)};
  auto &mul{mul_expr.template get<scalar_mul>()};
  // pow(x*y*pow(z,base), rhs) --> pow(x*y, rhs) * pos(z,base*rhs)
  const auto pows{get_all<scalar_pow>(m_lhs_node)};
  if (!pows.empty() && int_exp && factors_compose(pows)) {
    expr_holder_t result;
    for (const auto &expr : pows) {
      const auto &pow_expr{expr.get<scalar_pow>()};
      mul.symbol_map().erase(expr);
      mul.invalidate_hash();
      auto pow_n{pow(pow_expr.expr_lhs(), pow_expr.expr_rhs() * m_rhs)};
      if (!result.is_valid()) {
        result = std::move(pow_n);
      } else {
        result = result * std::move(pow_n);
      }
    }
    if (mul.symbol_map().empty()) {
      return (mul.coeff().is_valid() ? pow(mul.coeff(), m_rhs)
                                     : get_scalar_one()) *
             result;
    }
    if (mul.symbol_map().size() == 1 && !mul.coeff().is_valid()) {
      return pow(mul.symbol_map().begin()->second, m_rhs) * result;
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
  if (!pows.empty() && try_int_constant(m_rhs) && factors_compose(pows)) {
    expr_holder_t result;
    for (const auto &expr : pows) {
      const auto &pow_expr{expr.get<scalar_pow>()};
      mul.symbol_map().erase(expr);
      mul.invalidate_hash();
      auto pow_n{pow(pow_expr.expr_lhs(), pow_expr.expr_rhs() * m_rhs)};
      if (!result.is_valid()) {
        result = std::move(pow_n);
      } else {
        result = result * std::move(pow_n);
      }
    }
    if (mul.symbol_map().empty()) {
      return (mul.coeff().is_valid() ? pow(mul.coeff(), m_rhs)
                                     : get_scalar_one()) *
             result;
    }
    if (mul.symbol_map().size() == 1 && !mul.coeff().is_valid()) {
      return pow(mul.symbol_map().begin()->second, m_rhs) * result;
    }
    return pow(mul_expr, m_rhs) * result;
  }

  return get_default();
}

pow_base::pow_base(expr_holder_t lhs, expr_holder_t rhs)
    : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

pow_base::expr_holder_t pow_base::dispatch(scalar_exp const &) {
  return exp(m_lhs.template get<scalar_exp>().expr() * m_rhs);
}

/// pow(sqrt(x), n) → pow(x, n/2), only where sqrt(x) is real: for x < 0 the
/// left side is undefined while pow(x, n/2) may not be.
pow_base::expr_holder_t pow_base::dispatch(scalar_sqrt const &) {
  auto const &radicand{m_lhs.template get<scalar_sqrt>().expr()};
  if (!is_nonnegative(radicand) && !is_positive(radicand))
    return make_expression<scalar_pow>(std::move(m_lhs), std::move(m_rhs));
  auto half = make_expression<scalar_constant>(scalar_number{1, 2});
  return pow(radicand, m_rhs * half);
}

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
