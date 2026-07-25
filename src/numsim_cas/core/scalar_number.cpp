#include <numsim_cas/core/scalar_number.h>

#include <cmath>
#include <limits>
#include <ostream>

namespace numsim::cas {

// ─── Rational normalization ──────────────────────────────────────

/// Normalize: GCD-reduce, ensure den > 0. Returns variant (int64 if den==1).
static scalar_number::variant_t normalize_rational(std::int64_t num,
                                                   std::int64_t den) {
  if (den == 0) {
    // Division by zero — fall back to double for inf/nan
    return static_cast<double>(num) / 0.0;
  }
  // INT64_MIN cannot be negated/abs'd — demote to double (#349)
  if (num == std::numeric_limits<std::int64_t>::min() ||
      den == std::numeric_limits<std::int64_t>::min()) {
    return static_cast<double>(num) / static_cast<double>(den);
  }
  if (num == 0) {
    return std::int64_t{0};
  }
  // Ensure den > 0
  if (den < 0) {
    num = -num;
    den = -den;
  }
  auto g = std::gcd(std::abs(num), den);
  num /= g;
  den /= g;
  if (den == 1) {
    return num;
  }
  return rational_t{num, den};
}

scalar_number::scalar_number(std::int64_t num, std::int64_t den)
    : v_(normalize_rational(num, den)) {}

scalar_number::scalar_number(rational_t r)
    : v_(normalize_rational(r.num, r.den)) {}

// ─── Helpers ─────────────────────────────────────────────────────

namespace {

template <class T>
constexpr bool is_cplx_v =
    std::is_same_v<std::decay_t<T>, std::complex<double>>;

template <class T>
constexpr bool is_rat_v = std::is_same_v<std::decay_t<T>, rational_t>;

template <class T> std::complex<double> to_complex(T const &v) {
  if constexpr (is_cplx_v<T>) {
    return v;
  } else if constexpr (is_rat_v<T>) {
    return std::complex<double>(
        static_cast<double>(v.num) / static_cast<double>(v.den), 0.0);
  } else {
    return std::complex<double>(static_cast<double>(v), 0.0);
  }
}

template <class T> double to_double(T const &v) {
  if constexpr (is_rat_v<T>) {
    return static_cast<double>(v.num) / static_cast<double>(v.den);
  } else {
    return static_cast<double>(v);
  }
}

template <class T> rational_t to_rational(T const &v) {
  if constexpr (is_rat_v<T>) {
    return v;
  } else {
    // T must be int64_t here
    return rational_t{static_cast<std::int64_t>(v), 1};
  }
}

// Promotion hierarchy: int64 → rational → double → complex
// Index: int64=0, double=1, complex=2, rational=3
// Promotion rank: int64=0, rational=1, double=2, complex=3
constexpr int promotion_rank(std::size_t variant_index) {
  // variant order: int64(0), double(1), complex(2), rational(3)
  constexpr int ranks[] = {0, 2, 3, 1};
  return ranks[variant_index];
}

template <class Op>
scalar_number::variant_t promote_binary(scalar_number::variant_t const &a,
                                        scalar_number::variant_t const &b,
                                        Op op) {
  return std::visit(
      [&](auto const &x, auto const &y) -> scalar_number::variant_t {
        using X = std::decay_t<decltype(x)>;
        using Y = std::decay_t<decltype(y)>;

        // promote to complex if either is complex
        if constexpr (is_cplx_v<X> || is_cplx_v<Y>) {
          return op(to_complex(x), to_complex(y));
        }
        // promote to double if either is double
        else if constexpr (std::is_same_v<X, double> ||
                           std::is_same_v<Y, double>) {
          return op(to_double(x), to_double(y));
        }
        // promote to rational if either is rational
        else if constexpr (is_rat_v<X> || is_rat_v<Y>) {
          return op(to_rational(x), to_rational(y));
        }
        // both int64
        else {
          return op(static_cast<std::int64_t>(x), static_cast<std::int64_t>(y));
        }
      },
      a, b);
}

// Overflow-checked int64 arithmetic (#349). MSVC has no
// __builtin_*_overflow, hence the manual fallbacks.
inline bool add_overflows(std::int64_t a, std::int64_t b, std::int64_t &r) {
#if defined(__GNUC__) || defined(__clang__)
  return __builtin_add_overflow(a, b, &r);
#else
  constexpr auto mx = std::numeric_limits<std::int64_t>::max();
  constexpr auto mn = std::numeric_limits<std::int64_t>::min();
  if ((b > 0 && a > mx - b) || (b < 0 && a < mn - b))
    return true;
  r = a + b;
  return false;
#endif
}

inline bool sub_overflows(std::int64_t a, std::int64_t b, std::int64_t &r) {
#if defined(__GNUC__) || defined(__clang__)
  return __builtin_sub_overflow(a, b, &r);
#else
  constexpr auto mx = std::numeric_limits<std::int64_t>::max();
  constexpr auto mn = std::numeric_limits<std::int64_t>::min();
  if ((b < 0 && a > mx + b) || (b > 0 && a < mn + b))
    return true;
  r = a - b;
  return false;
#endif
}

inline bool mul_overflows(std::int64_t a, std::int64_t b, std::int64_t &r) {
#if defined(__GNUC__) || defined(__clang__)
  return __builtin_mul_overflow(a, b, &r);
#else
  constexpr auto mx = std::numeric_limits<std::int64_t>::max();
  constexpr auto mn = std::numeric_limits<std::int64_t>::min();
  if (a == 0 || b == 0) {
    r = 0;
    return false;
  }
  if (a == -1 && b == mn)
    return true;
  if (b == -1 && a == mn)
    return true;
  if (a > 0 ? (b > 0 ? a > mx / b : b < mn / a)
            : (b > 0 ? a < mn / b : a < mx / b))
    return true;
  r = a * b;
  return false;
#endif
}

inline double rat_to_double(rational_t const &r) {
  return static_cast<double>(r.num) / static_cast<double>(r.den);
}

// Rational arithmetic helpers. Overflowing intermediates demote to double
// instead of wrapping (UB) — value stays correct, exactness is lost (#349).
scalar_number::variant_t rat_add(rational_t a, rational_t b) {
  // a.num/a.den + b.num/b.den = (a.num*b.den + b.num*a.den) / (a.den*b.den)
  std::int64_t t1, t2, num, den;
  if (mul_overflows(a.num, b.den, t1) || mul_overflows(b.num, a.den, t2) ||
      add_overflows(t1, t2, num) || mul_overflows(a.den, b.den, den)) {
    return rat_to_double(a) + rat_to_double(b);
  }
  return normalize_rational(num, den);
}

scalar_number::variant_t rat_sub(rational_t a, rational_t b) {
  std::int64_t t1, t2, num, den;
  if (mul_overflows(a.num, b.den, t1) || mul_overflows(b.num, a.den, t2) ||
      sub_overflows(t1, t2, num) || mul_overflows(a.den, b.den, den)) {
    return rat_to_double(a) - rat_to_double(b);
  }
  return normalize_rational(num, den);
}

scalar_number::variant_t rat_mul(rational_t a, rational_t b) {
  // INT64_MIN cannot be abs'd for the gcd cross-cancel; it can arrive via
  // int->rational promotion, which skips normalization (review on #349)
  constexpr auto mn = std::numeric_limits<std::int64_t>::min();
  if (a.num == mn || b.num == mn || a.den == mn || b.den == mn) {
    return rat_to_double(a) * rat_to_double(b);
  }
  // Cross-cancel before multiplying to keep intermediates small
  auto g1 = std::gcd(std::abs(a.num), std::abs(b.den));
  auto g2 = std::gcd(std::abs(b.num), std::abs(a.den));
  std::int64_t num, den;
  if (mul_overflows(a.num / g1, b.num / g2, num) ||
      mul_overflows(a.den / g2, b.den / g1, den)) {
    return rat_to_double(a) * rat_to_double(b);
  }
  return normalize_rational(num, den);
}

scalar_number::variant_t rat_div(rational_t a, rational_t b) {
  if (b.num == 0) {
    // a / 0 → ±inf double, matching the int/int path (#349)
    return rat_to_double(a) / 0.0;
  }
  if (b.num == std::numeric_limits<std::int64_t>::min()) {
    return rat_to_double(a) / rat_to_double(b);
  }
  return rat_mul(a, {b.den, b.num});
}

} // anonymous namespace

// ─── Arithmetic ──────────────────────────────────────────────────

scalar_number operator+(scalar_number const &a, scalar_number const &b) {
  return scalar_number(promote_binary(a.v_, b.v_, [](auto x, auto y) {
    using T = std::decay_t<decltype(x)>;
    if constexpr (is_rat_v<T>) {
      return rat_add(x, y);
    } else if constexpr (std::is_same_v<T, std::int64_t>) {
      std::int64_t r;
      if (add_overflows(x, y, r))
        return scalar_number::variant_t{static_cast<double>(x) +
                                        static_cast<double>(y)};
      return scalar_number::variant_t{r};
    } else {
      return scalar_number::variant_t{x + y};
    }
  }));
}

scalar_number operator-(scalar_number const &a, scalar_number const &b) {
  return scalar_number(promote_binary(a.v_, b.v_, [](auto x, auto y) {
    using T = std::decay_t<decltype(x)>;
    if constexpr (is_rat_v<T>) {
      return rat_sub(x, y);
    } else if constexpr (std::is_same_v<T, std::int64_t>) {
      std::int64_t r;
      if (sub_overflows(x, y, r))
        return scalar_number::variant_t{static_cast<double>(x) -
                                        static_cast<double>(y)};
      return scalar_number::variant_t{r};
    } else {
      return scalar_number::variant_t{x - y};
    }
  }));
}

scalar_number operator*(scalar_number const &a, scalar_number const &b) {
  return scalar_number(promote_binary(a.v_, b.v_, [](auto x, auto y) {
    using T = std::decay_t<decltype(x)>;
    if constexpr (is_rat_v<T>) {
      return rat_mul(x, y);
    } else if constexpr (std::is_same_v<T, std::int64_t>) {
      std::int64_t r;
      if (mul_overflows(x, y, r))
        return scalar_number::variant_t{static_cast<double>(x) *
                                        static_cast<double>(y)};
      return scalar_number::variant_t{r};
    } else {
      return scalar_number::variant_t{x * y};
    }
  }));
}

scalar_number operator/(scalar_number const &a, scalar_number const &b) {
  return scalar_number(promote_binary(a.v_, b.v_, [](auto x, auto y) {
    using T = std::decay_t<decltype(x)>;
    if constexpr (is_rat_v<T>) {
      return rat_div(x, y);
    } else if constexpr (std::is_same_v<T, std::int64_t>) {
      // int / int → rational (exact)
      return normalize_rational(x, y);
    } else {
      return scalar_number::variant_t{x / y};
    }
  }));
}

scalar_number operator-(scalar_number const &a) {
  return scalar_number(std::visit(
      [](auto const &x) -> scalar_number::variant_t {
        using T = std::decay_t<decltype(x)>;
        if constexpr (is_rat_v<T>) {
          if (x.num == std::numeric_limits<std::int64_t>::min())
            return -rat_to_double(x);
          return rational_t{-x.num, x.den};
        } else if constexpr (std::is_same_v<T, std::int64_t>) {
          if (x == std::numeric_limits<std::int64_t>::min())
            return -static_cast<double>(x);
          return -x;
        } else {
          return -x;
        }
      },
      a.v_));
}

// ─── I/O ─────────────────────────────────────────────────────────

std::ostream &operator<<(std::ostream &os, scalar_number const &a) {
  std::visit(
      [&](auto const &val) {
        using T = std::decay_t<decltype(val)>;
        if constexpr (is_rat_v<T>) {
          os << val.num << "/" << val.den;
        } else {
          os << val;
        }
      },
      a.v_);
  return os;
}

// ─── Comparison ──────────────────────────────────────────────────

bool operator==(scalar_number const &a, scalar_number const &b) {
  int ra = promotion_rank(a.v_.index());
  int rb = promotion_rank(b.v_.index());

  // Same promotion rank: compare directly
  if (ra == rb) {
    return std::visit(
        [](auto const &x, auto const &y) -> bool {
          using X = std::decay_t<decltype(x)>;
          using Y = std::decay_t<decltype(y)>;
          if constexpr (std::is_same_v<X, Y>) {
            if constexpr (is_rat_v<X>) {
              return x.num == y.num && x.den == y.den;
            } else {
              return x == y;
            }
          } else {
            // int vs int (shouldn't happen since same rank), but handle
            return false;
          }
        },
        a.v_, b.v_);
  }

  // Cross-rank comparison: promote to common type
  return std::visit(
      [](auto const &x, auto const &y) -> bool {
        using X = std::decay_t<decltype(x)>;
        using Y = std::decay_t<decltype(y)>;
        if constexpr (is_cplx_v<X> || is_cplx_v<Y>) {
          return to_complex(x) == to_complex(y);
        } else if constexpr (std::is_same_v<X, double> ||
                             std::is_same_v<Y, double>) {
          return to_double(x) == to_double(y);
        } else if constexpr (is_rat_v<X> || is_rat_v<Y>) {
          auto rx = to_rational(x);
          auto ry = to_rational(y);
          return rx.num == ry.num && rx.den == ry.den;
        } else {
          return static_cast<double>(x) == static_cast<double>(y);
        }
      },
      a.v_, b.v_);
}

namespace {
// Compare two rationals safely against int64 overflow in the
// cross-multiplication. With both denominators positive (rational_t
// invariant) the inequality `a.num/a.den < b.num/b.den` is equivalent
// to `a.num*b.den < b.num*a.den`, but each product can overflow int64
// for numerator/denominator pairs near 2^63. (#142)
//
// __int128 path: GCC/Clang. Exact, no precision loss, single multiply.
// long-double fallback: MSVC. Correct for the typical small-rational
// CAS use case (≤ ~2^53 magnitude); loses precision beyond that.
bool rat_less(rational_t a, rational_t b) {
#if defined(__SIZEOF_INT128__)
  __int128 lhs = static_cast<__int128>(a.num) * b.den;
  __int128 rhs = static_cast<__int128>(b.num) * a.den;
  return lhs < rhs;
#else
  return static_cast<long double>(a.num) * static_cast<long double>(b.den) <
         static_cast<long double>(b.num) * static_cast<long double>(a.den);
#endif
}
} // namespace

bool operator<(scalar_number const &a, scalar_number const &b) {
  int ra = promotion_rank(a.v_.index());
  int rb = promotion_rank(b.v_.index());

  if (ra != rb)
    return ra < rb;

  return std::visit(
      [&](auto const &x) {
        using X = std::decay_t<decltype(x)>;
        auto const &y = std::get<X>(b.v_);
        if constexpr (is_cplx_v<X>) {
          if (x.real() != y.real())
            return x.real() < y.real();
          return x.imag() < y.imag();
        } else if constexpr (is_rat_v<X>) {
          return rat_less(x, y);
        } else {
          return x < y;
        }
      },
      a.v_);
}

bool numeric_less(scalar_number const &a, scalar_number const &b) {
  return std::visit(
      [](auto const &x, auto const &y) -> bool {
        using X = std::decay_t<decltype(x)>;
        using Y = std::decay_t<decltype(y)>;
        if constexpr (is_cplx_v<X> || is_cplx_v<Y>) {
          auto cx = to_complex(x);
          auto cy = to_complex(y);
          if (cx.real() != cy.real())
            return cx.real() < cy.real();
          return cx.imag() < cy.imag();
        } else if constexpr (std::is_same_v<X, double> ||
                             std::is_same_v<Y, double>) {
          return to_double(x) < to_double(y);
        } else if constexpr (is_rat_v<X> || is_rat_v<Y>) {
          return rat_less(to_rational(x), to_rational(y));
        } else {
          return static_cast<std::int64_t>(x) < static_cast<std::int64_t>(y);
        }
      },
      a.v_, b.v_);
}

// ─── Misc ────────────────────────────────────────────────────────

scalar_number scalar_number::abs() const noexcept {
  return std::visit(
      [](auto const &x) -> scalar_number {
        using T = std::decay_t<decltype(x)>;

        if constexpr (std::is_same_v<T, std::int64_t>) {
          // avoid overflow on INT64_MIN
          if (x == std::numeric_limits<std::int64_t>::min()) {
            return scalar_number(std::fabs(static_cast<double>(x)));
          }
          return scalar_number(x < 0 ? -x : x);
        } else if constexpr (std::is_same_v<T, double>) {
          return scalar_number(std::fabs(x));
        } else if constexpr (is_rat_v<T>) {
          return scalar_number(rational_t{x.num < 0 ? -x.num : x.num, x.den});
        } else { // std::complex<double>
          return scalar_number(std::abs(x));
        }
      },
      v_);
}

// ─── Pow ──────────────────────────────────────────────────────────

std::optional<scalar_number> pow(scalar_number const &base,
                                 scalar_number const &exp) {
  // Extract integer exponent from the variant.
  std::optional<std::int64_t> int_exp;
  std::visit(
      [&](auto const &v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::int64_t>) {
          int_exp = v;
        } else if constexpr (is_rat_v<T>) {
          if (v.den == 1)
            int_exp = v.num;
        }
        // double, complex → not exact
      },
      exp.raw());

  if (!int_exp)
    return std::nullopt;

  auto n = *int_exp;

  // base == 0 && n < 0 → division by zero
  if (base == scalar_number{0}) {
    if (n < 0)
      return std::nullopt;
    if (n == 0)
      return scalar_number{1}; // 0^0 = 1 by convention
    return scalar_number{0};
  }

  if (n == 0)
    return scalar_number{1};

  // Compute |n| via repeated squaring
  bool negative = n < 0;
  std::uint64_t abs_n =
      negative ? static_cast<std::uint64_t>(-n) : static_cast<std::uint64_t>(n);

  scalar_number result{1};
  scalar_number b = base;
  while (abs_n > 0) {
    if (abs_n & 1)
      result = result * b;
    b = b * b;
    abs_n >>= 1;
  }

  if (negative)
    result = scalar_number{1} / result;

  return result;
}

} // namespace numsim::cas
