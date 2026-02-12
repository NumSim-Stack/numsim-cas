#ifndef SCALAR_STD_H
#define SCALAR_STD_H

#include <cassert>
#include <sstream>
#include <string>
#include <type_traits>

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/scalar/scalar_abs.h>
#include <numsim_cas/scalar/scalar_acos.h>
#include <numsim_cas/scalar/scalar_asin.h>
#include <numsim_cas/scalar/scalar_atan.h>
#include <numsim_cas/scalar/scalar_binary_simplify_fwd.h>
#include <numsim_cas/scalar/scalar_constant.h>
#include <numsim_cas/scalar/scalar_cos.h>
#include <numsim_cas/scalar/scalar_exp.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/scalar/scalar_globals.h>
#include <numsim_cas/scalar/scalar_io.h>
#include <numsim_cas/scalar/scalar_log.h>
#include <numsim_cas/scalar/scalar_make_constant.h>
#include <numsim_cas/scalar/scalar_one.h>
#include <numsim_cas/scalar/scalar_power.h>
#include <numsim_cas/scalar/scalar_sign.h>
#include <numsim_cas/scalar/scalar_sin.h>
#include <numsim_cas/scalar/scalar_sqrt.h>
#include <numsim_cas/scalar/scalar_tan.h>

namespace numsim::cas {

template <class T> using decay_t = std::remove_cvref_t<T>;

template <class T>
concept scalar_expr_holder = requires {
  typename decay_t<T>::expr_type;
} && std::is_base_of_v<scalar_expression, typename decay_t<T>::expr_type>;

std::string to_string(expression_holder<scalar_expression> const &expr);

template <scalar_expr_holder L, scalar_expr_holder R>
auto pow(L &&expr_lhs, R &&expr_rhs) {
  assert(expr_rhs.is_valid());
  assert(expr_lhs.is_valid());

  if (is_same<scalar_one>(expr_lhs) ||
      (is_same<scalar_constant>(expr_lhs) &&
       expr_lhs.template get<scalar_constant>().value() == 1)) {
    return get_scalar_one();
  }

  if (is_same<scalar_zero>(expr_rhs))
    return get_scalar_one();

  if (is_same<scalar_one>(expr_rhs))
    return std::forward<L>(expr_lhs);

  if (is_same<scalar_constant>(expr_rhs) &&
      expr_rhs.template get<scalar_constant>().value() == 1) {
    return std::forward<L>(expr_lhs);
  }

  return binary_scalar_pow_simplify(std::forward<L>(expr_lhs),
                                    std::forward<R>(expr_rhs));
}

template <scalar_expr_holder L, typename R>
requires std::is_arithmetic_v<std::remove_cvref_t<R>>
auto pow(L &&expr_lhs, R expr_rhs) {
  auto constant{detail::tag_invoke(detail::make_constant_fn{},
                                   std::type_identity<scalar_expression>{},
                                   expr_rhs)};
  return binary_scalar_pow_simplify(std::forward<L>(expr_lhs),
                                    std::move(constant));
}

template <scalar_expr_holder E> auto sin(E &&e) {
  return make_expression<scalar_sin>(std::forward<E>(e));
}
template <scalar_expr_holder E> auto cos(E &&e) {
  return make_expression<scalar_cos>(std::forward<E>(e));
}
template <scalar_expr_holder E> auto tan(E &&e) {
  return make_expression<scalar_tan>(std::forward<E>(e));
}
template <scalar_expr_holder E> auto asin(E &&e) {
  return make_expression<scalar_asin>(std::forward<E>(e));
}
template <scalar_expr_holder E> auto acos(E &&e) {
  return make_expression<scalar_acos>(std::forward<E>(e));
}
template <scalar_expr_holder E> auto atan(E &&e) {
  return make_expression<scalar_atan>(std::forward<E>(e));
}
template <scalar_expr_holder E> auto exp(E &&e) {
  return make_expression<scalar_exp>(std::forward<E>(e));
}
template <scalar_expr_holder E> auto abs(E &&e) {
  return make_expression<scalar_abs>(std::forward<E>(e));
}
template <scalar_expr_holder E> auto sqrt(E &&e) {
  return make_expression<scalar_sqrt>(std::forward<E>(e));
}
template <scalar_expr_holder E> auto sign(E &&e) {
  return make_expression<scalar_sign>(std::forward<E>(e));
}
template <scalar_expr_holder E> auto log(E &&e) {
  return make_expression<scalar_log>(std::forward<E>(e));
}

} // namespace numsim::cas

#endif // SCALAR_STD_H
