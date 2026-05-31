#ifndef TENSOR_TO_SCALAR_IF_THEN_ELSE_H
#define TENSOR_TO_SCALAR_IF_THEN_ELSE_H

#include <numsim_cas/core/ternary_op.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

/**
 * @class tensor_to_scalar_if_then_else
 * @brief Piecewise t2s selection: `cond != 0 ? then : else`.
 *
 * The t2s analogue of `scalar_if_then_else`. All three operands live in
 * the tensor-to-scalar domain: the condition is itself a t2s expression
 * (e.g. `trace(A) > 0` would, at runtime, evaluate to a 0/1 indicator),
 * and the selected branch is a t2s value (e.g. `norm(A)` vs. `det(A)`).
 *
 * Used for tensor-domain constitutive responses whose result lives in
 * the scalar codomain — yield surfaces, damage activation indicators
 * driven by tensor invariants, contact-pressure switches on stress
 * invariants.
 *
 * Construction-time simplifications (applied in
 * `if_then_else(...)` in `tensor_to_scalar_std.h`):
 * - `if_then_else(t2s_zero, a, b) → b`
 * - `if_then_else(t2s_one, a, b)  → a`
 * - `if_then_else(cond, a, a) → a` (then and else identical)
 *
 * Differentiation mirrors the scalar form. Assumes the condition does
 * not depend on the differentiation variable; conditions that do
 * depend on x technically have Dirac contributions at the boundary
 * that this rule ignores — acceptable for the constitutive-model use
 * cases that hit the boundary on a measure-zero set.
 *
 * Closes part of #135; closes part of #210.
 */
class tensor_to_scalar_if_then_else final
    : public ternary_op<
          tensor_to_scalar_node_base_t<tensor_to_scalar_if_then_else>> {
public:
  using base =
      ternary_op<tensor_to_scalar_node_base_t<tensor_to_scalar_if_then_else>>;

  using base::base;
  tensor_to_scalar_if_then_else(tensor_to_scalar_if_then_else const &expr)
      : base(static_cast<base const &>(expr)) {}
  tensor_to_scalar_if_then_else(tensor_to_scalar_if_then_else &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  tensor_to_scalar_if_then_else() = delete;
  ~tensor_to_scalar_if_then_else() override = default;
  const tensor_to_scalar_if_then_else &
  operator=(tensor_to_scalar_if_then_else &&) = delete;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_IF_THEN_ELSE_H
