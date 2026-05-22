#ifndef SCALAR_NUMBER_H
#define SCALAR_NUMBER_H

#include <numsim_cas/core/hash_functions.h>

#include <complex>
#include <cstdint>
#include <iosfwd>
#include <numeric>
#include <optional>
#include <type_traits>
#include <variant>

namespace numsim::cas {

/// Exact rational number: always GCD-reduced with den > 0.
struct rational_t {
  std::int64_t num;
  std::int64_t den;
};

inline void hash_combine(std::size_t &seed, rational_t const &value) {
  hash_combine(seed, value.num);
  hash_combine(seed, value.den);
}

class scalar_number {
public:
  using variant_t =
      std::variant<std::int64_t, double, std::complex<double>, rational_t>;

  scalar_number() : v_(double{0}) {}
  scalar_number(std::int64_t v) : v_(v) {}
  scalar_number(double v) : v_(v) {}
  scalar_number(std::complex<double> v) : v_(v) {}

  /// Construct a rational. Normalizes via GCD; den==1 stores as int64_t.
  scalar_number(std::int64_t num, std::int64_t den);
  scalar_number(rational_t r);

  template <class T>
  requires(std::is_integral_v<T> && !std::is_same_v<T, bool>)
  scalar_number(T v) : v_(static_cast<std::int64_t>(v)) {}

  template <class T>
  requires(std::is_floating_point_v<T>)
  scalar_number(T v) : v_(static_cast<double>(v)) {}

  variant_t const &raw() const noexcept { return v_; }

  friend scalar_number operator+(scalar_number const &a,
                                 scalar_number const &b);
  friend scalar_number operator-(scalar_number const &a,
                                 scalar_number const &b);
  friend scalar_number operator*(scalar_number const &a,
                                 scalar_number const &b);
  friend scalar_number operator/(scalar_number const &a,
                                 scalar_number const &b);
  friend scalar_number operator-(scalar_number const &a);
  friend std::ostream &operator<<(std::ostream &os, scalar_number const &a);
  friend bool operator==(scalar_number const &a, scalar_number const &b);
  friend bool operator<(scalar_number const &a, scalar_number const &b);

  /// Numeric strict-less for comparison-operator folding.
  ///
  /// Unlike `operator<` (which is a total order for sort containers, with
  /// rank-lexicographic ordering across variant alternatives), this compares
  /// values numerically after cross-rank promotion — so `int(3) < double(3.0)`
  /// is `false`.
  ///
  /// **Known limitations** (locked in by tests in ScalarComparisonTest.h):
  ///
  /// - **Complex pairs are ordered real-then-imag** for totality. This is
  ///   *not* a mathematically meaningful ordering — complex numbers have no
  ///   natural order. The behaviour exists so the comparison operators can
  ///   produce a deterministic answer on complex inputs (which shouldn't
  ///   normally appear in constitutive-modelling use cases). See #144.
  ///
  /// - **NaN inputs are not handled per IEEE-754**. The structural-identity
  ///   fold in `make_comparison` (scalar_std.h) treats two `scalar_constant`
  ///   nodes holding NaN as equal, since they hash identically — so
  ///   `eq(NaN, NaN)` folds to 1 instead of 0. The CAS does not validate
  ///   NaN at construction; if your inputs can be NaN, do not rely on
  ///   comparison-fold semantics. See #143.
  ///
  /// Both behaviours are documented contracts; tightening either (e.g.
  /// refusing NaN at `make_scalar_constant`, asserting on complex inputs)
  /// would be a deliberate API change.
  friend bool numeric_less(scalar_number const &a, scalar_number const &b);

  [[nodiscard]] scalar_number abs() const noexcept;

  /// Exact pow for integer exponents. Returns nullopt for non-integer
  /// exponents.
  friend std::optional<scalar_number> pow(scalar_number const &base,
                                          scalar_number const &exp);

private:
  explicit scalar_number(variant_t vv) : v_(std::move(vv)) {
    if (auto *r = std::get_if<rational_t>(&v_)) {
      if (r->den == 1)
        v_ = r->num;
    }
  }

  variant_t v_;
};

} // namespace numsim::cas

#endif // SCALAR_NUMBER_H
