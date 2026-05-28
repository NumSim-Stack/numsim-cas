#ifndef TENSOR_STD_H
#define TENSOR_STD_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/scalar/scalar_constant.h>
#include <numsim_cas/scalar/scalar_one.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_zero.h>
#include <numsim_cas/tensor/identity_tensor.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/tensor_zero.h>
#include <numsim_cas/tensor/visitors/tensor_printer.h>
#include <numsim_cas/tensor/wrappers/tensor_pow.h>
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
  // pow(0, n) → 0
  if (is_same<tensor_zero>(expr_lhs))
    return make_expression<tensor_zero>(expr_lhs.get().dim(),
                                        expr_lhs.get().rank());
  // pow(A, 0) → identity
  if (is_same<scalar_zero>(expr_rhs) ||
      (is_same<scalar_constant>(expr_rhs) &&
       expr_rhs.template get<scalar_constant>().value() == scalar_number{0})) {
    return make_expression<identity_tensor>(expr_lhs.get().dim(),
                                            std::size_t{2});
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

  // pow(I, n) → I. The rank-2 identity is its own n-th power under
  // tensor multiplication for any n. Closes part of #96.
  if (is_same<identity_tensor>(expr_lhs))
    return std::forward<ExprLHS>(expr_lhs);

  // pow(inv(A), n) → inv(pow(A, n)) — pulls the inverse out so the
  // pow(A, n) result can fold further and the evaluator only needs to
  // invert once at the end. Constructed directly via
  // make_expression<tensor_inv> rather than calling inv() to avoid a
  // tensor_std.h → tensor_functions.h include cycle; the inv()
  // construction-time guards (rank-2 / zero / skew, see #187 / #192)
  // were already enforced when the inner tensor_inv we're matching on
  // was first built, so bypassing them here is safe.
  // Closes part of #96.
  if (is_same<tensor_inv>(expr_lhs)) {
    auto const &inv_node = expr_lhs.template get<tensor_inv>();
    return make_expression<tensor_inv>(
        pow(inv_node.expr(), std::forward<ExprRHS>(expr_rhs)));
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
