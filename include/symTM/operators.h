#ifndef OPERATORS_H
#define OPERATORS_H

#include "symTM_type_traits.h"
#include "symTM_forward.h"
#include "basic_functions.h"

namespace symTM {

template<class LHS, class RHS>
struct result_expression;

template<class T>
struct result_expression<T, T>{
  using type = T;
};

template<class T>
struct result_expression<symTM::scalar_expression<T>, int>{
  using type = symTM::scalar_expression<T>;
};

template<class T>
struct result_expression<symTM::scalar_expression<T>, double>{
  using type = symTM::scalar_expression<T>;
};

template<class T>
struct result_expression<symTM::scalar_expression<T>, float>{
  using type = symTM::scalar_expression<T>;
};

template<class T>
struct result_expression<int, symTM::scalar_expression<T>>{
  using type = symTM::scalar_expression<T>;
};

template<class T>
struct result_expression<double, symTM::scalar_expression<T>>{
  using type = symTM::scalar_expression<T>;
};

template<class T>
struct result_expression<float, symTM::scalar_expression<T>>{
  using type = symTM::scalar_expression<T>;
};

template<class T>
struct result_expression<symTM::tensor_expression<T>, symTM::scalar_expression<T>>{
  using type = symTM::tensor_expression<T>;
};

template<class T>
struct result_expression<symTM::scalar_expression<T>, symTM::tensor_expression<T>>{
  using type = symTM::tensor_expression<T>;
};

template<class LHS, class RHS>
using result_expression_t = typename result_expression<LHS, RHS>::type;

}


template<typename ExprLHS, typename ExprRHS,
          std::enable_if_t<std::is_base_of_v<symTM::expression, typename symTM::remove_cvref_t<ExprLHS>::expr_type>, bool> = true,
          std::enable_if_t<std::is_base_of_v<symTM::expression, typename symTM::remove_cvref_t<ExprRHS>::expr_type>, bool> = true>
constexpr inline auto operator+(ExprLHS &&lhs,
          ExprRHS &&rhs){
  assert(lhs.is_valid());
  assert(rhs.is_valid());
  return symTM::operator_overload<symTM::remove_cvref_t<ExprLHS>, symTM::remove_cvref_t<ExprRHS>>::add(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs));
}

template<typename ExprLHS, typename ExprRHS,
          std::enable_if_t<std::is_arithmetic_v<ExprLHS>, bool> = true,
          std::enable_if_t<std::is_base_of_v<symTM::expression, typename symTM::remove_cvref_t<ExprRHS>::expr_type>, bool> = true>
constexpr inline auto operator+([[maybe_unused]]ExprLHS &&lhs,
          [[maybe_unused]]ExprRHS &&rhs){
  using value_type = typename symTM::remove_cvref_t<ExprRHS>::expr_type::value_type;
  auto scalar{symTM::make_scalar_constant<value_type>(std::forward<ExprLHS>(lhs))};
  if(rhs.is_valid()){
    return symTM::operator_overload<decltype(scalar), symTM::remove_cvref_t<ExprRHS>>::add(std::move(scalar), std::forward<ExprRHS>(rhs));
  }
  return scalar;
}

template<typename ExprLHS, typename ExprRHS,
          std::enable_if_t<std::is_base_of_v<symTM::expression, typename symTM::remove_cvref_t<ExprLHS>::expr_type>, bool> = true,
          std::enable_if_t<std::is_arithmetic_v<ExprRHS>, bool> = true>
constexpr inline auto operator+([[maybe_unused]]ExprLHS &&lhs,
          [[maybe_unused]]ExprRHS &&rhs){
  using value_type = typename symTM::remove_cvref_t<ExprLHS>::expr_type::value_type;
  auto scalar{symTM::make_scalar_constant<value_type>(std::forward<ExprRHS>(rhs))};
  if(lhs.is_valid()){
    return symTM::operator_overload<symTM::remove_cvref_t<ExprLHS>, decltype(scalar)>::add(std::forward<ExprLHS>(lhs), std::move(scalar));
  }
  return scalar;
}

