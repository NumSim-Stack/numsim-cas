#include <numsim_cas/scalar/scalar_definitions.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/simplifier/scalar_simplifier_mul.h>

namespace numsim::cas {
namespace simplifier {

constant_mul::constant_mul(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      m_lhs_node{base::m_lhs.template get<scalar_constant>()} {}

constant_mul::expr_holder_t constant_mul::dispatch(scalar_constant const &rhs) {
  const auto value{m_lhs_node.value() * rhs.value()};
  return make_expression<scalar_constant>(value);
}

constant_mul::expr_holder_t
constant_mul::dispatch([[maybe_unused]] scalar_mul const &rhs) {
  if (m_lhs_node.value() == 1) {
    return std::move(m_rhs);
  }
  auto mul_expr{make_expression<scalar_mul>(rhs)};
  auto &mul{mul_expr.template get<scalar_mul>()};
  auto coeff{get_coefficient<scalar_traits>(mul, 1) * m_lhs_node.value()};
  mul.set_coeff(make_expression<scalar_constant>(coeff));
  return mul_expr;
}

n_ary_mul::n_ary_mul(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      m_lhs_node{base::m_lhs.template get<scalar_mul>()} {}

// expr * constant
n_ary_mul::expr_holder_t
n_ary_mul::dispatch([[maybe_unused]] scalar_constant const &rhs) {
  auto mul_expr{make_expression<scalar_mul>(m_lhs_node)};
  auto &mul{mul_expr.template get<scalar_mul>()};
  auto coeff{mul.coeff().is_valid() ? mul.coeff() * m_rhs : m_rhs};
  mul.set_coeff(std::move(coeff));
  return mul_expr;
}

n_ary_mul::expr_holder_t
n_ary_mul::dispatch([[maybe_unused]] scalar const &rhs) {
  /// do a deep copy of data
  auto expr_mul{make_expression<scalar_mul>(m_lhs_node)};
  auto &mul{expr_mul.template get<scalar_mul>()};
  /// check if sub_exp == expr_rhs for sub_exp \in expr_lhs
  auto pos{m_lhs_node.symbol_map().find(m_rhs)};
  if (pos != m_lhs_node.symbol_map().end()) {
    auto expr{pos->second * m_rhs};
    mul.symbol_map().erase(m_rhs);
    mul.push_back(expr);
    return expr_mul;
  }

  const auto pows{get_all<scalar_pow>(m_lhs_node)};
  for (const auto &expr : pows) {
    if (expr.get<scalar_pow>().expr_lhs() == m_rhs) {
      mul.symbol_map().erase(expr);
      expr_mul = std::move(expr_mul) * (expr * m_rhs);
      return expr_mul;
    }
  }

  /// no equal expr or sub_expr
  mul.push_back(m_rhs);
  return expr_mul;
}

n_ary_mul::expr_holder_t
n_ary_mul::dispatch([[maybe_unused]] scalar_pow const &rhs) {
  auto expr_mul{make_expression<scalar_mul>(m_lhs_node)};
  auto &mul{expr_mul.template get<scalar_mul>()};

  const auto &smap{m_lhs_node.symbol_map()};
  if (smap.contains(rhs.expr_lhs())) {
    mul.symbol_map().erase(rhs.expr_lhs());
    expr_mul = std::move(expr_mul) *
               pow(rhs.expr_lhs(), rhs.expr_rhs() + get_scalar_one());
    return expr_mul;
  }

  const auto pows{get_all<scalar_pow>(m_lhs_node)};
  for (const auto &expr : pows) {
    if (auto pow{simplify_scalar_pow_pow_mul(expr.template get<scalar_pow>(),
                                             rhs)}) {
      mul.symbol_map().erase(expr);
      expr_mul = std::move(expr_mul) * std::move(*pow);
      return expr_mul;
    }
  }

  mul.push_back(std::move(m_rhs));
  return expr_mul;
}

// (x*y*z)*(x*y*z)
n_ary_mul::expr_holder_t
n_ary_mul::dispatch([[maybe_unused]] scalar_mul const &rhs) {
  auto expr_mul{make_expression<scalar_mul>(m_lhs_node)};

  if (rhs.coeff().is_valid())
    expr_mul *= rhs.coeff();

  for (const auto &expr : rhs.symbol_map() | std::views::values) {
    expr_mul = expr_mul * expr;
  }
  return expr_mul;
}

// ------------------------------------------------------------
// exp_mul
// ------------------------------------------------------------
exp_mul::exp_mul(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      m_lhs_node{base::m_lhs.template get<scalar_exp>()} {}

// exp(a)*exp(b) --> exp(a+b)
exp_mul::expr_holder_t exp_mul::dispatch(scalar_exp const &rhs) {
  return exp(m_lhs_node.expr() + rhs.expr());
}

// exp(a)*(x*exp(b)*...) --> x*exp(a+b)*...
exp_mul::expr_holder_t exp_mul::dispatch(scalar_mul const &rhs) {
  auto expr_mul{make_expression<scalar_mul>(rhs)};
  auto &mul{expr_mul.template get<scalar_mul>()};

  const auto exps{get_all<scalar_exp>(rhs)};
  for (const auto &e : exps) {
    mul.symbol_map().erase(e);
    expr_mul = std::move(expr_mul) *
               exp(m_lhs_node.expr() + e.template get<scalar_exp>().expr());
    return expr_mul;
  }

  mul.push_back(std::move(m_lhs));
  return expr_mul;
}

// ------------------------------------------------------------
// n_ary_mul::dispatch(scalar_exp)
// ------------------------------------------------------------
n_ary_mul::expr_holder_t
n_ary_mul::dispatch([[maybe_unused]] scalar_exp const &rhs) {
  auto expr_mul{make_expression<scalar_mul>(m_lhs_node)};
  auto &mul{expr_mul.template get<scalar_mul>()};

  const auto exps{get_all<scalar_exp>(m_lhs_node)};
  for (const auto &e : exps) {
    mul.symbol_map().erase(e);
    expr_mul = std::move(expr_mul) *
               exp(e.template get<scalar_exp>().expr() + rhs.expr());
    return expr_mul;
  }

  mul.push_back(m_rhs);
  return expr_mul;
}

scalar_pow_mul::scalar_pow_mul(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      m_lhs_node{base::m_lhs.template get<scalar_pow>()} {}

scalar_pow_mul::expr_holder_t
scalar_pow_mul::dispatch([[maybe_unused]] scalar const &rhs) {
  const auto &power{m_lhs_node.expr_rhs()};
  const auto &pow_base{m_lhs_node.expr_lhs()};
  if (pow_base == m_rhs) {
    const auto rhs_expr{m_lhs_node.expr_rhs() + get_scalar_one()};
    return pow(m_lhs_node.expr_lhs(), rhs_expr);
  }

  // pow(expr, -expr_p) * expr_p --> expr
  if (is_same<scalar_negative>(power) &&
      power.get<scalar_negative>().expr() == m_rhs) {
    return m_lhs_node.expr_lhs();
  }
  return get_default();
}

// pow(expr, base_lhs) * pow(expr, base_rhs) --> pow(expr, base_lhs+base_rhs)
// pow(expr_lhs, base) * pow(expr_rhs, base) --> pow(expr_lhs * expr_rhs,
// base)
scalar_pow_mul::expr_holder_t
scalar_pow_mul::dispatch([[maybe_unused]] scalar_pow const &rhs) {
  if (m_lhs_node.expr_lhs() == rhs.expr_lhs()) {
    const auto rhs_expr{m_lhs_node.expr_rhs() + rhs.expr_rhs()};
    return pow(m_lhs_node.expr_lhs(), rhs_expr);
  }

  if (m_lhs_node.expr_rhs() == rhs.expr_rhs()) {
    const auto lhs_expr{m_lhs_node.expr_lhs() * rhs.expr_lhs()};
    return pow(lhs_expr, m_lhs_node.expr_rhs());
  }

  return get_default();
}

// pow(x,1) * (x*y*z)
scalar_pow_mul::expr_holder_t
scalar_pow_mul::dispatch([[maybe_unused]] scalar_mul const &rhs) {
  auto expr_mul{make_expression<scalar_mul>(rhs)};
  auto &mul{expr_mul.template get<scalar_mul>()};

  const auto &smap{rhs.symbol_map()};
  if (smap.contains(m_lhs_node.expr_lhs())) {
    mul.symbol_map().erase(m_lhs_node.expr_lhs());
    expr_mul =
        std::move(expr_mul) *
        pow(m_lhs_node.expr_lhs(), m_lhs_node.expr_rhs() + get_scalar_one());
    return expr_mul;
  }

  const auto pows{get_all<scalar_pow>(rhs)};
  for (const auto &expr : pows) {
    if (auto pow{simplify_scalar_pow_pow_mul(
            m_lhs_node, expr.template get<scalar_pow>())}) {
      mul.symbol_map().erase(expr);
      expr_mul = std::move(expr_mul) * std::move(*pow);
      return expr_mul;
    }
  }

  mul.push_back(std::move(m_lhs));
  return expr_mul;
}

symbol_mul::symbol_mul(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      m_lhs_node{base::m_lhs.template get<scalar>()} {}

/// x*x --> pow(x,2)
symbol_mul::expr_holder_t symbol_mul::dispatch(scalar const &rhs) {
  if (&m_lhs_node == &rhs) {
    return make_expression<scalar_pow>(std::move(m_rhs),
                                       make_expression<scalar_constant>(2));
  }
  return get_default();
}

symbol_mul::expr_holder_t
symbol_mul::dispatch([[maybe_unused]] scalar_mul const &rhs) {
  /// do a deep copy of data
  auto expr_mul{make_expression<scalar_mul>(rhs)};
  auto &mul{expr_mul.template get<scalar_mul>()};
  /// check if sub_exp == expr_rhs for sub_exp \in expr_lhs
  auto pos{rhs.symbol_map().find(m_lhs)};
  if (pos != rhs.symbol_map().end()) {
    // auto expr{binary_scalar_mul_simplify(pos->second, m_lhs)};
    auto expr{pos->second * m_lhs};
    mul.symbol_map().erase(m_lhs);
    mul.push_back(expr);
    return expr_mul;
  }
  /// no equal expr or sub_expr
  mul.push_back(m_lhs);
  return expr_mul;
}

/// x * pow(x,expr) --> pow(x,expr+1)
symbol_mul::expr_holder_t symbol_mul::dispatch(scalar_pow const &rhs) {
  if (m_lhs == rhs.expr_lhs()) {
    return pow(m_lhs, rhs.expr_rhs() + get_scalar_one());
  }

  return get_default();
}

mul_base::mul_base(expr_holder_t lhs, expr_holder_t rhs)
    : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

mul_base::expr_holder_t mul_base::dispatch(scalar_negative const &lhs) {
  auto expr{lhs.expr() * m_rhs};
  return -expr;
}

mul_base::expr_holder_t mul_base::dispatch(scalar_constant const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  constant_mul visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

mul_base::expr_holder_t mul_base::dispatch(scalar_mul const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  n_ary_mul visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

mul_base::expr_holder_t mul_base::dispatch(scalar const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  symbol_mul visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

mul_base::expr_holder_t mul_base::dispatch(scalar_pow const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  scalar_pow_mul visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

mul_base::expr_holder_t mul_base::dispatch(scalar_exp const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  exp_mul visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

// zero * expr --> zero
mul_base::expr_holder_t mul_base::dispatch(scalar_zero const &) {
  return m_lhs;
}

// one * expr --> expr
mul_base::expr_holder_t mul_base::dispatch(scalar_one const &) { return m_rhs; }

} // namespace simplifier
} // namespace numsim::cas
