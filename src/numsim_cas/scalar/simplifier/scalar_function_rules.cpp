#include <numsim_cas/scalar/simplifier/scalar_function_rules.h>

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>

// Bodies mirror the previous inline folds in scalar_std.h exactly; #417
// extracts them into named, testable rules without changing behavior.

namespace numsim::cas::scalar_rules {

// ── sin ──────────────────────────────────────────────────────────────
std::optional<holder> try_sin_zero(holder const &e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  return {};
}
std::optional<holder> try_sin_inverse(holder const &e) {
  if (is_same<scalar_asin>(e))
    return e.get<scalar_asin>().expr();
  return {};
}
std::optional<holder> try_sin_odd(holder const &e) {
  if (is_same<scalar_negative>(e))
    return -sin(e.get<scalar_negative>().expr());
  return {};
}
std::optional<holder> try_sin_of_acos(holder const &e) {
  // sin(acos x) = √(1-x²); acos maps into [0,π] where sin ≥ 0.
  if (is_same<scalar_acos>(e)) {
    auto const &x = e.get<scalar_acos>().expr();
    return sqrt(get_scalar_one() - pow(x, 2));
  }
  return {};
}
std::optional<holder> try_sin_of_atan(holder const &e) {
  // sin(atan x) = x/√(1+x²).
  if (is_same<scalar_atan>(e)) {
    auto const &x = e.get<scalar_atan>().expr();
    auto den = sqrt(get_scalar_one() + pow(x, 2));
    return x / std::move(den);
  }
  return {};
}

// ── cos ──────────────────────────────────────────────────────────────
std::optional<holder> try_cos_zero(holder const &e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_one();
  return {};
}
std::optional<holder> try_cos_inverse(holder const &e) {
  if (is_same<scalar_acos>(e))
    return e.get<scalar_acos>().expr();
  return {};
}
std::optional<holder> try_cos_even(holder const &e) {
  if (is_same<scalar_negative>(e))
    return cos(e.get<scalar_negative>().expr());
  return {};
}
std::optional<holder> try_cos_of_asin(holder const &e) {
  // cos(asin x) = √(1-x²); asin maps into [-π/2,π/2] where cos ≥ 0.
  if (is_same<scalar_asin>(e)) {
    auto const &x = e.get<scalar_asin>().expr();
    return sqrt(get_scalar_one() - pow(x, 2));
  }
  return {};
}
std::optional<holder> try_cos_of_atan(holder const &e) {
  // cos(atan x) = 1/√(1+x²).
  if (is_same<scalar_atan>(e)) {
    auto const &x = e.get<scalar_atan>().expr();
    auto den = sqrt(get_scalar_one() + pow(x, 2));
    return get_scalar_one() / std::move(den);
  }
  return {};
}

// ── tan ──────────────────────────────────────────────────────────────
std::optional<holder> try_tan_zero(holder const &e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  return {};
}
std::optional<holder> try_tan_inverse(holder const &e) {
  if (is_same<scalar_atan>(e))
    return e.get<scalar_atan>().expr();
  return {};
}
std::optional<holder> try_tan_odd(holder const &e) {
  if (is_same<scalar_negative>(e))
    return -tan(e.get<scalar_negative>().expr());
  return {};
}
std::optional<holder> try_tan_of_asin(holder const &e) {
  // tan(asin x) = x/√(1-x²).
  if (is_same<scalar_asin>(e)) {
    auto const &x = e.get<scalar_asin>().expr();
    auto den = sqrt(get_scalar_one() - pow(x, 2));
    return x / std::move(den);
  }
  return {};
}

// ── asin/acos/atan ───────────────────────────────────────────────────
std::optional<holder> try_asin_zero(holder const &e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  return {};
}
std::optional<holder> try_asin_odd(holder const &e) {
  if (is_same<scalar_negative>(e))
    return -asin(e.get<scalar_negative>().expr());
  return {};
}
std::optional<holder> try_acos_one(holder const &e) {
  if (is_constant_one(e))
    return get_scalar_zero();
  return {};
}
std::optional<holder> try_atan_zero(holder const &e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  return {};
}
std::optional<holder> try_atan_odd(holder const &e) {
  if (is_same<scalar_negative>(e))
    return -atan(e.get<scalar_negative>().expr());
  return {};
}

// ── exp/log ──────────────────────────────────────────────────────────
std::optional<holder> try_exp_zero(holder const &e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_one();
  return {};
}
std::optional<holder> try_exp_of_log(holder const &e) {
  if (is_same<scalar_log>(e))
    return e.get<scalar_log>().expr();
  return {};
}
std::optional<holder> try_log_one(holder const &e) {
  if (is_constant_one(e))
    return get_scalar_zero();
  return {};
}
std::optional<holder> try_log_of_exp(holder const &e) {
  if (is_same<scalar_exp>(e))
    return e.get<scalar_exp>().expr();
  return {};
}
std::optional<holder> try_log_of_sqrt(holder const &e) {
  if (is_same<scalar_sqrt>(e)) {
    auto half = make_expression<scalar_constant>(scalar_number{1, 2});
    return log(e.get<scalar_sqrt>().expr()) * half;
  }
  return {};
}
std::optional<holder> try_log_of_pow(holder const &e) {
  if (is_same<scalar_pow>(e)) {
    auto const &p = e.get<scalar_pow>();
    if (is_positive(p.expr_lhs()))
      return p.expr_rhs() * log(p.expr_lhs());
  }
  return {};
}

// ── sqrt ─────────────────────────────────────────────────────────────
std::optional<holder> try_sqrt_zero(holder const &e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  return {};
}
std::optional<holder> try_sqrt_one(holder const &e) {
  if (is_constant_one(e))
    return get_scalar_one();
  return {};
}
std::optional<holder> try_sqrt_of_square(holder const &e) {
  if (is_same<scalar_pow>(e)) {
    auto const &p = e.get<scalar_pow>();
    if (is_same<scalar_constant>(p.expr_rhs()) &&
        p.expr_rhs().get<scalar_constant>().value() == scalar_number{2}) {
      if (is_nonnegative(p.expr_lhs()) || is_positive(p.expr_lhs()))
        return p.expr_lhs();
    }
  }
  return {};
}
std::optional<holder> try_sqrt_of_exp(holder const &e) {
  // sqrt(exp x) → exp(x/2), routed through pow(exp x, 1/2) so pow_base
  // rewrites it to exp(x·1/2) (matches the original scalar_std.h path).
  if (is_same<scalar_exp>(e)) {
    auto half = make_expression<scalar_constant>(scalar_number{1, 2});
    return binary_scalar_pow_simplify(e, std::move(half));
  }
  return {};
}

// ── abs/sign ─────────────────────────────────────────────────────────
std::optional<holder> try_abs_nonneg(holder const &e) {
  if (is_positive(e) || is_nonnegative(e))
    return e;
  return {};
}
std::optional<holder> try_abs_nonpos(holder const &e) {
  if (is_negative(e) || is_nonpositive(e))
    return -e;
  return {};
}
std::optional<holder> try_sign_zero(holder const &e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  return {};
}
std::optional<holder> try_sign_positive(holder const &e) {
  if (is_positive(e))
    return get_scalar_one();
  return {};
}
std::optional<holder> try_sign_negative(holder const &e) {
  if (is_negative(e))
    return -get_scalar_one();
  return {};
}

} // namespace numsim::cas::scalar_rules
