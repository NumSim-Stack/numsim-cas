#include <numsim_cas/core/scalar_number.h>

#include <cmath>
#include <limits>
#include <ostream>

namespace numsim::cas {

namespace {

template <class T>
constexpr bool is_cplx_v =
    std::is_same_v<std::decay_t<T>, std::complex<double>>;

template <class T> std::complex<double> to_complex(T const &v) {
  if constexpr (is_cplx_v<T>) {
    return v;
  } else {
    return std::complex<double>(static_cast<double>(v), 0.0);
  }
}

template <class Op>
scalar_number::variant_t promote_binary(scalar_number::variant_t const &a,
                                        scalar_number::variant_t const &b,
                                        Op op) {
  return std::visit(
      [&](auto const &x, auto const &y) -> scalar_number::variant_t {
        using X = std::decay_t<decltype(x)>;
        using Y = std::decay_t<decltype(y)>;

        // promote to complex if needed
        if constexpr (std::is_same_v<X, std::complex<double>> ||
                      std::is_same_v<Y, std::complex<double>>) {
          auto cx = to_complex(x);
          auto cy = to_complex(y);
          return op(cx, cy);
        }
        // promote to double if needed
        else if constexpr (std::is_same_v<X, double> ||
                           std::is_same_v<Y, double>) {
          return op(static_cast<double>(x), static_cast<double>(y));
        }
        // otherwise int64
        else {
          return op(static_cast<std::int64_t>(x), static_cast<std::int64_t>(y));
        }
      },
      a, b);
}

} // anonymous namespace

scalar_number operator+(scalar_number const &a, scalar_number const &b) {
  return scalar_number(
      promote_binary(a.v_, b.v_, [](auto x, auto y) { return x + y; }));
}

scalar_number operator-(scalar_number const &a, scalar_number const &b) {
  return scalar_number(
      promote_binary(a.v_, b.v_, [](auto x, auto y) { return x - y; }));
}

scalar_number operator*(scalar_number const &a, scalar_number const &b) {
  return scalar_number(
      promote_binary(a.v_, b.v_, [](auto x, auto y) { return x * y; }));
}

scalar_number operator/(scalar_number const &a, scalar_number const &b) {
  return scalar_number(
      promote_binary(a.v_, b.v_, [](auto x, auto y) { return x / y; }));
}

scalar_number operator-(scalar_number const &a) {
  return scalar_number(std::visit(
      [](auto const &x) -> scalar_number::variant_t { return -x; }, a.v_));
}

std::ostream &operator<<(std::ostream &os, scalar_number const &a) {
  std::visit([&](auto &val) { os << val; }, a.v_);
  return os;
}

bool operator==(scalar_number const &a, scalar_number const &b) {
  return std::visit(
      [](auto const &x, auto const &y) -> bool {
        using X = std::decay_t<decltype(x)>;
        using Y = std::decay_t<decltype(y)>;
        if constexpr (std::is_same_v<X, Y>) {
          return x == y;
        } else if constexpr (std::is_same_v<X, std::complex<double>> ||
                             std::is_same_v<Y, std::complex<double>>) {
          return to_complex(x) == to_complex(y);
        } else {
          return static_cast<double>(x) == static_cast<double>(y);
        }
      },
      a.v_, b.v_);
}

bool operator<(scalar_number const &a, scalar_number const &b) {
  if (a.v_.index() != b.v_.index())
    return a.v_.index() < b.v_.index();
  return std::visit(
      [&](auto const &x) {
        using X = std::decay_t<decltype(x)>;
        auto const &y = std::get<X>(b.v_);
        if constexpr (std::is_same_v<X, std::complex<double>>) {
          if (x.real() != y.real())
            return x.real() < y.real();
          return x.imag() < y.imag();
        } else {
          return x < y;
        }
      },
      a.v_);
}

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
        } else { // std::complex<double>
          return scalar_number(std::abs(x));
        }
      },
      v_);
}

} // namespace numsim::cas
