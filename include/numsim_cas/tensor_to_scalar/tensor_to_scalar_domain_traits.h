#ifndef TENSOR_TO_SCALAR_DOMAIN_TRAITS_H
#define TENSOR_TO_SCALAR_DOMAIN_TRAITS_H

#include <numsim_cas/core/domain_traits.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>

namespace numsim::cas {

template <> struct domain_traits<tensor_to_scalar_expression> {
  using expression_type = tensor_to_scalar_expression;
  using expr_holder_t = expression_holder<tensor_to_scalar_expression>;
  using add_type = tensor_to_scalar_add;
  using mul_type = tensor_to_scalar_mul;
  using negative_type = tensor_to_scalar_negative;
  using zero_type = tensor_to_scalar_zero;
  using one_type = tensor_to_scalar_one;
  using constant_type = tensor_to_scalar_scalar_wrapper;
  using pow_type = tensor_to_scalar_pow;
  using symbol_type = void; // no single symbol type
  using visitable_t = tensor_to_scalar_visitable_t;
  using visitor_return_expr_t = tensor_to_scalar_visitor_return_expr_t;
  static expr_holder_t zero() { return make_expression<tensor_to_scalar_zero>(); }
  static expr_holder_t one() { return make_expression<tensor_to_scalar_one>(); }
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_DOMAIN_TRAITS_H
