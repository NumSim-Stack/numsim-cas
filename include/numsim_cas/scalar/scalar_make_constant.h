#ifndef SCALAR_MAKE_CONSTANT_H
#define SCALAR_MAKE_CONSTANT_H

#include <numsim_cas/core/make_constant.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/scalar/scalar_globals.h>

namespace numsim::cas::detail {
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
  // TODO std::complex support
  if (v < 0) {
    return -make_expression<scalar_constant>(std::abs(static_cast<V>(v)));
  }
  return make_expression<scalar_constant>(static_cast<V>(v));
}

} // namespace numsim::cas::detail

#endif // SCALAR_MAKE_CONSTANT_H
