#ifndef TENSOR_STD_H
#define TENSOR_STD_H

#include "../expression_holder.h"
#include "../numsim_cas_type_traits.h"
#include "visitors/tensor_printer.h"
#include <sstream>

namespace std {

template <typename ValueType>
auto to_string(
    numsim::cas::expression_holder<numsim::cas::tensor_expression<ValueType>>
        &&expr) {
  std::stringstream ss;
  numsim::cas::tensor_printer<ValueType, std::stringstream> printer(ss);
  printer.apply(expr);
  return ss.str();
}

template <typename ValueType>
auto to_string(
    numsim::cas::expression_holder<numsim::cas::tensor_expression<ValueType>>
        &expr) {
  std::stringstream ss;
  numsim::cas::tensor_printer<ValueType, std::stringstream> printer(ss);
  printer.apply(expr);
  return ss.str();
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<
        std::is_base_of_v<
            numsim::cas::tensor_expression<
                typename numsim::cas::remove_cvref_t<ExprLHS>::value_type>,
            typename numsim::cas::remove_cvref_t<ExprLHS>::expr_type>,
        bool> = true,
    std::enable_if_t<
        std::is_base_of_v<
            numsim::cas::scalar_expression<
                typename numsim::cas::remove_cvref_t<ExprRHS>::value_type>,
            typename numsim::cas::remove_cvref_t<ExprRHS>::expr_type>,
        bool> = true,
    std::enable_if_t<std::is_integral_v<typename numsim::cas::remove_cvref_t<
                         ExprRHS>::value_type>,
                     bool> = true>
auto pow(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs) {
  using value_type = std::common_type_t<
      typename numsim::cas::remove_cvref_t<ExprLHS>::expr_type::value_type,
      typename numsim::cas::remove_cvref_t<ExprRHS>::expr_type::value_type>;
  return numsim::cas::make_expression<numsim::cas::tensor_pow<value_type>>(
      std::forward<ExprLHS>(expr_lhs), std::forward<ExprRHS>(expr_rhs));
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<
        std::is_base_of_v<
            numsim::cas::tensor_expression<
                typename numsim::cas::remove_cvref_t<ExprLHS>::value_type>,
            typename numsim::cas::remove_cvref_t<ExprLHS>::expr_type>,
        bool> = true,
    std::enable_if_t<std::is_integral_v<ExprRHS>, bool> = true>
auto pow(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs) {
  using value_type = std::common_type_t<
      typename numsim::cas::remove_cvref_t<ExprLHS>::expr_type::value_type,
      ExprRHS>;
  auto constant{numsim::cas::make_expression<numsim::cas::scalar_constant<int>>(
      expr_rhs)};
  return numsim::cas::make_expression<numsim::cas::tensor_pow<value_type>>(
      std::forward<ExprLHS>(expr_lhs), std::move(constant));
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<
        std::is_base_of_v<
            numsim::cas::tensor_expression<
                typename numsim::cas::remove_cvref_t<ExprLHS>::value_type>,
            typename numsim::cas::remove_cvref_t<ExprLHS>::expr_type>,
        bool> = true,
    std::enable_if_t<std::is_integral_v<ExprRHS>, bool> = true>
auto pow(ExprLHS const &expr_lhs, ExprRHS &&expr_rhs) {
  using value_type = std::common_type_t<
      typename numsim::cas::remove_cvref_t<ExprLHS>::expr_type::value_type,
      ExprRHS>;
  auto constant{numsim::cas::make_expression<numsim::cas::scalar_constant<int>>(
      expr_rhs)};
  return numsim::cas::make_expression<numsim::cas::tensor_pow<value_type>>(
      expr_lhs, std::move(constant));
}

} // namespace std

#endif // TENSOR_STD_H
