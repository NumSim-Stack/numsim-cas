#include <numsim_cas/scalar/visitors/scalar_limit_visitor.h>

#include <numsim_cas/scalar/scalar_assume.h>
#include <numsim_cas/scalar/scalar_functions.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <ranges>

namespace numsim::cas {

using dir = limit_result::direction;

scalar_limit_visitor::scalar_limit_visitor(expr_holder_t const &limit_var,
                                           limit_target target)
    : m_limit_var(limit_var), m_target(target) {}

namespace {

// Nonnegative by construction, so a limit of zero is approached from above.
bool is_structurally_nonnegative(
    expression_holder<scalar_expression> const &e) {
  if (is_same<scalar_abs>(e) || is_same<scalar_exp>(e))
    return true;
  if (is_same<scalar_pow>(e)) {
    auto exponent = try_int_constant(e.get<scalar_pow>().expr_rhs());
    return exponent && *exponent % 2 == 0;
  }
  return false;
}

// asin and acos are real only on [-1, 1]; outside it the value is NaN.
bool is_within_unit_interval(expression_holder<scalar_expression> const &e) {
  if (is_same<scalar_sin>(e) || is_same<scalar_cos>(e) ||
      is_same<scalar_sign>(e))
    return true;
  auto value = domain_traits<scalar_expression>::try_numeric(e);
  if (!value)
    return false;
  auto magnitude = std::visit(
      [](auto const &x) -> double {
        using V = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<V, std::complex<double>>) {
          return std::abs(x.real());
        } else if constexpr (std::is_same_v<V, rational_t>) {
          return std::abs(static_cast<double>(x.num) /
                          static_cast<double>(x.den));
        } else {
          return std::abs(static_cast<double>(x));
        }
      },
      value->raw());
  return magnitude <= 1.0;
}

limit_result target_to_limit(limit_target target) {
  using pt = limit_target::point;
  switch (target.target) {
  case pt::zero_plus:
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

bool scalar_limit_visitor::zero_from_above(expr_holder_t const &expr) const {
  if (expr == m_limit_var)
    return m_target.target == limit_target::point::zero_plus;
  // sqrt is nonnegative exactly where it is defined, so it reaches zero from
  // above only if its operand does.
  if (is_same<scalar_sqrt>(expr))
    return zero_from_above(expr.get<scalar_sqrt>().expr());
  return is_positive(expr) || is_nonnegative(expr) ||
         is_structurally_nonnegative(expr);
}

bool scalar_limit_visitor::zero_from_below(expr_holder_t const &expr) const {
  if (expr == m_limit_var)
    return m_target.target == limit_target::point::zero_minus;
  return is_negative(expr);
}

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

void scalar_limit_visitor::operator()(scalar const &v) {
  // Not the limit variable (handled in apply()), so a constant of unknown sign
  // unless its assumptions say otherwise.
  if (v.assumptions().contains(positive{}))
    m_result = {dir::finite_positive};
  else if (v.assumptions().contains(negative{}))
    m_result = {dir::finite_negative};
  else
    m_result = {dir::unknown};
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
        } else if constexpr (std::is_same_v<V, rational_t>) {
          return static_cast<double>(x.num) / static_cast<double>(x.den);
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
  for (auto const &child : v.symbol_map() | std::views::values) {
    result = combine_add(result, apply(child));
  }
  m_result = result;
}

void scalar_limit_visitor::operator()(scalar_mul const &v) {
  limit_result result{dir::finite_positive}; // multiplicative identity
  if (v.coeff().is_valid()) {
    result = apply(v.coeff());
  }
  for (auto const &child : v.symbol_map() | std::views::values) {
    result = combine_mul(result, apply(child));
  }
  m_result = result;
}

void scalar_limit_visitor::operator()(scalar_negative const &v) {
  m_result = apply_neg(apply(v.expr()));
}

void scalar_limit_visitor::operator()(scalar_pow const &v) {
  // An even integer exponent makes the power nonnegative from either side.
  auto exponent = try_int_constant(v.expr_rhs());
  bool base_from_above = zero_from_above(v.expr_lhs());
  bool from_above = base_from_above || (exponent && *exponent % 2 == 0);
  auto base = apply(v.expr_lhs());
  m_result = apply_pow(base, apply(v.expr_rhs()), from_above);
  // a non-integer power of a base that may reach zero from below is NaN
  if (base.dir == dir::zero && !base_from_above && !exponent)
    m_result = {dir::unknown};
}

// ─── Functions ────────────────────────────────────────────────────

// For a finite nonzero argument sin, cos and tan can take either sign; at
// infinity sin and cos oscillate.
void scalar_limit_visitor::operator()(scalar_sin const &v) {
  auto child = apply(v.expr());
  if (child.dir == dir::indeterminate)
    m_result = child;
  else if (child.dir == dir::zero)
    m_result = {dir::zero};
  else
    m_result = {dir::unknown};
}

void scalar_limit_visitor::operator()(scalar_cos const &v) {
  auto child = apply(v.expr());
  if (child.dir == dir::indeterminate)
    m_result = child;
  else if (child.dir == dir::zero)
    m_result = {dir::finite_positive};
  else
    m_result = {dir::unknown};
}

void scalar_limit_visitor::operator()(scalar_tan const &v) {
  auto child = apply(v.expr());
  if (child.dir == dir::indeterminate)
    m_result = child;
  else if (child.dir == dir::zero)
    m_result = {dir::zero};
  else
    m_result = {dir::unknown};
}

void scalar_limit_visitor::operator()(scalar_exp const &v) {
  m_result = limit_algebra::apply_exp(apply(v.expr()));
}

void scalar_limit_visitor::operator()(scalar_log const &v) {
  m_result = apply_log(apply(v.expr()), zero_from_above(v.expr()));
}

void scalar_limit_visitor::operator()(scalar_sqrt const &v) {
  m_result = apply_sqrt(apply(v.expr()), zero_from_above(v.expr()));
}

void scalar_limit_visitor::operator()(scalar_abs const &v) {
  m_result = apply_abs(apply(v.expr()));
}

void scalar_limit_visitor::operator()(scalar_sign const &v) {
  auto child = apply(v.expr());
  switch (child.dir) {
  case dir::indeterminate:
    m_result = child;
    break;
  case dir::zero:
    // sign jumps at 0: the limit is 1 only when the argument stays positive
    if (zero_from_above(v.expr()))
      m_result = {dir::finite_positive};
    else if (zero_from_below(v.expr()))
      m_result = {dir::finite_negative};
    else
      m_result = {dir::unknown};
    break;
  case dir::finite_positive:
  case dir::pos_infinity:
    m_result = {dir::finite_positive};
    break;
  case dir::finite_negative:
  case dir::neg_infinity:
    m_result = {dir::finite_negative};
    break;
  default:
    m_result = {dir::unknown};
  }
}

// asin and acos are real only on [-1, 1], so a sign needs an argument that is
// provably in range: asin(0) = 0, acos(0) = pi/2.
void scalar_limit_visitor::inverse_trig(expr_holder_t const &arg,
                                        bool is_asin) {
  auto child = apply(arg);
  if (child.dir == dir::indeterminate) {
    m_result = child;
    return;
  }
  if (child.dir == dir::zero) {
    m_result =
        is_asin ? limit_result{dir::zero} : limit_result{dir::finite_positive};
    return;
  }
  if (!is_within_unit_interval(arg)) {
    m_result = {dir::unknown};
    return;
  }
  if (child.dir == dir::finite_negative) {
    // asin maps [-1, 0) below zero; acos maps it into (pi/2, pi]
    m_result = {is_asin ? dir::finite_negative : dir::finite_positive};
    return;
  }
  // asin(c) > 0 for c in (0, 1]; acos(1) = 0 leaves acos open
  if (is_asin && child.dir == dir::finite_positive)
    m_result = {dir::finite_positive};
  else
    m_result = {dir::unknown};
}

void scalar_limit_visitor::operator()(scalar_asin const &v) {
  inverse_trig(v.expr(), true);
}

void scalar_limit_visitor::operator()(scalar_acos const &v) {
  inverse_trig(v.expr(), false);
}

void scalar_limit_visitor::operator()(scalar_atan const &v) {
  auto child = apply(v.expr());
  switch (child.dir) {
  case dir::indeterminate:
    m_result = child;
    break;
  case dir::zero:
    m_result = {dir::zero};
    break;
  case dir::finite_positive:
  case dir::pos_infinity:
    m_result = {dir::finite_positive};
    break;
  case dir::finite_negative:
  case dir::neg_infinity:
    m_result = {dir::finite_negative};
    break;
  default:
    m_result = {dir::unknown};
  }
}

void scalar_limit_visitor::operator()(
    [[maybe_unused]] scalar_named_expression const &v) {
  // Generic user-defined function: can't determine limit
  m_result = {dir::unknown};
}

// ─── Comparison nodes (#136) ─────────────────────────────────────
// Indicators are step functions in {0, 1}. Resolving the limit
// requires knowing the comparison's exact crossing point relative to
// the limit target, which the children's one-sided behaviour alone
// rarely pins down. The conservative `unknown` here can be tightened
// later (e.g. lt(+inf, finite) → 0) if that ever shows up as a real
// bottleneck. Tightening goes in one place because all six ops share
// this implementation via the static_assert below.
void scalar_limit_visitor::operator()([[maybe_unused]] scalar_lt const &) {
  m_result = {dir::unknown};
}
void scalar_limit_visitor::operator()([[maybe_unused]] scalar_gt const &) {
  m_result = {dir::unknown};
}
void scalar_limit_visitor::operator()([[maybe_unused]] scalar_le const &) {
  m_result = {dir::unknown};
}
void scalar_limit_visitor::operator()([[maybe_unused]] scalar_ge const &) {
  m_result = {dir::unknown};
}
void scalar_limit_visitor::operator()([[maybe_unused]] scalar_eq const &) {
  m_result = {dir::unknown};
}
void scalar_limit_visitor::operator()([[maybe_unused]] scalar_ne const &) {
  m_result = {dir::unknown};
}

// ─── Min / max (#137) ──────────────────────────────────────────────
// The limit of max(a, b) is max(lim a, lim b) when both limits exist
// and the comparison is decidable. We don't have that machinery in
// the limit_visitor yet, so report unknown — same as the comparison
// nodes above. Future tightening: when both children's limits are
// finite real numbers, evaluate max/min concretely.
void scalar_limit_visitor::operator()([[maybe_unused]] scalar_max const &) {
  m_result = {dir::unknown};
}
void scalar_limit_visitor::operator()([[maybe_unused]] scalar_min const &) {
  m_result = {dir::unknown};
}

// if_then_else (#135): limit depends on the condition's eventual
// behaviour near the limit target — out of scope for the current
// limit machinery. Report unknown.
void scalar_limit_visitor::operator()(
    [[maybe_unused]] scalar_if_then_else const &) {
  m_result = {dir::unknown};
}

} // namespace numsim::cas
