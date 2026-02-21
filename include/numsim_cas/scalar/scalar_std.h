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
#include <numsim_cas/scalar/scalar_assume.h>
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

[[nodiscard]] std::string
to_string(expression_holder<scalar_expression> const &expr);

template <scalar_expr_holder L, scalar_expr_holder R>
[[nodiscard]] auto pow(L &&expr_lhs, R &&expr_rhs) {
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
[[nodiscard]] auto pow(L &&expr_lhs, R expr_rhs) {
  auto constant{detail::tag_invoke(detail::make_constant_fn{},
                                   std::type_identity<scalar_expression>{},
                                   expr_rhs)};
  return binary_scalar_pow_simplify(std::forward<L>(expr_lhs),
                                    std::move(constant));
}

template <scalar_expr_holder E> [[nodiscard]] auto sin(E &&e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  if (is_same<scalar_asin>(e))
    return e.template get<scalar_asin>().expr();
  return make_expression<scalar_sin>(std::forward<E>(e));
}
template <scalar_expr_holder E> [[nodiscard]] auto cos(E &&e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_one();
  if (is_same<scalar_acos>(e))
    return e.template get<scalar_acos>().expr();
  return make_expression<scalar_cos>(std::forward<E>(e));
}
template <scalar_expr_holder E> [[nodiscard]] auto tan(E &&e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  if (is_same<scalar_atan>(e))
    return e.template get<scalar_atan>().expr();
  return make_expression<scalar_tan>(std::forward<E>(e));
}
template <scalar_expr_holder E> [[nodiscard]] auto asin(E &&e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  return make_expression<scalar_asin>(std::forward<E>(e));
}
template <scalar_expr_holder E> [[nodiscard]] auto acos(E &&e) {
  if (is_same<scalar_one>(e) ||
      (is_same<scalar_constant>(e) &&
       e.template get<scalar_constant>().value() == scalar_number{1}))
    return get_scalar_zero();
  return make_expression<scalar_acos>(std::forward<E>(e));
}
template <scalar_expr_holder E> [[nodiscard]] auto atan(E &&e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  return make_expression<scalar_atan>(std::forward<E>(e));
}
template <scalar_expr_holder E> [[nodiscard]] auto exp(E &&e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_one();
  if (is_same<scalar_log>(e))
    return e.template get<scalar_log>().expr();
  return make_expression<scalar_exp>(std::forward<E>(e));
}
template <scalar_expr_holder E> [[nodiscard]] auto abs(E &&e) {
  if (is_positive(e) || is_nonnegative(e))
    return std::forward<E>(e);
  if (is_negative(e) || is_nonpositive(e))
    return -std::forward<E>(e);
  return make_expression<scalar_abs>(std::forward<E>(e));
}
template <scalar_expr_holder E> [[nodiscard]] auto sqrt(E &&e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  if (is_same<scalar_one>(e) ||
      (is_same<scalar_constant>(e) &&
       e.template get<scalar_constant>().value() == scalar_number{1}))
    return get_scalar_one();
  if (is_same<scalar_pow>(e)) {
    auto const &p = e.template get<scalar_pow>();
    if (is_same<scalar_constant>(p.expr_rhs()) &&
        p.expr_rhs().template get<scalar_constant>().value() ==
            scalar_number{2}) {
      if (is_nonnegative(p.expr_lhs()) || is_positive(p.expr_lhs()))
        return p.expr_lhs();
    }
  }
  return make_expression<scalar_sqrt>(std::forward<E>(e));
}
template <scalar_expr_holder E> [[nodiscard]] auto sign(E &&e) {
  if (is_positive(e))
    return get_scalar_one();
  if (is_negative(e))
    return -get_scalar_one();
  return make_expression<scalar_sign>(std::forward<E>(e));
}
template <scalar_expr_holder E> [[nodiscard]] auto log(E &&e) {
  if (is_same<scalar_one>(e) ||
      (is_same<scalar_constant>(e) &&
       e.template get<scalar_constant>().value() == scalar_number{1}))
    return get_scalar_zero();
  if (is_same<scalar_exp>(e))
    return e.template get<scalar_exp>().expr();
  return make_expression<scalar_log>(std::forward<E>(e));
}

} // namespace numsim::cas

#endif // SCALAR_STD_H
