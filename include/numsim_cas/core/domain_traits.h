#ifndef DOMAIN_TRAITS_H
#define DOMAIN_TRAITS_H

#include <concepts>
#include <numsim_cas/core/scalar_number.h>
#include <numsim_cas/numsim_cas_type_traits.h>
#include <optional>

namespace numsim::cas {

template <typename Domain> struct domain_traits;

/// Compile-time interface validation for domain_traits specializations.
///
/// Without these concepts, a domain that forgets to define a required type
/// alias (e.g. add_type) or misspells a static method (e.g. try_numeric)
/// produces a cryptic template error deep inside a generic simplifier
/// algorithm. The concepts catch these mistakes at the point of
/// instantiation with a clear "constraint not satisfied" diagnostic.
///
/// Two-level hierarchy:
///   basic_expression_domain    — minimal interface used by add_dispatch
///                                and print helpers; all three domains
///                                (scalar, tensor, tensor_to_scalar) satisfy.
///   arithmetic_expression_domain — full arithmetic interface (mul, one,
///                                  constant, make_constant); scalar and
///                                  tensor_to_scalar satisfy; tensor does
///                                  not (it has no n_ary mul or constants).

/// All three domains (scalar, tensor_to_scalar, tensor) satisfy this.
template <typename Domain>
concept basic_expression_domain = requires {
  typename domain_traits<Domain>::expression_type;
  typename domain_traits<Domain>::expr_holder_t;
  typename domain_traits<Domain>::add_type;
  typename domain_traits<Domain>::mul_type;
  typename domain_traits<Domain>::negative_type;
  typename domain_traits<Domain>::zero_type;
  typename domain_traits<Domain>::one_type;
  typename domain_traits<Domain>::constant_type;
  typename domain_traits<Domain>::pow_type;
  typename domain_traits<Domain>::symbol_type;
  typename domain_traits<Domain>::visitable_t;
  typename domain_traits<Domain>::visitor_return_expr_t;
} && requires(typename domain_traits<Domain>::expr_holder_t const &expr) {
  {
    domain_traits<Domain>::try_numeric(expr)
  } -> std::same_as<std::optional<scalar_number>>;
  {
    domain_traits<Domain>::zero(expr)
  } -> std::same_as<typename domain_traits<Domain>::expr_holder_t>;
};

/// Scalar and tensor_to_scalar satisfy this; tensor does not.
template <typename Domain>
concept arithmetic_expression_domain =
    basic_expression_domain<Domain> &&
    !std::is_void_v<typename domain_traits<Domain>::mul_type> &&
    !std::is_void_v<typename domain_traits<Domain>::one_type> &&
    !std::is_void_v<typename domain_traits<Domain>::constant_type> && requires {
      {
        domain_traits<Domain>::zero()
      } -> std::same_as<typename domain_traits<Domain>::expr_holder_t>;
      {
        domain_traits<Domain>::one()
      } -> std::same_as<typename domain_traits<Domain>::expr_holder_t>;
    } && requires(scalar_number const &val) {
      {
        domain_traits<Domain>::make_constant(val)
      } -> std::same_as<typename domain_traits<Domain>::expr_holder_t>;
    };

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
