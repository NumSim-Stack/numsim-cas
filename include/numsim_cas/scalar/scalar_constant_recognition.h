#ifndef NUMSIM_CAS_SCALAR_CONSTANT_RECOGNITION_H
#define NUMSIM_CAS_SCALAR_CONSTANT_RECOGNITION_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/scalar_number.h>
#include <numsim_cas/scalar/scalar_constant.h>
#include <numsim_cas/scalar/scalar_one.h>
#include <numsim_cas/scalar/scalar_zero.h>

namespace numsim::cas {

// Numeric-constant recognition (#262, Layer A). Is `e` numerically exactly
// 1 / 0 in either representable form — the scalar_one/scalar_zero singleton, or
// a scalar_constant carrying that value? These replace the hand-spelled idiom
//   is_same<scalar_one>(e) || (is_same<scalar_constant>(e) && value() == 1)
// duplicated across the scalar/tensor construction-time simplifiers. Exact
// match to the checks they replace (no negation unwrap — same behavior as
// before). Kept in this leaf header (constant nodes only) so consumers avoid
// the scalar_std ↔ scalar_domain_traits include cycle that delegating to
// domain_traits::try_numeric would introduce.
[[nodiscard]] inline bool
is_constant_one(expression_holder<scalar_expression> const &e) {
  return is_same<scalar_one>(e) ||
         (is_same<scalar_constant>(e) &&
          e.get<scalar_constant>().value() == scalar_number{1});
}

[[nodiscard]] inline bool
is_constant_zero(expression_holder<scalar_expression> const &e) {
  return is_same<scalar_zero>(e) ||
         (is_same<scalar_constant>(e) &&
          e.get<scalar_constant>().value() == scalar_number{0});
}

} // namespace numsim::cas

#endif // NUMSIM_CAS_SCALAR_CONSTANT_RECOGNITION_H
