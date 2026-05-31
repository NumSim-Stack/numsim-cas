#ifndef TENSOR_STD_H
#define TENSOR_STD_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/scalar/scalar_constant.h>
#include <numsim_cas/scalar/scalar_one.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_zero.h>
#include <numsim_cas/tensor/identity_tensor.h>
#include <numsim_cas/tensor/levi_civita_tensor.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/tensor_if_then_else.h>
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

// ─── Levi-Civita symbol factory (#34) ──────────────────────────────
// `levi_civita(dim)` constructs the rank-`dim` permutation symbol
// ε in `dim` spatial dimensions. Only dim ∈ {2, 3, 4} are supported
// (matching `tmech::levi_civita`'s coverage). The 3-D case is the
// usual ε_{ijk} for cross products and curl; 2-D is the rank-2 skew
// unit; 4-D is the rank-4 form used in relativistic and continuum
// constructions.
[[nodiscard]] inline auto levi_civita(std::size_t dim) {
  return make_expression<levi_civita_tensor>(dim);
}

// ─── if_then_else (#135 / #210) ──────────────────────────────────────
// Piecewise tensor selection with a SCALAR condition. The condition
// is a scalar 0/1 indicator (per #136's comparison convention); the
// branches are tensors that must share dim and rank.
//
// Construction-time simplifications:
//   if_then_else(scalar_zero, a, b) → b
//   if_then_else(scalar_one, a, b)  → a
//   if_then_else(cond, a, a) → a   (then and else identical)
template <scalar_expr_holder Cond, tensor_expr_holder Then,
          tensor_expr_holder Else>
[[nodiscard]] auto if_then_else(Cond &&cond, Then &&then_expr,
                                Else &&else_expr) {
  assert(cond.is_valid());
  assert(then_expr.is_valid());
  assert(else_expr.is_valid());
  // Branches must share shape — gate before any simplification.
  assert(then_expr.get().dim() == else_expr.get().dim());
  assert(then_expr.get().rank() == else_expr.get().rank());
  if (is_same<scalar_zero>(cond))
    return std::forward<Else>(else_expr);
  if (is_same<scalar_one>(cond))
    return std::forward<Then>(then_expr);
  if (then_expr == else_expr)
    return std::forward<Then>(then_expr);
  return make_expression<tensor_if_then_else>(std::forward<Cond>(cond),
                                              std::forward<Then>(then_expr),
                                              std::forward<Else>(else_expr));
}

} // namespace numsim::cas

#endif // TENSOR_STD_H
