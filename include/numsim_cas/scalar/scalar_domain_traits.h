#ifndef SCALAR_DOMAIN_TRAITS_H
#define SCALAR_DOMAIN_TRAITS_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/domain_traits.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_globals.h>
#include <optional>

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
  static expr_holder_t zero(expr_holder_t const &) { return zero(); }
  static expr_holder_t one() { return get_scalar_one(); }

  static std::optional<scalar_number> try_numeric(expr_holder_t const &expr) {
    if (!expr.is_valid())
      return std::nullopt;
    if (is_same<scalar_zero>(expr))
      return scalar_number{0};
    if (is_same<scalar_one>(expr))
      return scalar_number{1};
    if (is_same<scalar_constant>(expr))
      return expr.template get<scalar_constant>().value();
    if (is_same<scalar_negative>(expr)) {
      auto inner = try_numeric(expr.template get<scalar_negative>().expr());
      if (inner)
        return -(*inner);
    }
    return std::nullopt;
  }

  static expr_holder_t make_constant(scalar_number const &value) {
    return make_expression<scalar_constant>(value);
  }
};

static_assert(arithmetic_expression_domain<scalar_expression>);

} // namespace numsim::cas

#endif // SCALAR_DOMAIN_TRAITS_H
