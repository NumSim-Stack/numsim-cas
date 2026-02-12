#ifndef TENSOR_TO_SCALAR_STD_H
#define TENSOR_TO_SCALAR_STD_H

#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_printer.h>
#include <sstream>

namespace numsim::cas {

inline auto
to_string(const expression_holder<tensor_to_scalar_expression> &expr) {
  std::stringstream ss;
  tensor_to_scalar_printer<std::stringstream> printer(ss);
  printer.apply(expr);
  return ss.str();
}

template <typename ExprLHS, typename ExprRHS,
          std::enable_if_t<
              std::is_base_of_v<tensor_to_scalar_expression,
                                typename remove_cvref_t<ExprLHS>::expr_type>,
              bool> = true,
          std::enable_if_t<
              std::is_base_of_v<tensor_to_scalar_expression,
                                typename remove_cvref_t<ExprRHS>::expr_type>,
              bool> = true>
auto pow(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs) {
  return make_expression<tensor_to_scalar_pow>(std::forward<ExprLHS>(expr_lhs),
                                               std::forward<ExprRHS>(expr_rhs));
}

template <typename ExprLHS, typename ExprRHS,
          std::enable_if_t<
              std::is_base_of_v<tensor_to_scalar_expression,
                                typename remove_cvref_t<ExprLHS>::expr_type>,
              bool> = true,
          std::enable_if_t<std::is_arithmetic_v<ExprRHS>, bool> = true>
auto pow(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs) {
  auto constant{make_expression<tensor_to_scalar_scalar_wrapper>(
      make_expression<scalar_constant>(expr_rhs))};
  return make_expression<tensor_to_scalar_pow>(std::forward<ExprLHS>(expr_lhs),
                                               std::move(constant));
}

template <typename ExprLHS, typename ExprRHS,
          std::enable_if_t<
              std::is_base_of_v<tensor_to_scalar_expression,
                                typename remove_cvref_t<ExprLHS>::expr_type>,
              bool> = true,
          std::enable_if_t<std::is_fundamental_v<ExprRHS>, bool> = true>
auto pow(ExprLHS const &expr_lhs, ExprRHS &&expr_rhs) {
  auto constant{make_expression<tensor_to_scalar_scalar_wrapper>(
      make_expression<scalar_constant>(expr_rhs))};
  return make_expression<tensor_to_scalar_pow>(expr_lhs, std::move(constant));
}

template <typename ExprLHS, typename ExprRHS,
          std::enable_if_t<
              std::is_base_of_v<tensor_to_scalar_expression,
                                typename remove_cvref_t<ExprLHS>::expr_type>,
              bool> = true,
          std::enable_if_t<
              std::is_base_of_v<scalar_expression,
                                typename remove_cvref_t<ExprRHS>::expr_type>,
              bool> = true>
auto pow(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs) {
  return make_expression<tensor_to_scalar_pow>(
      std::forward<ExprLHS>(expr_lhs),
      make_expression<tensor_to_scalar_scalar_wrapper>(
          std::forward<ExprRHS>(expr_rhs)));
}

template <
    typename Expr,
    std::enable_if_t<std::is_same_v<typename std::decay_t<Expr>::expr_type,
                                    tensor_to_scalar_expression>,
                     bool> = true>
auto log(Expr &&expr) {
  return make_expression<tensor_to_scalar_log>(std::forward<Expr>(expr));
}

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_STD_H
