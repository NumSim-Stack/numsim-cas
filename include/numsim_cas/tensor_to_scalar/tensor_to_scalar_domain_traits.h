#ifndef TENSOR_TO_SCALAR_DOMAIN_TRAITS_H
#define TENSOR_TO_SCALAR_DOMAIN_TRAITS_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/domain_traits.h>
#include <numsim_cas/scalar/scalar_domain_traits.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>
#include <optional>

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

  static std::optional<scalar_number> try_numeric(expr_holder_t const &expr) {
    if (!expr.is_valid())
      return std::nullopt;
    if (is_same<tensor_to_scalar_zero>(expr))
      return scalar_number{0};
    if (is_same<tensor_to_scalar_one>(expr))
      return scalar_number{1};
    if (is_same<tensor_to_scalar_scalar_wrapper>(expr)) {
      return domain_traits<scalar_expression>::try_numeric(
          expr.template get<tensor_to_scalar_scalar_wrapper>().expr());
    }
    if (is_same<tensor_to_scalar_negative>(expr)) {
      auto inner = try_numeric(
          expr.template get<tensor_to_scalar_negative>().expr());
      if (inner)
        return -(*inner);
    }
    return std::nullopt;
  }

  static expr_holder_t make_constant(scalar_number const &value) {
    return make_expression<tensor_to_scalar_scalar_wrapper>(
        make_expression<scalar_constant>(value));
  }
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_DOMAIN_TRAITS_H
