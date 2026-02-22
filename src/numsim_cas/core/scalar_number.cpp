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
    return std::complex<double>(static_cast<double>(v.num) /
                                    static_cast<double>(v.den),
                                0.0);
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
          return op(static_cast<std::int64_t>(x),
                    static_cast<std::int64_t>(y));
        }
      },
      a, b);
}

// Rational arithmetic helpers
rational_t rat_add(rational_t a, rational_t b) {
  // a.num/a.den + b.num/b.den = (a.num*b.den + b.num*a.den) / (a.den*b.den)
  auto num = a.num * b.den + b.num * a.den;
  auto den = a.den * b.den;
  auto g = std::gcd(std::abs(num), std::abs(den));
  return {num / g, den / g};
}

rational_t rat_sub(rational_t a, rational_t b) {
  auto num = a.num * b.den - b.num * a.den;
  auto den = a.den * b.den;
  auto g = std::gcd(std::abs(num), std::abs(den));
  return {num / g, den / g};
}

rational_t rat_mul(rational_t a, rational_t b) {
  // Cross-cancel before multiplying to avoid overflow
  auto g1 = std::gcd(std::abs(a.num), std::abs(b.den));
  auto g2 = std::gcd(std::abs(b.num), std::abs(a.den));
  auto num = (a.num / g1) * (b.num / g2);
  auto den = (a.den / g2) * (b.den / g1);
  if (den < 0) {
    num = -num;
    den = -den;
  }
  return {num, den};
}

rational_t rat_div(rational_t a, rational_t b) {
  return rat_mul(a, {b.den, b.num});
}

} // anonymous namespace

// ─── Arithmetic ──────────────────────────────────────────────────

scalar_number operator+(scalar_number const &a, scalar_number const &b) {
  return scalar_number(promote_binary(a.v_, b.v_, [](auto x, auto y) {
    using T = std::decay_t<decltype(x)>;
    if constexpr (is_rat_v<T>) {
      return scalar_number::variant_t{rat_add(x, y)};
    } else {
      return scalar_number::variant_t{x + y};
    }
  }));
}

scalar_number operator-(scalar_number const &a, scalar_number const &b) {
  return scalar_number(promote_binary(a.v_, b.v_, [](auto x, auto y) {
    using T = std::decay_t<decltype(x)>;
    if constexpr (is_rat_v<T>) {
      return scalar_number::variant_t{rat_sub(x, y)};
    } else {
      return scalar_number::variant_t{x - y};
    }
  }));
}

scalar_number operator*(scalar_number const &a, scalar_number const &b) {
  return scalar_number(promote_binary(a.v_, b.v_, [](auto x, auto y) {
    using T = std::decay_t<decltype(x)>;
    if constexpr (is_rat_v<T>) {
      return scalar_number::variant_t{rat_mul(x, y)};
    } else {
      return scalar_number::variant_t{x * y};
    }
  }));
}

scalar_number operator/(scalar_number const &a, scalar_number const &b) {
  return scalar_number(promote_binary(a.v_, b.v_, [](auto x, auto y) {
    using T = std::decay_t<decltype(x)>;
    if constexpr (is_rat_v<T>) {
      return scalar_number::variant_t{rat_div(x, y)};
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
          return rational_t{-x.num, x.den};
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
          // Cross-multiply: a.num/a.den < b.num/b.den
          // Since den > 0, this is: a.num * b.den < b.num * a.den
          return x.num * y.den < y.num * x.den;
        } else {
          return x < y;
        }
      },
      a.v_);
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

} // namespace numsim::cas
