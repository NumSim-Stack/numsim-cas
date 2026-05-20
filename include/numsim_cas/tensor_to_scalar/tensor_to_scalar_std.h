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

template <tensor_to_scalar_expr_holder ExprLHS,
          tensor_to_scalar_expr_holder ExprRHS>
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

template <tensor_to_scalar_expr_holder ExprLHS, typename ExprRHS>
requires std::is_arithmetic_v<std::remove_cvref_t<ExprRHS>>
[[nodiscard]] auto pow(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs) {
  auto constant{make_expression<tensor_to_scalar_scalar_wrapper>(
      make_expression<scalar_constant>(expr_rhs))};
  return pow(std::forward<ExprLHS>(expr_lhs), std::move(constant));
}

template <tensor_to_scalar_expr_holder ExprLHS, typename ExprRHS>
requires std::is_fundamental_v<std::remove_cvref_t<ExprRHS>>
[[nodiscard]] auto pow(ExprLHS const &expr_lhs, ExprRHS &&expr_rhs) {
  auto constant{make_expression<tensor_to_scalar_scalar_wrapper>(
      make_expression<scalar_constant>(expr_rhs))};
  return pow(expr_lhs, std::move(constant));
}

template <tensor_to_scalar_expr_holder ExprLHS, scalar_expr_holder ExprRHS>
[[nodiscard]] auto pow(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs) {
  return pow(std::forward<ExprLHS>(expr_lhs),
             make_expression<tensor_to_scalar_scalar_wrapper>(
                 std::forward<ExprRHS>(expr_rhs)));
}

template <tensor_to_scalar_expr_holder Expr>
[[nodiscard]] auto log(Expr &&expr) {
  return make_expression<tensor_to_scalar_log>(std::forward<Expr>(expr));
}

template <tensor_to_scalar_expr_holder Expr>
[[nodiscard]] auto exp(Expr &&expr) {
  // exp(0) → 1
  if (is_same<tensor_to_scalar_zero>(expr))
    return make_expression<tensor_to_scalar_one>();

  // exp(log(x)) → x (inverse pair)
  if (is_same<tensor_to_scalar_log>(expr))
    return expr.template get<tensor_to_scalar_log>().expr();

  return make_expression<tensor_to_scalar_exp>(std::forward<Expr>(expr));
}

template <tensor_to_scalar_expr_holder Expr>
[[nodiscard]] auto sqrt(Expr &&expr) {
  // sqrt(0) → 0
  if (is_same<tensor_to_scalar_zero>(expr))
    return make_expression<tensor_to_scalar_zero>();

  // sqrt(1) → 1
  if (is_same<tensor_to_scalar_one>(expr))
    return make_expression<tensor_to_scalar_one>();

  // sqrt(scalar_wrapper(1)) → 1
  {
    using traits = domain_traits<tensor_to_scalar_expression>;
    auto val = traits::try_numeric(expr);
    if (val && *val == scalar_number{1})
      return make_expression<tensor_to_scalar_one>();
  }

  return make_expression<tensor_to_scalar_sqrt>(std::forward<Expr>(expr));
}

namespace tensor_to_scalar_detail {
inline auto t2s_constant(scalar_number v) {
  return make_expression<tensor_to_scalar_scalar_wrapper>(
      make_expression<scalar_constant>(v));
}
} // namespace tensor_to_scalar_detail

template <tensor_to_scalar_expr_holder Expr>
[[nodiscard]] auto log10(Expr &&expr) {
  expression_holder<tensor_to_scalar_expression> log_ten =
      make_expression<tensor_to_scalar_log>(
          tensor_to_scalar_detail::t2s_constant(scalar_number{10}));
  return log(std::forward<Expr>(expr)) / std::move(log_ten);
}

template <tensor_to_scalar_expr_holder Expr>
[[nodiscard]] auto sinh(Expr const &expr) {
  if (is_same<tensor_to_scalar_zero>(expr))
    return make_expression<tensor_to_scalar_zero>();
  auto two = tensor_to_scalar_detail::t2s_constant(scalar_number{2});
  return (exp(expr) - exp(-expr)) / std::move(two);
}

template <tensor_to_scalar_expr_holder Expr>
[[nodiscard]] auto cosh(Expr const &expr) {
  if (is_same<tensor_to_scalar_zero>(expr))
    return make_expression<tensor_to_scalar_one>();
  // cosh(-x) = cosh(x) — even function.
  if (is_same<tensor_to_scalar_negative>(expr))
    return cosh(expr.template get<tensor_to_scalar_negative>().expr());
  auto two = tensor_to_scalar_detail::t2s_constant(scalar_number{2});
  return (exp(expr) + exp(-expr)) / std::move(two);
}

template <tensor_to_scalar_expr_holder Expr>
[[nodiscard]] auto tanh(Expr const &expr) {
  if (is_same<tensor_to_scalar_zero>(expr))
    return make_expression<tensor_to_scalar_zero>();
  // tanh(-x) = -tanh(x) — odd function.
  if (is_same<tensor_to_scalar_negative>(expr))
    return -tanh(expr.template get<tensor_to_scalar_negative>().expr());
  return sinh(expr) / cosh(expr);
}

template <tensor_to_scalar_expr_holder Expr>
[[nodiscard]] auto asinh(Expr const &expr) {
  if (is_same<tensor_to_scalar_zero>(expr))
    return make_expression<tensor_to_scalar_zero>();
  auto one = make_expression<tensor_to_scalar_one>();
  return log(expr + sqrt(pow(expr, 2) + std::move(one)));
}

template <tensor_to_scalar_expr_holder Expr>
[[nodiscard]] auto acosh(Expr const &expr) {
  // acosh(1) = 0
  if (is_same<tensor_to_scalar_one>(expr))
    return make_expression<tensor_to_scalar_zero>();
  auto one = make_expression<tensor_to_scalar_one>();
  return log(expr + sqrt(pow(expr, 2) - std::move(one)));
}

template <tensor_to_scalar_expr_holder Expr>
[[nodiscard]] auto atanh(Expr const &expr) {
  if (is_same<tensor_to_scalar_zero>(expr))
    return make_expression<tensor_to_scalar_zero>();
  // Build numerator and denominator as separate named statements so the
  // moves are unambiguously ordered before the division consumes them.
  // (operator/'s operand evaluation order is unspecified in C++.)
  auto num = make_expression<tensor_to_scalar_one>() + expr;
  auto den = make_expression<tensor_to_scalar_one>() - expr;
  auto two = tensor_to_scalar_detail::t2s_constant(scalar_number{2});
  return log(std::move(num) / std::move(den)) / std::move(two);
}

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_STD_H
