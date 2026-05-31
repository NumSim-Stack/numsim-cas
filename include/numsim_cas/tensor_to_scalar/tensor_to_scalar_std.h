#ifndef TENSOR_TO_SCALAR_STD_H
#define TENSOR_TO_SCALAR_STD_H

#include <numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_pow.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_if_then_else.h>
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

// CONTRACT NOTE: `pow(x, -1)` is deliberately NOT short-circuited to a
// dedicated reciprocal node. The tensor ÷ tensor_to_scalar operator
// (tensor_operators.h, see #147) implements division as
// `lhs × pow(rhs, -1)` and relies on the result being a regular
// `tensor_to_scalar_pow` node so the existing pow-of-pow flatten
// (`pow(pow(x, m), n) → pow(x, m*n)`) handles further composition. If
// you add a `pow(x, -1) → reciprocal_node` rule here, also update the
// div operator to construct that node directly, and update the lock-in
// tests in `tests/TensorToScalarDivOperatorTest.h::ResultShapeIsMulPow`
// and `DivByPowFlattensExponent`.
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

// ─── if_then_else (#135 / #210) ────────────────────────────────────────
// Piecewise t2s selection: cond != 0 ? then : else. The condition lives
// in the t2s domain (typically a comparison built from tensor
// invariants — e.g. `trace(A) > 0`). All three operands are t2s.
//
// Construction-time simplifications (mirroring scalar_if_then_else):
//   if_then_else(t2s_zero, a, b) → b
//   if_then_else(t2s_one, a, b)  → a
//   if_then_else(cond, a, a) → a   (then and else identical)
template <tensor_to_scalar_expr_holder Cond, tensor_to_scalar_expr_holder Then,
          tensor_to_scalar_expr_holder Else>
[[nodiscard]] auto if_then_else(Cond &&cond, Then &&then_expr,
                                Else &&else_expr) {
  assert(cond.is_valid());
  assert(then_expr.is_valid());
  assert(else_expr.is_valid());
  if (is_same<tensor_to_scalar_zero>(cond))
    return std::forward<Else>(else_expr);
  if (is_same<tensor_to_scalar_one>(cond))
    return std::forward<Then>(then_expr);
  // Identical branches collapse regardless of cond
  if (then_expr.get().hash_value() == else_expr.get().hash_value())
    return std::forward<Then>(then_expr);
  return make_expression<tensor_to_scalar_if_then_else>(
      std::forward<Cond>(cond), std::forward<Then>(then_expr),
      std::forward<Else>(else_expr));
}

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_STD_H
