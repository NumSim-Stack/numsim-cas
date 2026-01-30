#ifndef SCALAR_NUMBER_H
#define SCALAR_NUMBER_H

#include <complex>
#include <cstdint>
#include <type_traits>
#include <variant>

namespace numsim::cas {

class scalar_number {
public:
  using variant_t = std::variant<std::int64_t, double, std::complex<double>>;

  scalar_number() : v_(double{0}) {}
  scalar_number(std::int64_t v) : v_(v) {}
  scalar_number(double v) : v_(v) {}
  scalar_number(std::complex<double> v) : v_(v) {}

  template <class T>
  requires(std::is_integral_v<T> && !std::is_same_v<T, bool>)
  scalar_number(T v) : v_(static_cast<std::int64_t>(v)) {}

  template <class T>
  requires(std::is_floating_point_v<T>)
  scalar_number(T v) : v_(static_cast<double>(v)) {}

  variant_t const &raw() const noexcept { return v_; }

  friend scalar_number operator+(scalar_number const &a,
                                 scalar_number const &b) {
    return scalar_number(
        promote_binary(a.v_, b.v_, [](auto x, auto y) { return x + y; }));
  }
  friend scalar_number operator-(scalar_number const &a,
                                 scalar_number const &b) {
    return scalar_number(
        promote_binary(a.v_, b.v_, [](auto x, auto y) { return x - y; }));
  }
  friend scalar_number operator*(scalar_number const &a,
                                 scalar_number const &b) {
    return scalar_number(
        promote_binary(a.v_, b.v_, [](auto x, auto y) { return x * y; }));
  }
  friend scalar_number operator/(scalar_number const &a,
                                 scalar_number const &b) {
    return scalar_number(
        promote_binary(a.v_, b.v_, [](auto x, auto y) { return x / y; }));
  }

  friend scalar_number operator-(scalar_number const &a) {
    return scalar_number(
        std::visit([](auto const &x) -> variant_t { return -x; }, a.v_));
  }

  friend std::ostream &operator<<(std::ostream &os, scalar_number const &a) {
    std::visit([&](auto &val) { os << val; }, a.v_);
    return os;
  }

  friend bool operator==(scalar_number const &a, scalar_number const &b) {
    return a.v_ == b.v_;
  }

  // Optional: strict ordering (lexicographic) so you can use it in maps/sets.
  friend bool operator<(scalar_number const &a, scalar_number const &b) {
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
  [[nodiscard]] scalar_number abs() const noexcept {
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
            return scalar_number(std::abs(
                x)); // magnitude -> double -> stored as scalar_number(double)
          }
        },
        v_);
  }

private:
  explicit scalar_number(variant_t vv) : v_(std::move(vv)) {}

  template <class T>
  static constexpr bool is_cplx_v =
      std::is_same_v<std::decay_t<T>, std::complex<double>>;

  template <class T> static std::complex<double> to_complex(T const &v) {
    if constexpr (is_cplx_v<T>) {
      return v;
    } else {
      return std::complex<double>(static_cast<double>(v), 0.0);
    }
  }

  template <class Op>
  static variant_t promote_binary(variant_t const &a, variant_t const &b,
                                  Op op) {
    return std::visit(
        [&](auto const &x, auto const &y) -> variant_t {
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
            return op(static_cast<std::int64_t>(x),
                      static_cast<std::int64_t>(y));
          }
        },
        a, b);
  }

private:
  variant_t v_;
};

} // namespace numsim::cas

#endif // SCALAR_NUMBER_H
