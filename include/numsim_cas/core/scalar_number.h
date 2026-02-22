#ifndef SCALAR_NUMBER_H
#define SCALAR_NUMBER_H

#include <numsim_cas/core/hash_functions.h>

#include <complex>
#include <cstdint>
#include <iosfwd>
#include <numeric>
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

  [[nodiscard]] scalar_number abs() const noexcept;

private:
  explicit scalar_number(variant_t vv) : v_(vv) {}

  variant_t v_;
};

} // namespace numsim::cas

#endif // SCALAR_NUMBER_H
