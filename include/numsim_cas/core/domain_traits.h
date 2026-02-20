#ifndef DOMAIN_TRAITS_H
#define DOMAIN_TRAITS_H

#include <numsim_cas/core/scalar_number.h>
#include <numsim_cas/numsim_cas_type_traits.h>

namespace numsim::cas {

template <typename Domain> struct domain_traits;

template <typename Traits, typename ExprT, typename ValueTypeT>
scalar_number get_coefficient(ExprT const &expr, ValueTypeT const &value) {
  if constexpr (is_detected_v<has_coefficient, ExprT>) {
    auto const &coeff = expr.coeff();
    if (coeff.is_valid()) {
      auto val = Traits::try_numeric(coeff);
      if (val)
        return *val;
    }
    return value;
  }
  return value;
}

} // namespace numsim::cas

#endif // DOMAIN_TRAITS_H
