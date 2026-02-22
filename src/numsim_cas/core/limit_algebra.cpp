#include <numsim_cas/core/limit_algebra.h>

#include <algorithm>
#include <cmath>

namespace numsim::cas {

namespace {

using dir = limit_result::direction;
using gtype = growth_rate::type;

bool is_infinite(dir d) {
  return d == dir::pos_infinity || d == dir::neg_infinity;
}

bool is_finite(dir d) {
  return d == dir::zero || d == dir::finite_positive ||
         d == dir::finite_negative;
}

bool is_positive(dir d) {
  return d == dir::pos_infinity || d == dir::finite_positive;
}

bool is_negative(dir d) {
  return d == dir::neg_infinity || d == dir::finite_negative;
}

dir flip_sign(dir d) {
  switch (d) {
  case dir::pos_infinity:
    return dir::neg_infinity;
  case dir::neg_infinity:
    return dir::pos_infinity;
  case dir::finite_positive:
    return dir::finite_negative;
  case dir::finite_negative:
    return dir::finite_positive;
  default:
    return d;
  }
}

// Compare growth rates: return >0 if a grows faster, <0 if b grows faster, 0
// if same
int compare_growth(growth_rate a, growth_rate b) {
  // Order: constant < logarithmic < polynomial < exponential
  auto rank = [](gtype t) -> int {
    switch (t) {
    case gtype::constant:
      return 0;
    case gtype::logarithmic:
      return 1;
    case gtype::polynomial:
      return 2;
    case gtype::exponential:
      return 3;
    case gtype::unknown:
      return -1;
    }
    return -1;
  };

  int ra = rank(a.rate);
  int rb = rank(b.rate);
  if (ra != rb)
    return ra - rb;

  // Same rate type: compare exponents
  if (a.exponent > b.exponent)
    return 1;
  if (a.exponent < b.exponent)
    return -1;
  return 0;
}

growth_rate faster_growth(growth_rate a, growth_rate b) {
  return compare_growth(a, b) >= 0 ? a : b;
}

} // namespace

limit_result limit_algebra::combine_add(limit_result a, limit_result b) {
  // Propagate indeterminate/unknown
  if (a.dir == dir::indeterminate || b.dir == dir::indeterminate)
    return {dir::indeterminate};
  if (a.dir == dir::unknown || b.dir == dir::unknown)
    return {dir::unknown};

  // zero is additive identity
  if (a.dir == dir::zero)
    return b;
  if (b.dir == dir::zero)
    return a;

  // inf + inf (same sign): infinity with faster growth
  if (a.dir == dir::pos_infinity && b.dir == dir::pos_infinity)
    return {dir::pos_infinity, faster_growth(a.rate, b.rate)};
  if (a.dir == dir::neg_infinity && b.dir == dir::neg_infinity)
    return {dir::neg_infinity, faster_growth(a.rate, b.rate)};

  // inf - inf: indeterminate
  if (is_infinite(a.dir) && is_infinite(b.dir))
    return {dir::indeterminate};

  // inf + finite: infinity dominates (keep same rate)
  if (is_infinite(a.dir) && is_finite(b.dir))
    return a;
  if (is_finite(a.dir) && is_infinite(b.dir))
    return b;

  // finite + finite: finite (can't determine exact sign in general)
  return {dir::finite_positive};
}

limit_result limit_algebra::combine_mul(limit_result a, limit_result b) {
  // Propagate indeterminate/unknown
  if (a.dir == dir::indeterminate || b.dir == dir::indeterminate)
    return {dir::indeterminate};
  if (a.dir == dir::unknown || b.dir == dir::unknown)
    return {dir::unknown};

  // zero * anything
  if (a.dir == dir::zero && b.dir == dir::zero)
    return {dir::zero};
  if (a.dir == dir::zero && is_finite(b.dir))
    return {dir::zero};
  if (is_finite(a.dir) && b.dir == dir::zero)
    return {dir::zero};
  // 0 * inf: indeterminate
  if (a.dir == dir::zero && is_infinite(b.dir))
    return {dir::indeterminate};
  if (is_infinite(a.dir) && b.dir == dir::zero)
    return {dir::indeterminate};

  // Determine result sign
  bool result_positive = (is_positive(a.dir) == is_positive(b.dir)) ||
                         (is_negative(a.dir) == is_negative(b.dir));

  // inf * inf: larger infinity
  if (is_infinite(a.dir) && is_infinite(b.dir)) {
    growth_rate combined{};
    if (a.rate.rate == gtype::polynomial && b.rate.rate == gtype::polynomial) {
      combined = {gtype::polynomial, a.rate.exponent + b.rate.exponent};
    } else if (a.rate.rate == gtype::exponential ||
               b.rate.rate == gtype::exponential) {
      combined = {gtype::exponential,
                  std::max(a.rate.exponent, b.rate.exponent)};
    } else {
      combined = faster_growth(a.rate, b.rate);
    }
    return {result_positive ? dir::pos_infinity : dir::neg_infinity, combined};
  }

  // inf * finite_nonzero: infinity (same rate)
  if (is_infinite(a.dir) && is_finite(b.dir)) {
    return {result_positive ? dir::pos_infinity : dir::neg_infinity, a.rate};
  }
  if (is_finite(a.dir) && is_infinite(b.dir)) {
    return {result_positive ? dir::pos_infinity : dir::neg_infinity, b.rate};
  }

  // finite * finite: finite
  if (is_positive(a.dir) == is_positive(b.dir))
    return {dir::finite_positive};
  return {dir::finite_negative};
}

limit_result limit_algebra::apply_neg(limit_result a) {
  if (a.dir == dir::indeterminate || a.dir == dir::unknown)
    return a;
  return {flip_sign(a.dir), a.rate};
}

limit_result limit_algebra::apply_log(limit_result a) {
  if (a.dir == dir::indeterminate || a.dir == dir::unknown)
    return a;

  switch (a.dir) {
  case dir::zero:
    // log(0+) = -inf (logarithmic)
    return {dir::neg_infinity, {gtype::logarithmic, 1.0}};
  case dir::finite_positive:
    // log(finite_positive) = finite
    return {dir::finite_positive};
  case dir::finite_negative:
    // log(negative) is undefined in reals
    return {dir::unknown};
  case dir::pos_infinity:
    // log(+inf) = +inf (logarithmic -- slower than any polynomial)
    return {dir::pos_infinity, {gtype::logarithmic, 1.0}};
  case dir::neg_infinity:
    // log(-inf) undefined in reals
    [[fallthrough]];
  default:
    return {dir::unknown};
  }
}

limit_result limit_algebra::apply_pow(limit_result base,
                                      limit_result exponent) {
  if (base.dir == dir::indeterminate || exponent.dir == dir::indeterminate)
    return {dir::indeterminate};
  if (base.dir == dir::unknown || exponent.dir == dir::unknown)
    return {dir::unknown};

  // Only handle constant/finite exponents for now
  if (is_finite(exponent.dir) || exponent.dir == dir::zero) {
    if (exponent.dir == dir::zero) {
      // x^0 = 1
      return {dir::finite_positive};
    }

    bool exp_positive = is_positive(exponent.dir);

    switch (base.dir) {
    case dir::zero:
      if (exp_positive) {
        // 0^(+c) = 0
        return {dir::zero};
      } else {
        // 0^(-c) = +inf (polynomial)
        return {dir::pos_infinity, {gtype::polynomial, 1.0}};
      }
    case dir::finite_positive:
      // finite_pos ^ finite = finite_pos
      return {dir::finite_positive};
    case dir::finite_negative:
      // finite_neg ^ finite: sign depends on exponent, treat as unknown
      return {dir::unknown};
    case dir::pos_infinity:
      if (exp_positive) {
        // (+inf)^(+c) = +inf (polynomial)
        return {dir::pos_infinity, {gtype::polynomial, 1.0}};
      } else {
        // (+inf)^(-c) = 0
        return {dir::zero};
      }
    case dir::neg_infinity:
      // (-inf)^c: sign depends on parity, treat as unknown
      [[fallthrough]];
    default:
      return {dir::unknown};
    }
  }

  // Infinite exponent cases
  if (base.dir == dir::finite_positive) {
    // c^(+inf) or c^(-inf): depends on whether c > 1 or c < 1
    // Can't determine without knowing the actual value
    return {dir::unknown};
  }

  // inf^inf etc: complex
  return {dir::unknown};
}

limit_result limit_algebra::apply_sqrt(limit_result a) {
  if (a.dir == dir::indeterminate || a.dir == dir::unknown)
    return a;

  switch (a.dir) {
  case dir::zero:
    return {dir::zero};
  case dir::finite_positive:
    return {dir::finite_positive};
  case dir::finite_negative:
    return {dir::unknown}; // sqrt of negative
  case dir::pos_infinity: {
    // sqrt(+inf) = +inf, but slower: poly(p) -> poly(p/2)
    growth_rate rate = a.rate;
    if (rate.rate == gtype::polynomial) {
      rate.exponent /= 2.0;
    }
    return {dir::pos_infinity, rate};
  }
  case dir::neg_infinity:
    // sqrt of negative
    [[fallthrough]];
  default:
    return {dir::unknown};
  }
}

limit_result limit_algebra::apply_abs(limit_result a) {
  if (a.dir == dir::indeterminate || a.dir == dir::unknown)
    return a;

  switch (a.dir) {
  case dir::zero:
    return {dir::zero};
  case dir::finite_positive:
  case dir::finite_negative:
    return {dir::finite_positive};
  case dir::pos_infinity:
  case dir::neg_infinity:
    return {dir::pos_infinity, a.rate};
  default:
    return {dir::unknown};
  }
}

limit_result limit_algebra::apply_reciprocal(limit_result a) {
  if (a.dir == dir::indeterminate || a.dir == dir::unknown)
    return a;

  switch (a.dir) {
  case dir::zero:
    // 1/0 = +inf (polynomial, degree 1)
    return {dir::pos_infinity, {gtype::polynomial, 1.0}};
  case dir::finite_positive:
    return {dir::finite_positive};
  case dir::finite_negative:
    return {dir::finite_negative};
  case dir::pos_infinity:
  case dir::neg_infinity:
    // 1/inf = 0
    return {dir::zero};
  default:
    return {dir::unknown};
  }
}

limit_result limit_algebra::apply_exp(limit_result a) {
  if (a.dir == dir::indeterminate || a.dir == dir::unknown)
    return a;

  switch (a.dir) {
  case dir::zero:
    // exp(0) = 1
  case dir::finite_positive:
  case dir::finite_negative:
    // exp(finite) = finite positive
    return {dir::finite_positive};
  case dir::pos_infinity:
    // exp(+inf) = +inf (exponential)
    return {dir::pos_infinity, {gtype::exponential, 1.0}};
  case dir::neg_infinity:
    // exp(-inf) = 0
    return {dir::zero};
  default:
    return {dir::unknown};
  }
}

} // namespace numsim::cas
