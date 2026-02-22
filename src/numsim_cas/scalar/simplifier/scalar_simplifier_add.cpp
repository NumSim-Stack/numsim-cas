#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/scalar/scalar_definitions.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/simplifier/scalar_simplifier_add.h>

namespace numsim::cas {
namespace simplifier {

// ------------------------------------------------------------
// Pythagorean identity helper
// ------------------------------------------------------------
static std::optional<expression_holder<scalar_expression>>
try_trig_pythagorean(scalar_pow const &a, scalar_pow const &b) {
  // Both exponents must be constant 2
  auto a_exp = is_same_r<scalar_constant>(a.expr_rhs());
  auto b_exp = is_same_r<scalar_constant>(b.expr_rhs());
  if (!a_exp || !b_exp)
    return {};
  if (a_exp->get().value() != scalar_number{2})
    return {};
  if (b_exp->get().value() != scalar_number{2})
    return {};

  // One base must be sin(x) and the other cos(x) for same x
  auto a_sin = is_same_r<scalar_sin>(a.expr_lhs());
  auto b_cos = is_same_r<scalar_cos>(b.expr_lhs());
  if (a_sin && b_cos && a_sin->get().expr() == b_cos->get().expr())
    return get_scalar_one();

  auto a_cos = is_same_r<scalar_cos>(a.expr_lhs());
  auto b_sin = is_same_r<scalar_sin>(b.expr_lhs());
  if (a_cos && b_sin && a_cos->get().expr() == b_sin->get().expr())
    return get_scalar_one();

  return {};
}

// ------------------------------------------------------------
// pow_add
// ------------------------------------------------------------
pow_add::pow_add(expr_holder_t lhs, expr_holder_t rhs)
    : algo(std::move(lhs), std::move(rhs)),
      m_lhs_node{algo::m_lhs.template get<scalar_pow>()} {}

pow_add::expr_holder_t pow_add::dispatch(scalar_pow const &rhs) {
  if (auto result = try_trig_pythagorean(m_lhs_node, rhs))
    return std::move(*result);
  return algo::get_default();
}

// ------------------------------------------------------------
// n_ary_add::dispatch(scalar_pow)
// ------------------------------------------------------------
n_ary_add::expr_holder_t n_ary_add::dispatch(scalar_pow const &rhs) {
  const auto pows = get_all<scalar_pow>(lhs);
  for (const auto &e : pows) {
    if (auto result =
            try_trig_pythagorean(e.template get<scalar_pow>(), rhs)) {
      auto add_expr{make_expression<scalar_add>(lhs)};
      auto &add{add_expr.template get<scalar_add>()};
      add.symbol_map().erase(e);
      const auto value{get_coefficient<scalar_traits>(lhs, 0) + 1};
      add.coeff().free();
      if (value != 0) {
        add.set_coeff(scalar_traits::make_constant(value));
      }
      return add_expr;
    }
  }
  // No Pythagorean match: push RHS into existing add
  auto add_expr{make_expression<scalar_add>(lhs)};
  auto &add{add_expr.template get<scalar_add>()};
  add.push_back(m_rhs);
  return add_expr;
}

// ------------------------------------------------------------
// add_base
// ------------------------------------------------------------
add_base::add_base(expr_holder_t lhs, expr_holder_t rhs)
    : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

add_base::expr_holder_t add_base::dispatch(scalar_constant const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  constant_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(scalar_one const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  add_scalar_one visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(scalar_add const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  n_ary_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(scalar_mul const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  n_ary_mul_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(scalar const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  symbol_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(scalar_negative const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  add_negative visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(scalar_pow const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  pow_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(scalar_zero const &) {
  return std::move(m_rhs);
}

} // namespace simplifier
} // namespace numsim::cas