template<typename ExprLHS, typename ExprRHS,
          std::enable_if_t<std::is_base_of_v<symTM::expression, typename symTM::remove_cvref_t<ExprLHS>::expr_type>, bool> = true,
          std::enable_if_t<std::is_base_of_v<symTM::expression, typename symTM::remove_cvref_t<ExprRHS>::expr_type>, bool> = true>
constexpr inline auto operator-(ExprLHS &&lhs, ExprRHS &&rhs){
  assert(lhs.is_valid());
  assert(rhs.is_valid());
  return symTM::operator_overload<symTM::remove_cvref_t<ExprLHS>, symTM::remove_cvref_t<ExprRHS>>::sub(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs));
}

template<typename ExprLHS, typename ExprRHS,
          std::enable_if_t<std::is_base_of_v<symTM::expression, typename symTM::remove_cvref_t<ExprLHS>::expr_type>, bool> = true,
          std::enable_if_t<std::is_base_of_v<symTM::expression, typename symTM::remove_cvref_t<ExprRHS>::expr_type>, bool> = true>
constexpr inline auto operator/(ExprLHS &&lhs, ExprRHS &&rhs){
  assert(lhs.is_valid());
  assert(rhs.is_valid());
  return symTM::operator_overload<symTM::remove_cvref_t<ExprLHS>, symTM::remove_cvref_t<ExprRHS>>::div(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs));
}

template<typename ExprLHS, typename ExprRHS,
          std::enable_if_t<std::is_base_of_v<symTM::expression, typename symTM::remove_cvref_t<ExprLHS>::expr_type>, bool> = true,
          std::enable_if_t<std::is_base_of_v<symTM::expression, typename symTM::remove_cvref_t<ExprRHS>::expr_type>, bool> = true>
constexpr inline auto operator*(ExprLHS &&lhs, ExprRHS &&rhs){
  assert(lhs.is_valid());
  assert(rhs.is_valid());
  return symTM::operator_overload<symTM::remove_cvref_t<ExprLHS>, symTM::remove_cvref_t<ExprRHS>>::mul(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs));
}

template<typename ExprLHS, typename ExprRHS,
          typename = std::enable_if_t<std::is_arithmetic_v<ExprLHS>, bool>,
          std::enable_if_t<std::is_base_of_v<symTM::expression, typename symTM::remove_cvref_t<ExprRHS>::expr_type>, bool> = true>
constexpr inline auto//symTM::expression_holder<result_expression_t<ExprLHS, typename symTM::remove_cvref_t<ExprRHS>::expr_type>>
operator*([[maybe_unused]]ExprLHS &&lhs,
          [[maybe_unused]]ExprRHS &&rhs){
  using value_type = typename symTM::remove_cvref_t<ExprRHS>::expr_type::value_type;
  assert(rhs.is_valid());
  auto scalar{symTM::make_scalar_constant<value_type>(std::forward<ExprLHS>(lhs))};
  return symTM::operator_overload<decltype(scalar), symTM::remove_cvref_t<ExprRHS>>::mul(std::move(scalar), std::forward<ExprRHS>(rhs));
}

template<typename ExprLHS, typename ExprRHS,
          std::enable_if_t<std::is_base_of_v<symTM::expression, typename symTM::remove_cvref_t<ExprLHS>::expr_type>, bool> = true,
          std::enable_if_t<std::is_arithmetic_v<ExprRHS>, bool> = true>
constexpr inline auto//symTM::expression_holder<result_expression_t<typename symTM::remove_cvref_t<ExprLHS>::expr_type, typename symTM::remove_cvref_t<ExprRHS>>>
operator*([[maybe_unused]]ExprLHS &&lhs,
          [[maybe_unused]]ExprRHS &&rhs){
  using value_type = typename symTM::remove_cvref_t<ExprLHS>::expr_type::value_type;
  assert(lhs.is_valid());
  auto scalar{symTM::make_scalar_constant<value_type>(std::forward<ExprRHS>(rhs))};
  return symTM::operator_overload<symTM::remove_cvref_t<ExprLHS>, decltype(scalar)>::mul(std::forward<ExprLHS>(lhs), std::move(scalar));
}

#endif // OPERATORS_H
