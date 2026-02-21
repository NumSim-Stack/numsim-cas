#include <numsim_cas/scalar/visitors/scalar_limit_visitor.h>

#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <ranges>

namespace numsim::cas {

using dir = limit_result::direction;

scalar_limit_visitor::scalar_limit_visitor(expr_holder_t const &limit_var,
                                           limit_target target)
    : m_limit_var(limit_var), m_target(target) {}

namespace {

limit_result target_to_limit(limit_target target) {
  using pt = limit_target::point;
  switch (target.target) {
  case pt::zero_plus:
    return {dir::zero};
  case pt::zero_minus:
    return {dir::zero};
  case pt::pos_infinity:
    return {dir::pos_infinity, {growth_rate::type::polynomial, 1.0}};
  case pt::neg_infinity:
    return {dir::neg_infinity, {growth_rate::type::polynomial, 1.0}};
  }
  return {dir::unknown};
}

} // namespace

limit_result scalar_limit_visitor::apply(expr_holder_t const &expr) {
  if (!expr.is_valid())
    return {dir::zero};
  // If this expression IS the limit variable, return target behavior
  if (expr == m_limit_var) {
    m_result = target_to_limit(m_target);
    return m_result;
  }
  expr.template get<scalar_visitable_t>().accept(*this);
  return m_result;
}

// ─── Leaf nodes ───────────────────────────────────────────────────

void scalar_limit_visitor::operator()([[maybe_unused]] scalar const &) {
  // If we reach here, this symbol is NOT the limit variable
  // (the limit variable case is handled in apply() before dispatching)
  m_result = {dir::finite_positive};
}

void scalar_limit_visitor::operator()([[maybe_unused]] scalar_zero const &) {
  m_result = {dir::zero};
}

void scalar_limit_visitor::operator()([[maybe_unused]] scalar_one const &) {
  m_result = {dir::finite_positive};
}

void scalar_limit_visitor::operator()(scalar_constant const &v) {
  auto val = std::visit(
      [](auto const &x) -> double {
        using V = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<V, std::complex<double>>) {
          return x.real();
        } else {
          return static_cast<double>(x);
        }
      },
      v.value().raw());

  if (val > 0)
    m_result = {dir::finite_positive};
  else if (val < 0)
    m_result = {dir::finite_negative};
  else
    m_result = {dir::zero};
}

// ─── Arithmetic ───────────────────────────────────────────────────

void scalar_limit_visitor::operator()(scalar_add const &v) {
  limit_result result{dir::zero};
  if (v.coeff().is_valid()) {
    result = apply(v.coeff());
  }
  for (auto const &child : v.hash_map() | std::views::values) {
    result = combine_add(result, apply(child));
  }
  m_result = result;
}

void scalar_limit_visitor::operator()(scalar_mul const &v) {
  limit_result result{dir::finite_positive}; // multiplicative identity
  if (v.coeff().is_valid()) {
    result = apply(v.coeff());
  }
  for (auto const &child : v.hash_map() | std::views::values) {
    result = combine_mul(result, apply(child));
  }
  m_result = result;
}

void scalar_limit_visitor::operator()(scalar_negative const &v) {
  m_result = apply_neg(apply(v.expr()));
}

void scalar_limit_visitor::operator()(scalar_pow const &v) {
  m_result = apply_pow(apply(v.expr_lhs()), apply(v.expr_rhs()));
}

void scalar_limit_visitor::operator()(scalar_rational const &v) {
  auto num = apply(v.expr_lhs());
  auto den = apply(v.expr_rhs());
  m_result = combine_mul(num, apply_reciprocal(den));
}

// ─── Functions ────────────────────────────────────────────────────

void scalar_limit_visitor::operator()(scalar_sin const &v) {
  auto child = apply(v.expr());
  if (child.dir == dir::indeterminate || child.dir == dir::unknown) {
    m_result = child;
  } else if (child.dir == dir::pos_infinity || child.dir == dir::neg_infinity) {
    // sin oscillates => indeterminate
    m_result = {dir::unknown};
  } else {
    // finite input => bounded output in [-1, 1]
    m_result = {dir::finite_positive};
  }
}

void scalar_limit_visitor::operator()(scalar_cos const &v) {
  auto child = apply(v.expr());
  if (child.dir == dir::indeterminate || child.dir == dir::unknown) {
    m_result = child;
  } else if (child.dir == dir::pos_infinity || child.dir == dir::neg_infinity) {
    m_result = {dir::unknown};
  } else {
    m_result = {dir::finite_positive};
  }
}

void scalar_limit_visitor::operator()(scalar_tan const &v) {
  auto child = apply(v.expr());
  if (child.dir == dir::indeterminate || child.dir == dir::unknown) {
    m_result = child;
  } else {
    // tan can diverge at pi/2 + n*pi, treat as unknown in general
    m_result = {dir::unknown};
  }
}

void scalar_limit_visitor::operator()(scalar_exp const &v) {
  m_result = limit_algebra::apply_exp(apply(v.expr()));
}

void scalar_limit_visitor::operator()(scalar_log const &v) {
  m_result = apply_log(apply(v.expr()));
}

void scalar_limit_visitor::operator()(scalar_sqrt const &v) {
  m_result = apply_sqrt(apply(v.expr()));
}

void scalar_limit_visitor::operator()(scalar_abs const &v) {
  m_result = apply_abs(apply(v.expr()));
}

void scalar_limit_visitor::operator()(scalar_sign const &v) {
  auto child = apply(v.expr());
  if (child.dir == dir::indeterminate || child.dir == dir::unknown) {
    m_result = child;
  } else {
    // sign is bounded in {-1, 0, 1}
    m_result = {dir::finite_positive};
  }
}

void scalar_limit_visitor::operator()(scalar_asin const &v) {
  auto child = apply(v.expr());
  if (child.dir == dir::indeterminate || child.dir == dir::unknown) {
    m_result = child;
  } else {
    // asin is bounded [-pi/2, pi/2] for finite input
    m_result = {dir::finite_positive};
  }
}

void scalar_limit_visitor::operator()(scalar_acos const &v) {
  auto child = apply(v.expr());
  if (child.dir == dir::indeterminate || child.dir == dir::unknown) {
    m_result = child;
  } else {
    m_result = {dir::finite_positive};
  }
}

void scalar_limit_visitor::operator()(scalar_atan const &v) {
  auto child = apply(v.expr());
  if (child.dir == dir::indeterminate || child.dir == dir::unknown) {
    m_result = child;
  } else if (child.dir == dir::pos_infinity) {
    // atan(+inf) = pi/2
    m_result = {dir::finite_positive};
  } else if (child.dir == dir::neg_infinity) {
    // atan(-inf) = -pi/2
    m_result = {dir::finite_negative};
  } else {
    m_result = {dir::finite_positive};
  }
}

void scalar_limit_visitor::operator()(
    [[maybe_unused]] scalar_named_expression const &v) {
  // Generic user-defined function: can't determine limit
  m_result = {dir::unknown};
}

} // namespace numsim::cas
