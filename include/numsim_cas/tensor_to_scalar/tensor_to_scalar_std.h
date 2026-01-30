#ifndef TENSOR_TO_SCALAR_STD_H
#define TENSOR_TO_SCALAR_STD_H

#include "../basic_functions.h"
#include "../expression_holder.h"
#include "../numsim_cas_type_traits.h"
#include "visitors/tensor_to_scalar_printer.h"
#include <sstream>

namespace std {

auto to_string(
    numsim::cas::expression_holder<numsim::cas::tensor_to_scalar_expression>
        &&expr) {
  std::stringstream ss;
  numsim::cas::tensor_to_scalar_printer<std::stringstream> printer(ss);
  printer.apply(expr);
  return ss.str();
}

auto to_string(const numsim::cas::expression_holder<
               numsim::cas::tensor_to_scalar_expression> &expr) {
  std::stringstream ss;
  numsim::cas::tensor_to_scalar_printer<std::stringstream> printer(ss);
  printer.apply(expr);
  return ss.str();
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<std::is_base_of_v<numsim::cas::tensor_to_scalar_expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprLHS>::expr_type>,
                     bool> = true,
    std::enable_if_t<std::is_base_of_v<numsim::cas::tensor_to_scalar_expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprRHS>::expr_type>,
                     bool> = true>
auto pow(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs) {
  return numsim::cas::make_expression<numsim::cas::tensor_to_scalar_pow>(
      std::forward<ExprLHS>(expr_lhs), std::forward<ExprRHS>(expr_rhs));
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<std::is_base_of_v<numsim::cas::tensor_to_scalar_expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprLHS>::expr_type>,
                     bool> = true,
    std::enable_if_t<std::is_arithmetic_v<ExprRHS>, bool> = true>
auto pow(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs) {
  auto constant{
      numsim::cas::make_expression<numsim::cas::scalar_constant>(expr_rhs)};
  return numsim::cas::make_expression<
      numsim::cas::tensor_to_scalar_pow_with_scalar_exponent>(
      std::forward<ExprLHS>(expr_lhs), std::move(constant));
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<std::is_base_of_v<numsim::cas::tensor_to_scalar_expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprLHS>::expr_type>,
                     bool> = true,
    std::enable_if_t<std::is_fundamental_v<ExprRHS>, bool> = true>
auto pow(ExprLHS const &expr_lhs, ExprRHS &&expr_rhs) {
  auto constant{
      numsim::cas::make_expression<numsim::cas::scalar_constant>(expr_rhs)};
  return numsim::cas::make_expression<
      numsim::cas::tensor_to_scalar_pow_with_scalar_exponent>(
      expr_lhs, std::move(constant));
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<std::is_base_of_v<numsim::cas::tensor_to_scalar_expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprLHS>::expr_type>,
                     bool> = true,
    std::enable_if_t<std::is_base_of_v<numsim::cas::scalar_expression,
                                       typename numsim::cas::remove_cvref_t<
                                           ExprRHS>::expr_type>,
                     bool> = true>
auto pow(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs) {
  return numsim::cas::make_expression<
      numsim::cas::tensor_to_scalar_pow_with_scalar_exponent>(
      std::forward<ExprLHS>(expr_lhs), std::forward<ExprRHS>(expr_rhs));
}

template <
    typename Expr,
    std::enable_if_t<std::is_same_v<typename std::decay_t<Expr>::expr_type,
                                    numsim::cas::tensor_to_scalar_expression>,
                     bool> = true>
auto log(Expr &&expr) {
  return numsim::cas::make_expression<numsim::cas::tensor_to_scalar_log>(
      std::forward<Expr>(expr));
}

} // namespace std

#endif // TENSOR_TO_SCALAR_STD_H
