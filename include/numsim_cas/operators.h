#ifndef OPERATORS_H
#define OPERATORS_H

#include "basic_functions.h"
#include "numsim_cas_forward.h"
#include "numsim_cas_type_traits.h"

namespace numsim::cas {

template <class LHS, class RHS> struct result_expression;

template <class T> struct result_expression<T, T> {
  using type = T;
};

template <class T>
struct result_expression<numsim::cas::scalar_expression<T>, int> {
  using type = numsim::cas::scalar_expression<T>;
};

template <class T>
struct result_expression<numsim::cas::scalar_expression<T>, double> {
  using type = numsim::cas::scalar_expression<T>;
};

template <class T>
struct result_expression<numsim::cas::scalar_expression<T>, float> {
  using type = numsim::cas::scalar_expression<T>;
};

template <class T>
struct result_expression<int, numsim::cas::scalar_expression<T>> {
  using type = numsim::cas::scalar_expression<T>;
};

template <class T>
struct result_expression<double, numsim::cas::scalar_expression<T>> {
  using type = numsim::cas::scalar_expression<T>;
};

template <class T>
struct result_expression<float, numsim::cas::scalar_expression<T>> {
  using type = numsim::cas::scalar_expression<T>;
};

template <class T>
struct result_expression<numsim::cas::tensor_expression<T>,
                         numsim::cas::scalar_expression<T>> {
  using type = numsim::cas::tensor_expression<T>;
};

template <class T>
struct result_expression<numsim::cas::scalar_expression<T>,
                         numsim::cas::tensor_expression<T>> {
  using type = numsim::cas::tensor_expression<T>;
};

template <class LHS, class RHS>
using result_expression_t = typename result_expression<LHS, RHS>::type;

} // namespace numsim::cas

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<std::is_base_of_v<numsim::cas::expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprLHS>::expr_type>,
                     bool> = true,
    std::enable_if_t<std::is_base_of_v<numsim::cas::expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprRHS>::expr_type>,
                     bool> = true>
