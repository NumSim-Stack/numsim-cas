#ifndef TENSOR_STD_H
#define TENSOR_STD_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/scalar/scalar_constant.h>
#include <numsim_cas/scalar/scalar_one.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_zero.h>
#include <numsim_cas/tensor/functions/tensor_pow.h>
#include <numsim_cas/tensor/kronecker_delta.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/visitors/tensor_printer.h>
#include <sstream>

// namespace std {
namespace numsim::cas {

[[nodiscard]] inline auto to_string(
    numsim::cas::expression_holder<numsim::cas::tensor_expression> &&expr) {
  std::stringstream ss;
  numsim::cas::tensor_printer<std::stringstream> printer(ss);
  printer.apply(expr);
  return ss.str();
}

[[nodiscard]] inline auto to_string(
    numsim::cas::expression_holder<numsim::cas::tensor_expression> &expr) {
  std::stringstream ss;
  numsim::cas::tensor_printer<std::stringstream> printer(ss);
  printer.apply(expr);
  return ss.str();
}

template <tensor_expr_holder ExprLHS, scalar_expr_holder ExprRHS>
[[nodiscard]] auto pow(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs) {
  // pow(A, 0) → identity
  if (is_same<scalar_zero>(expr_rhs) ||
      (is_same<scalar_constant>(expr_rhs) &&
       expr_rhs.template get<scalar_constant>().value() == scalar_number{0})) {
    return make_expression<kronecker_delta>(expr_lhs.get().dim());
  }

  // pow(A, 1) → A
  if (is_same<scalar_one>(expr_rhs) ||
      (is_same<scalar_constant>(expr_rhs) &&
       expr_rhs.template get<scalar_constant>().value() == scalar_number{1})) {
    return std::forward<ExprLHS>(expr_lhs);
  }

  // pow(pow(A, a), b) → pow(A, a*b)
  if (is_same<tensor_pow>(expr_lhs)) {
    auto const &inner = expr_lhs.template get<tensor_pow>();
    return pow(inner.expr_lhs(), inner.expr_rhs() * expr_rhs);
  }

  return numsim::cas::make_expression<numsim::cas::tensor_pow>(
      std::forward<ExprLHS>(expr_lhs), std::forward<ExprRHS>(expr_rhs));
}

template <tensor_expr_holder ExprLHS, typename ExprRHS>
requires std::is_integral_v<std::remove_cvref_t<ExprRHS>>
[[nodiscard]] auto pow(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs) {
  auto constant{
      numsim::cas::make_expression<numsim::cas::scalar_constant>(expr_rhs)};
  return pow(std::forward<ExprLHS>(expr_lhs), std::move(constant));
}

} // namespace numsim::cas

#endif // TENSOR_STD_H
