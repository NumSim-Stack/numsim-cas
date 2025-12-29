#ifndef SCALAR_STD_H
#define SCALAR_STD_H

#include "../functions.h"
#include "../numsim_cas_forward.h"
#include "../numsim_cas_type_traits.h"
#include "scalar_expression.h"
#include "scalar_one.h"
#include "visitors/scalar_printer.h"
#include <sstream>

namespace std {

template <typename ValueType>
auto to_string(
    numsim::cas::expression_holder<numsim::cas::scalar_expression<ValueType>>
        &&expr) {
  std::stringstream ss;
  numsim::cas::scalar_printer<ValueType, std::stringstream> printer(ss);
  printer.apply(expr);
  return ss.str();
}

template <typename ValueType>
auto to_string(
    numsim::cas::expression_holder<numsim::cas::scalar_expression<ValueType>>
        &expr) {
  std::stringstream ss;
  numsim::cas::scalar_printer<ValueType, std::stringstream> printer(ss);
  printer.apply(expr);
  return ss.str();
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<
        std::is_base_of_v<
            numsim::cas::scalar_expression<
                typename numsim::cas::remove_cvref_t<ExprLHS>::value_type>,
            typename numsim::cas::remove_cvref_t<ExprLHS>::expr_type>,
        bool> = true,
    std::enable_if_t<
        std::is_base_of_v<
            numsim::cas::scalar_expression<
                typename numsim::cas::remove_cvref_t<ExprRHS>::value_type>,
            typename numsim::cas::remove_cvref_t<ExprRHS>::expr_type>,
        bool> = true>
auto pow(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs) {
  using value_type_rhs =
      typename numsim::cas::remove_cvref_t<ExprRHS>::expr_type::value_type;
  using value_type = std::common_type_t<
      typename numsim::cas::remove_cvref_t<ExprLHS>::expr_type::value_type,
      value_type_rhs>;

  assert(expr_rhs.is_valid());
  assert(expr_lhs.is_valid());

  if (numsim::cas::is_same<numsim::cas::scalar_one<value_type_rhs>>(expr_rhs)) {
    return std::forward<ExprLHS>(expr_lhs);
  }

  if (numsim::cas::is_same<numsim::cas::scalar_constant<value_type_rhs>>(
          expr_rhs) &&
      expr_rhs.template get<numsim::cas::scalar_constant<value_type_rhs>>()() ==
          static_cast<value_type_rhs>(1)) {
    return std::forward<ExprLHS>(expr_lhs);
  }

  return numsim::cas::make_expression<numsim::cas::scalar_pow<value_type>>(
      std::forward<ExprLHS>(expr_lhs), std::forward<ExprRHS>(expr_rhs));
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<
        std::is_base_of_v<
            numsim::cas::scalar_expression<
                typename numsim::cas::remove_cvref_t<ExprLHS>::value_type>,
            typename numsim::cas::remove_cvref_t<ExprLHS>::expr_type>,
        bool> = true,
    std::enable_if_t<std::is_arithmetic_v<ExprRHS>, bool> = true>
auto pow(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs) {
  using value_type = std::common_type_t<
      typename numsim::cas::remove_cvref_t<ExprLHS>::expr_type::value_type,
      ExprRHS>;

  assert(expr_lhs.is_valid());

  if (expr_rhs == static_cast<value_type>(1)) {
    return std::forward<ExprLHS>(expr_lhs);
  }

  auto constant{
      numsim::cas::make_expression<numsim::cas::scalar_constant<value_type>>(
          expr_rhs)};
  return numsim::cas::make_expression<numsim::cas::scalar_pow<value_type>>(
      std::forward<ExprLHS>(expr_lhs), std::move(constant));
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<
        std::is_base_of_v<
            numsim::cas::scalar_expression<
                typename numsim::cas::remove_cvref_t<ExprLHS>::value_type>,
            typename numsim::cas::remove_cvref_t<ExprLHS>::expr_type>,
        bool> = true,
    std::enable_if_t<std::is_fundamental_v<ExprRHS>, bool> = true>
auto pow(ExprLHS const &expr_lhs, ExprRHS &&expr_rhs) {
  using value_type = std::common_type_t<
      typename numsim::cas::remove_cvref_t<ExprLHS>::expr_type::value_type,
      ExprRHS>;

  assert(expr_lhs.is_valid());

  if (expr_rhs == static_cast<value_type>(1)) {
    return expr_lhs;
  }

  auto constant{
      numsim::cas::make_expression<numsim::cas::scalar_constant<value_type>>(
          expr_rhs)};
  return numsim::cas::make_expression<numsim::cas::scalar_pow<value_type>>(
      expr_lhs, std::move(constant));
}

template <typename Expr,
          std::enable_if_t<
              std::is_same_v<typename std::decay_t<Expr>::expr_type,
                             numsim::cas::scalar_expression<
                                 typename std::decay_t<Expr>::value_type>>,
              bool> = true>
auto sin(Expr &&expr) {
  using value_type = typename std::decay_t<Expr>::value_type;
  return numsim::cas::make_expression<numsim::cas::scalar_sin<value_type>>(
      std::forward<Expr>(expr));
}

template <typename Expr,
          std::enable_if_t<
              std::is_same_v<typename std::decay_t<Expr>::expr_type,
                             numsim::cas::scalar_expression<
                                 typename std::decay_t<Expr>::value_type>>,
              bool> = true>
auto cos(Expr &&expr) {
  using value_type =
      typename numsim::cas::remove_cvref_t<Expr>::expr_type::value_type;
  return numsim::cas::make_expression<numsim::cas::scalar_cos<value_type>>(
      std::forward<Expr>(expr));
}

template <typename Expr,
          std::enable_if_t<
              std::is_same_v<typename std::decay_t<Expr>::expr_type,
                             numsim::cas::scalar_expression<
                                 typename std::decay_t<Expr>::value_type>>,
              bool> = true>
auto tan(Expr &&expr) {
  using value_type =
      typename numsim::cas::remove_cvref_t<Expr>::expr_type::value_type;
  return numsim::cas::make_expression<numsim::cas::scalar_tan<value_type>>(
      std::forward<Expr>(expr));
}

template <typename Expr,
          std::enable_if_t<
              std::is_same_v<typename std::decay_t<Expr>::expr_type,
                             numsim::cas::scalar_expression<
                                 typename std::decay_t<Expr>::value_type>>,
              bool> = true>
auto asin(Expr &&expr) {
  using value_type = typename std::decay_t<Expr>::value_type;
  return numsim::cas::make_expression<numsim::cas::scalar_asin<value_type>>(
      std::forward<Expr>(expr));
}

template <typename Expr,
          std::enable_if_t<
              std::is_same_v<typename std::decay_t<Expr>::expr_type,
                             numsim::cas::scalar_expression<
                                 typename std::decay_t<Expr>::value_type>>,
              bool> = true>
auto acos(Expr &&expr) {
  using value_type =
      typename numsim::cas::remove_cvref_t<Expr>::expr_type::value_type;
  return numsim::cas::make_expression<numsim::cas::scalar_acos<value_type>>(
      std::forward<Expr>(expr));
}

template <typename Expr,
          std::enable_if_t<
              std::is_same_v<typename std::decay_t<Expr>::expr_type,
                             numsim::cas::scalar_expression<
                                 typename std::decay_t<Expr>::value_type>>,
              bool> = true>
auto atan(Expr &&expr) {
  using value_type =
      typename numsim::cas::remove_cvref_t<Expr>::expr_type::value_type;
  return numsim::cas::make_expression<numsim::cas::scalar_atan<value_type>>(
      std::forward<Expr>(expr));
}

template <typename Expr,
          std::enable_if_t<
              std::is_same_v<typename std::decay_t<Expr>::expr_type,
                             numsim::cas::scalar_expression<
                                 typename std::decay_t<Expr>::value_type>>,
              bool> = true>
auto exp(Expr &&expr) {
  using value_type =
      typename numsim::cas::remove_cvref_t<Expr>::expr_type::value_type;
  return numsim::cas::make_expression<numsim::cas::scalar_exp<value_type>>(
      std::forward<Expr>(expr));
}

template <typename Expr,
          std::enable_if_t<
              std::is_same_v<typename std::decay_t<Expr>::expr_type,
                             numsim::cas::scalar_expression<
                                 typename std::decay_t<Expr>::value_type>>,
              bool> = true>
auto abs(Expr &&expr) {
  using value_type =
      typename numsim::cas::remove_cvref_t<Expr>::expr_type::value_type;
  return numsim::cas::make_expression<numsim::cas::scalar_abs<value_type>>(
      std::forward<Expr>(expr));
}

template <typename Expr,
          std::enable_if_t<
              std::is_same_v<typename std::decay_t<Expr>::expr_type,
                             numsim::cas::scalar_expression<
                                 typename std::decay_t<Expr>::value_type>>,
              bool> = true>
auto sqrt(Expr &&expr) {
  using value_type =
      typename numsim::cas::remove_cvref_t<Expr>::expr_type::value_type;
  return numsim::cas::make_expression<numsim::cas::scalar_sqrt<value_type>>(
      std::forward<Expr>(expr));
}

template <typename Expr,
          std::enable_if_t<
              std::is_same_v<typename std::decay_t<Expr>::expr_type,
                             numsim::cas::scalar_expression<
                                 typename std::decay_t<Expr>::value_type>>,
              bool> = true>
auto sign(Expr &&expr) {
  using value_type =
      typename numsim::cas::remove_cvref_t<Expr>::expr_type::value_type;
  return numsim::cas::make_expression<numsim::cas::scalar_sign<value_type>>(
      std::forward<Expr>(expr));
}

template <typename Expr,
          std::enable_if_t<
              std::is_same_v<typename std::decay_t<Expr>::expr_type,
                             numsim::cas::scalar_expression<
                                 typename std::decay_t<Expr>::value_type>>,
              bool> = true>
auto log(Expr &&expr) {
  using value_type =
      typename numsim::cas::remove_cvref_t<Expr>::expr_type::value_type;
  return numsim::cas::make_expression<numsim::cas::scalar_log<value_type>>(
      std::forward<Expr>(expr));
}
} // namespace std

#endif // SCALAR_STD_H