constexpr inline numsim::cas::result_expression_t<ExprLHS, ExprRHS>
operator+(ExprLHS &&lhs, ExprRHS &&rhs) {
  assert(lhs.is_valid());
  assert(rhs.is_valid());
  return numsim::cas::operator_overload<
      numsim::cas::remove_cvref_t<ExprLHS>,
      numsim::cas::remove_cvref_t<ExprRHS>>::add(std::forward<ExprLHS>(lhs),
                                                 std::forward<ExprRHS>(rhs));
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<std::is_arithmetic_v<ExprLHS>, bool> = true,
    std::enable_if_t<std::is_base_of_v<numsim::cas::expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprRHS>::expr_type>,
                     bool> = true>
constexpr inline numsim::cas::result_expression_t<ExprLHS, ExprRHS>
operator+([[maybe_unused]] ExprLHS &&lhs, [[maybe_unused]] ExprRHS &&rhs) {
  using value_type =
      typename numsim::cas::remove_cvref_t<ExprRHS>::expr_type::value_type;
  auto scalar{numsim::cas::make_scalar_constant<value_type>(
      std::forward<ExprLHS>(lhs))};
  if (rhs.is_valid()) {
    return numsim::cas::operator_overload<
        decltype(scalar),
        numsim::cas::remove_cvref_t<ExprRHS>>::add(std::move(scalar),
                                                   std::forward<ExprRHS>(rhs));
  }
  return scalar;
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<std::is_base_of_v<numsim::cas::expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprLHS>::expr_type>,
                     bool> = true,
    std::enable_if_t<std::is_arithmetic_v<ExprRHS>, bool> = true>
constexpr inline numsim::cas::result_expression_t<ExprLHS, ExprRHS>
operator+([[maybe_unused]] ExprLHS &&lhs, [[maybe_unused]] ExprRHS &&rhs) {
  using value_type =
      typename numsim::cas::remove_cvref_t<ExprLHS>::expr_type::value_type;
  auto scalar{numsim::cas::make_scalar_constant<value_type>(
      std::forward<ExprRHS>(rhs))};
  if (lhs.is_valid()) {
    return numsim::cas::operator_overload<
        numsim::cas::remove_cvref_t<ExprLHS>,
        decltype(scalar)>::add(std::forward<ExprLHS>(lhs), std::move(scalar));
  }
  return scalar;
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<std::is_base_of_v<numsim::cas::expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprLHS>::expr_type>,
                     bool> = true,
    std::enable_if_t<std::is_base_of_v<numsim::cas::expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprRHS>::expr_type>,
                     bool> = true>
constexpr inline numsim::cas::result_expression_t<ExprLHS, ExprRHS>
operator-(ExprLHS &&lhs, ExprRHS &&rhs) {
  assert(lhs.is_valid());
  assert(rhs.is_valid());
  return numsim::cas::operator_overload<
      numsim::cas::remove_cvref_t<ExprLHS>,
      numsim::cas::remove_cvref_t<ExprRHS>>::sub(std::forward<ExprLHS>(lhs),
                                                 std::forward<ExprRHS>(rhs));
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<std::is_arithmetic_v<ExprLHS>, bool> = true,
    std::enable_if_t<std::is_base_of_v<numsim::cas::expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprRHS>::expr_type>,
                     bool> = true>
constexpr inline numsim::cas::result_expression_t<ExprLHS, ExprRHS>
operator-([[maybe_unused]] ExprLHS &&lhs, [[maybe_unused]] ExprRHS &&rhs) {
  using value_type =
      typename numsim::cas::remove_cvref_t<ExprRHS>::expr_type::value_type;
  auto scalar{numsim::cas::make_scalar_constant<value_type>(
      std::forward<ExprLHS>(lhs))};
  if (rhs.is_valid()) {
    return numsim::cas::operator_overload<
        decltype(scalar),
        numsim::cas::remove_cvref_t<ExprRHS>>::sub(std::move(scalar),
                                                   std::forward<ExprRHS>(rhs));
  }
  return scalar;
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<std::is_base_of_v<numsim::cas::expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprLHS>::expr_type>,
                     bool> = true,
    std::enable_if_t<std::is_arithmetic_v<ExprRHS>, bool> = true>
constexpr inline numsim::cas::result_expression_t<ExprLHS, ExprRHS>
operator-([[maybe_unused]] ExprLHS &&lhs, [[maybe_unused]] ExprRHS &&rhs) {
  using value_type =
      typename numsim::cas::remove_cvref_t<ExprLHS>::expr_type::value_type;
  auto scalar{numsim::cas::make_scalar_constant<value_type>(
      std::forward<ExprRHS>(rhs))};
  if (lhs.is_valid()) {
    return numsim::cas::operator_overload<
        numsim::cas::remove_cvref_t<ExprLHS>,
        decltype(scalar)>::sub(std::forward<ExprLHS>(lhs), std::move(scalar));
  }
  return scalar;
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<std::is_base_of_v<numsim::cas::expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprLHS>::expr_type>,
                     bool> = true,
    std::enable_if_t<std::is_base_of_v<numsim::cas::expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprRHS>::expr_type>,
                     bool> = true>
constexpr inline numsim::cas::result_expression_t<ExprLHS, ExprRHS>
operator/(ExprLHS &&lhs, ExprRHS &&rhs) {
  assert(lhs.is_valid());
  assert(rhs.is_valid());
  return numsim::cas::operator_overload<
      numsim::cas::remove_cvref_t<ExprLHS>,
      numsim::cas::remove_cvref_t<ExprRHS>>::div(std::forward<ExprLHS>(lhs),
                                                 std::forward<ExprRHS>(rhs));
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<std::is_base_of_v<numsim::cas::expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprLHS>::expr_type>,
                     bool> = true,
    std::enable_if_t<std::is_fundamental_v<ExprRHS>, bool> = true>
constexpr inline numsim::cas::result_expression_t<ExprLHS, ExprRHS>
operator/(ExprLHS &&lhs, ExprRHS &&rhs) {
  using value_type =
      typename numsim::cas::remove_cvref_t<ExprLHS>::expr_type::value_type;
  assert(lhs.is_valid());
  auto scalar{numsim::cas::make_scalar_constant<value_type>(
      std::forward<ExprRHS>(rhs))};
  return numsim::cas::operator_overload<
      numsim::cas::remove_cvref_t<ExprLHS>,
      decltype(scalar)>::div(std::forward<ExprLHS>(lhs), std::move(scalar));
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<std::is_fundamental_v<ExprLHS>, bool> = true,
    std::enable_if_t<std::is_base_of_v<numsim::cas::expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprRHS>::expr_type>,
                     bool> = true>
constexpr inline numsim::cas::result_expression_t<ExprLHS, ExprRHS>
operator/(ExprLHS &&lhs, ExprRHS &&rhs) {
  using value_type =
      typename numsim::cas::remove_cvref_t<ExprRHS>::expr_type::value_type;
  assert(rhs.is_valid());
  auto scalar{numsim::cas::make_scalar_constant<value_type>(
      std::forward<ExprLHS>(lhs))};
  return numsim::cas::operator_overload<
      decltype(scalar),
      numsim::cas::remove_cvref_t<ExprRHS>>::div(std::move(scalar),
                                                 std::forward<ExprRHS>(rhs));
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<std::is_base_of_v<numsim::cas::expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprLHS>::expr_type>,
                     bool> = true,
    std::enable_if_t<std::is_base_of_v<numsim::cas::expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprRHS>::expr_type>,
                     bool> = true>
constexpr inline numsim::cas::result_expression_t<ExprLHS, ExprRHS>
operator*(ExprLHS &&lhs, ExprRHS &&rhs) {
  assert(lhs.is_valid());
  assert(rhs.is_valid());
  return numsim::cas::operator_overload<
      numsim::cas::remove_cvref_t<ExprLHS>,
      numsim::cas::remove_cvref_t<ExprRHS>>::mul(std::forward<ExprLHS>(lhs),
                                                 std::forward<ExprRHS>(rhs));
}

template <
    typename ExprLHS, typename ExprRHS,
    typename = std::enable_if_t<std::is_arithmetic_v<ExprLHS>, bool>,
    std::enable_if_t<std::is_base_of_v<numsim::cas::expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprRHS>::expr_type>,
                     bool> = true>
constexpr inline numsim::cas::result_expression_t<ExprLHS, ExprRHS>
operator*([[maybe_unused]] ExprLHS &&lhs, [[maybe_unused]] ExprRHS &&rhs) {
  using value_type =
      typename numsim::cas::remove_cvref_t<ExprRHS>::expr_type::value_type;
  assert(rhs.is_valid());
  auto scalar{numsim::cas::make_scalar_constant<value_type>(
      std::forward<ExprLHS>(lhs))};
  return numsim::cas::operator_overload<
      decltype(scalar),
      numsim::cas::remove_cvref_t<ExprRHS>>::mul(std::move(scalar),
                                                 std::forward<ExprRHS>(rhs));
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<std::is_base_of_v<numsim::cas::expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprLHS>::expr_type>,
                     bool> = true,
    std::enable_if_t<std::is_arithmetic_v<ExprRHS>, bool> = true>
constexpr inline numsim::cas::result_expression_t<ExprLHS, ExprRHS>
operator*([[maybe_unused]] ExprLHS &&lhs, [[maybe_unused]] ExprRHS &&rhs) {
  using value_type =
      typename numsim::cas::remove_cvref_t<ExprLHS>::expr_type::value_type;
  assert(lhs.is_valid());
  auto scalar{numsim::cas::make_scalar_constant<value_type>(
      std::forward<ExprRHS>(rhs))};
  return numsim::cas::operator_overload<
      numsim::cas::remove_cvref_t<ExprLHS>,
      decltype(scalar)>::mul(std::forward<ExprLHS>(lhs), std::move(scalar));
}

#endif // OPERATORS_H
