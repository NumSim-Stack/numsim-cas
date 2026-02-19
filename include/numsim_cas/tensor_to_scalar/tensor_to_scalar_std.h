#ifndef TENSOR_TO_SCALAR_STD_H
#define TENSOR_TO_SCALAR_STD_H

#include <numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_pow.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_printer.h>
#include <sstream>

namespace numsim::cas {

[[nodiscard]] inline auto
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
[[nodiscard]] auto pow(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs) {
  // pow(1, x) --> 1
  if (is_same<tensor_to_scalar_one>(expr_lhs))
    return make_expression<tensor_to_scalar_one>();

  // pow(x, 0) --> 1
  if (is_same<tensor_to_scalar_zero>(expr_rhs))
    return make_expression<tensor_to_scalar_one>();

  // pow(x, 1) --> x
  if (is_same<tensor_to_scalar_one>(expr_rhs))
    return std::forward<ExprLHS>(expr_lhs);

  // Check numeric 0/1 via try_numeric (catches scalar_wrapper constants)
  {
    using traits = domain_traits<tensor_to_scalar_expression>;
    auto rhs_val = traits::try_numeric(expr_rhs);
    if (rhs_val) {
      if (*rhs_val == scalar_number{0})
        return make_expression<tensor_to_scalar_one>();
      if (*rhs_val == scalar_number{1})
        return std::forward<ExprLHS>(expr_lhs);
    }
    auto lhs_val = traits::try_numeric(expr_lhs);
    if (lhs_val && *lhs_val == scalar_number{1})
      return make_expression<tensor_to_scalar_one>();
  }

  // Full simplification via visitor dispatch
  auto &_lhs{expr_lhs.template get<tensor_to_scalar_visitable_t>()};
  tensor_to_scalar_detail::simplifier::pow_base visitor(
      std::forward<ExprLHS>(expr_lhs), std::forward<ExprRHS>(expr_rhs));
  return _lhs.accept(visitor);
}

template <typename ExprLHS, typename ExprRHS,
          std::enable_if_t<
              std::is_base_of_v<tensor_to_scalar_expression,
                                typename remove_cvref_t<ExprLHS>::expr_type>,
              bool> = true,
          std::enable_if_t<std::is_arithmetic_v<ExprRHS>, bool> = true>
[[nodiscard]] auto pow(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs) {
  auto constant{make_expression<tensor_to_scalar_scalar_wrapper>(
      make_expression<scalar_constant>(expr_rhs))};
  return pow(std::forward<ExprLHS>(expr_lhs), std::move(constant));
}

template <typename ExprLHS, typename ExprRHS,
          std::enable_if_t<
              std::is_base_of_v<tensor_to_scalar_expression,
                                typename remove_cvref_t<ExprLHS>::expr_type>,
              bool> = true,
          std::enable_if_t<std::is_fundamental_v<ExprRHS>, bool> = true>
[[nodiscard]] auto pow(ExprLHS const &expr_lhs, ExprRHS &&expr_rhs) {
  auto constant{make_expression<tensor_to_scalar_scalar_wrapper>(
      make_expression<scalar_constant>(expr_rhs))};
  return pow(expr_lhs, std::move(constant));
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
[[nodiscard]] auto pow(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs) {
  return pow(std::forward<ExprLHS>(expr_lhs),
             make_expression<tensor_to_scalar_scalar_wrapper>(
                 std::forward<ExprRHS>(expr_rhs)));
}

template <
    typename Expr,
    std::enable_if_t<std::is_same_v<typename std::decay_t<Expr>::expr_type,
                                    tensor_to_scalar_expression>,
                     bool> = true>
[[nodiscard]] auto log(Expr &&expr) {
  return make_expression<tensor_to_scalar_log>(std::forward<Expr>(expr));
}

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_STD_H
