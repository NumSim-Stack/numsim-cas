#ifndef SCALAR_STD_H
#define SCALAR_STD_H

#include "../symTM_forward.h"
#include "../symTM_type_traits.h"
#include "visitors/scalar_printer.h"
#include "../functions.h"
#include <sstream>

namespace std {

template <typename ValueType>
auto to_string(
    numsim::cas::expression_holder<numsim::cas::scalar_expression<ValueType>> &&expr) {
  std::stringstream ss;
  numsim::cas::scalar_printer<ValueType, std::stringstream> printer(ss);
  printer.apply(expr);
  return ss.str();
}

template <typename ValueType>
auto to_string(
    numsim::cas::expression_holder<numsim::cas::scalar_expression<ValueType>> &expr) {
  std::stringstream ss;
  numsim::cas::scalar_printer<ValueType, std::stringstream> printer(ss);
  printer.apply(expr);
  return ss.str();
}
template<typename ExprLHS, typename ExprRHS,
          std::enable_if_t<std::is_base_of_v<numsim::cas::expression, typename numsim::cas::remove_cvref_t<ExprLHS>::expr_type>, bool> = true,
          std::enable_if_t<std::is_base_of_v<numsim::cas::expression, typename numsim::cas::remove_cvref_t<ExprRHS>::expr_type>, bool> = true>
auto pow(ExprLHS && expr_lhs, ExprRHS && expr_rhs){
  using value_type = std::common_type_t<typename numsim::cas::remove_cvref_t<ExprLHS>::expr_type::value_type,
                                        typename numsim::cas::remove_cvref_t<ExprRHS>::expr_type::value_type>;

  return numsim::cas::make_expression<numsim::cas::scalar_pow<value_type>>(std::forward<ExprLHS>(expr_lhs), std::forward<ExprRHS>(expr_rhs));
}

template<typename Expr,
          std::enable_if_t<std::is_base_of_v<numsim::cas::expression, typename numsim::cas::remove_cvref_t<Expr>::expr_type>, bool> = true>
auto sin(Expr && expr){
  using value_type = typename numsim::cas::remove_cvref_t<Expr>::expr_type::value_type;
  return numsim::cas::make_expression<numsim::cas::scalar_sin<value_type>>(std::forward<Expr>(expr));
}

template<typename Expr,
          std::enable_if_t<std::is_base_of_v<numsim::cas::expression, typename numsim::cas::remove_cvref_t<Expr>::expr_type>, bool> = true>
auto cos(Expr && expr){
  using value_type = typename numsim::cas::remove_cvref_t<Expr>::expr_type::value_type;
  return numsim::cas::make_expression<numsim::cas::scalar_cos<value_type>>(std::forward<Expr>(expr));
}

template<typename Expr,
          std::enable_if_t<std::is_base_of_v<numsim::cas::expression, typename numsim::cas::remove_cvref_t<Expr>::expr_type>, bool> = true>
auto tan(Expr && expr){
  using value_type = typename numsim::cas::remove_cvref_t<Expr>::expr_type::value_type;
  return numsim::cas::make_expression<numsim::cas::scalar_tan<value_type>>(std::forward<Expr>(expr));
}

template<typename Expr,
          std::enable_if_t<std::is_base_of_v<numsim::cas::expression, typename numsim::cas::remove_cvref_t<Expr>::expr_type>, bool> = true>
auto exp(Expr && expr){
  using value_type = typename numsim::cas::remove_cvref_t<Expr>::expr_type::value_type;
  return numsim::cas::make_expression<numsim::cas::scalar_exp<value_type>>(std::forward<Expr>(expr));
}

template<typename Expr,
          std::enable_if_t<std::is_base_of_v<numsim::cas::expression, typename numsim::cas::remove_cvref_t<Expr>::expr_type>, bool> = true>
auto abs(Expr && expr){
  using value_type = typename numsim::cas::remove_cvref_t<Expr>::expr_type::value_type;
  return numsim::cas::make_expression<numsim::cas::scalar_abs<value_type>>(std::forward<Expr>(expr));
}






template <
    typename ExprType, typename ValueType,
    std::enable_if_t<
        std::is_base_of_v<ExprType, std::remove_pointer_t<numsim::cas::get_type_t<
                                        numsim::cas::scalar_expression<ValueType>>>>,
        bool> = true>
auto sqrt(ExprType &&expr) {
  return numsim::cas::make_expression<numsim::cas::scalar_sqrt<ValueType>>(
      std::forward<ExprType>(expr));
}

template <
    typename ExprType, typename ValueType,
    std::enable_if_t<
        std::is_base_of_v<ExprType, std::remove_pointer_t<numsim::cas::get_type_t<
                                        numsim::cas::scalar_expression<ValueType>>>>,
        bool> = true>
auto cos(ExprType &&expr) {
  return numsim::cas::make_expression<numsim::cas::scalar_cos<ValueType>>(
      std::forward<ExprType>(expr));
}

template <
    typename ExprType, typename ValueType,
    std::enable_if_t<
        std::is_base_of_v<ExprType, std::remove_pointer_t<numsim::cas::get_type_t<
                                        numsim::cas::scalar_expression<ValueType>>>>,
        bool> = true>
auto sin(ExprType &&expr) {
  return numsim::cas::make_expression<numsim::cas::scalar_sin<ValueType>>(
      std::forward<ExprType>(expr));
}

template <
    typename ExprType, typename ValueType,
    std::enable_if_t<
        std::is_base_of_v<ExprType, std::remove_pointer_t<numsim::cas::get_type_t<
                                        numsim::cas::scalar_expression<ValueType>>>>,
        bool> = true>
auto tan(ExprType &&expr) {
  return numsim::cas::make_expression<numsim::cas::scalar_tan<ValueType>>(
      std::forward<ExprType>(expr));
}

template <
    typename ExprType, typename ValueType,
    std::enable_if_t<
        std::is_base_of_v<ExprType, std::remove_pointer_t<numsim::cas::get_type_t<
                                        numsim::cas::scalar_expression<ValueType>>>>,
        bool> = true>
auto sign(ExprType &&expr) {
  return numsim::cas::make_expression<numsim::cas::scalar_sign<ValueType>>(
      std::forward<ExprType>(expr));
}

template <
    typename ExprType, typename ValueType,
    std::enable_if_t<
        std::is_base_of_v<ExprType, std::remove_pointer_t<numsim::cas::get_type_t<
                                        numsim::cas::scalar_expression<ValueType>>>>,
        bool> = true>
auto log(ExprType &&expr) {
  return numsim::cas::make_expression<numsim::cas::scalar_log<ValueType>>(
      std::forward<ExprType>(expr));
}

template <
    typename ExprType, typename ValueType,
    std::enable_if_t<
        std::is_base_of_v<ExprType, std::remove_pointer_t<numsim::cas::get_type_t<
                                        numsim::cas::scalar_expression<ValueType>>>>,
        bool> = true>
auto exp(ExprType &&expr) {
  return numsim::cas::make_expression<numsim::cas::scalar_exp<ValueType>>(
      std::forward<ExprType>(expr));
}
} // namespace std

#endif // SCALAR_STD_H
