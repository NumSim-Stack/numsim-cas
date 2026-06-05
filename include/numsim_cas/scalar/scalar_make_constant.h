#ifndef SCALAR_MAKE_CONSTANT_H
#define SCALAR_MAKE_CONSTANT_H

#include <numsim_cas/core/make_constant.h>
#include <numsim_cas/core/scalar_number.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/scalar/scalar_globals.h>

namespace numsim::cas::detail {

// scalar_number → canonical scalar_expression holder. Shared by the
// simplifier coefficient-collapse paths (via `scalar_traits::make_constant`)
// AND by the arithmetic-literal overload below, so both routes produce
// structurally identical nodes for the same value (#184). Without this
// the simplifier would build raw `scalar_constant{1}` for sums collapsing
// to one while the public API returned the `scalar_one` singleton.
inline expression_holder<scalar_expression>
tag_invoke(make_constant_fn, std::type_identity<scalar_expression>,
           scalar_number const &v) {
  if (v == scalar_number{0})
    return get_scalar_zero();
  if (v == scalar_number{1})
    return get_scalar_one();
  if (v == scalar_number{-1})
    return -get_scalar_one();
  if (numeric_less(v, scalar_number{0})) {
    return -make_expression<scalar_constant>(-v);
  }
  return make_expression<scalar_constant>(v);
}

// arithmetic -> scalar_constant
template <class T>
requires std::is_arithmetic_v<std::remove_cvref_t<T>>
expression_holder<scalar_expression>
tag_invoke(make_constant_fn, std::type_identity<scalar_expression>, T &&v) {
  // adapt to your actual scalar number type:
  using V = std::remove_cvref_t<T>;
  if constexpr (std::is_integral_v<T>) { // NOLINT(bugprone-branch-clone)
    if (v == 0)
      return get_scalar_zero();
    if (v == 1)
      return get_scalar_one();
    if (v == -1)
      return -get_scalar_one();
  } else {
    // Floating: treat *exact* 0.0 and 1.0 as special
    if (v == 0)
      return get_scalar_zero();
    if (v == 1)
      return get_scalar_one();
    if (v == -1)
      return -get_scalar_one();
  }
  // TODO(#267): std::complex support
  if (v < 0) {
    return -make_expression<scalar_constant>(std::abs(static_cast<V>(v)));
  }
  return make_expression<scalar_constant>(static_cast<V>(v));
}

} // namespace numsim::cas::detail

#endif // SCALAR_MAKE_CONSTANT_H
