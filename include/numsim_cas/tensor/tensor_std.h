#ifndef TENSOR_STD_H
#define TENSOR_STD_H

#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/visitors/tensor_printer.h>
#include <sstream>

// namespace std {
namespace numsim::cas {

inline auto to_string(
    numsim::cas::expression_holder<numsim::cas::tensor_expression> &&expr) {
  std::stringstream ss;
  numsim::cas::tensor_printer<std::stringstream> printer(ss);
  printer.apply(expr);
  return ss.str();
}

inline auto to_string(
    numsim::cas::expression_holder<numsim::cas::tensor_expression> &expr) {
  std::stringstream ss;
  numsim::cas::tensor_printer<std::stringstream> printer(ss);
  printer.apply(expr);
  return ss.str();
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<
        std::is_base_of_v<numsim::cas::tensor_expression,
                          typename std::remove_cvref_t<ExprLHS>::expr_type>,
        bool> = true,
    std::enable_if_t<
        std::is_base_of_v<numsim::cas::scalar_expression,
                          typename std::remove_cvref_t<ExprRHS>::expr_type>,
        bool> = true>
auto pow(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs) {
  return numsim::cas::make_expression<numsim::cas::tensor_pow>(
      std::forward<ExprLHS>(expr_lhs), std::forward<ExprRHS>(expr_rhs));
}

template <
    typename ExprLHS, typename ExprRHS,
    std::enable_if_t<
        std::is_base_of_v<numsim::cas::tensor_expression,
                          typename std::remove_cvref_t<ExprLHS>::expr_type>,
        bool> = true,
    std::enable_if_t<std::is_integral_v<ExprRHS>, bool> = true>
auto pow(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs) {
  auto constant{
      numsim::cas::make_expression<numsim::cas::scalar_constant>(expr_rhs)};
  return numsim::cas::make_expression<numsim::cas::tensor_pow>(
      std::forward<ExprLHS>(expr_lhs), std::move(constant));
}

// template <
//     typename ExprLHS, typename ExprRHS,
//     std::enable_if_t<std::is_base_of_v<numsim::cas::tensor_expression,
//                                        typename std::remove_cvref_t<
//                                            ExprLHS>::expr_type>,
//                      bool> = true,
//     std::enable_if_t<std::is_integral_v<ExprRHS>, bool> = true>
// auto pow(ExprLHS const &expr_lhs, ExprRHS &&expr_rhs) {
//   auto constant{
//       numsim::cas::make_expression<numsim::cas::scalar_constant>(expr_rhs)};
//   return numsim::cas::make_expression<numsim::cas::tensor_pow>(
//       expr_lhs, std::move(constant));
// }

} // namespace numsim::cas

#endif // TENSOR_STD_H
