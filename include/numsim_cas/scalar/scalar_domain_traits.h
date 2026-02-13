#ifndef SCALAR_DOMAIN_TRAITS_H
#define SCALAR_DOMAIN_TRAITS_H

#include <numsim_cas/core/domain_traits.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_globals.h>

namespace numsim::cas {

template <> struct domain_traits<scalar_expression> {
  using expression_type = scalar_expression;
  using expr_holder_t = expression_holder<scalar_expression>;
  using add_type = scalar_add;
  using mul_type = scalar_mul;
  using negative_type = scalar_negative;
  using zero_type = scalar_zero;
  using one_type = scalar_one;
  using constant_type = scalar_constant;
  using pow_type = scalar_pow;
  using symbol_type = scalar;
  using visitable_t = scalar_visitable_t;
  using visitor_return_expr_t = scalar_visitor_return_expr_t;
  static expr_holder_t zero() { return get_scalar_zero(); }
  static expr_holder_t one() { return get_scalar_one(); }
};

} // namespace numsim::cas

#endif // SCALAR_DOMAIN_TRAITS_H
