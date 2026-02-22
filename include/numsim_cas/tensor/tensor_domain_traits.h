#ifndef TENSOR_DOMAIN_TRAITS_H
#define TENSOR_DOMAIN_TRAITS_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/domain_traits.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <optional>

namespace numsim::cas {

template <> struct domain_traits<tensor_expression> {
  using expression_type = tensor_expression;
  using expr_holder_t = expression_holder<tensor_expression>;
  using add_type = tensor_add;
  using mul_type = void; // tensor_scalar_mul is not n_ary_tree-based
  using negative_type = tensor_negative;
  using zero_type = tensor_zero;
  using one_type = void;      // no generic tensor one
  using constant_type = void; // no tensor constant type
  using pow_type = tensor_pow;
  using symbol_type = tensor;
  using visitable_t = tensor_visitable_t;
  using visitor_return_expr_t = tensor_visitor_return_expr_t;

  static expr_holder_t zero(expr_holder_t const &ref) {
    return make_expression<tensor_zero>(ref.get().dim(), ref.get().rank());
  }

  static std::optional<scalar_number> try_numeric(expr_holder_t const &) {
    return std::nullopt;
  }
};

static_assert(basic_expression_domain<tensor_expression>);

} // namespace numsim::cas

#endif // TENSOR_DOMAIN_TRAITS_H
