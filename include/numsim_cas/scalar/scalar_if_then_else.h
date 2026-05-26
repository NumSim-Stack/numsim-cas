#ifndef SCALAR_IF_THEN_ELSE_H
#define SCALAR_IF_THEN_ELSE_H

#include <numsim_cas/core/ternary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

/**
 * @class scalar_if_then_else
 * @brief Piecewise scalar selection: `cond != 0 ? then : else`.
 *
 * The condition is a scalar evaluating to `0.0` for false and any non-zero
 * value (canonically `1.0`) for true — matching the indicator-value
 * convention of the comparison nodes from #136. This is the "Option B"
 * design from the issue (predicates are scalars, not a separate domain).
 *
 * Used to express piecewise constitutive responses:
 * - Damage activation: `if_then_else(yield_func > 0, damaged, elastic)`
 * - Contact pressure: `if_then_else(gap > 0, contact_force, 0)`
 * - Sub-gradient differentiation of `max` / `min` (closes the gap left
 *   by #137 — see `MaxDiffThrowsUntilIfThenElseLands`).
 *
 * Construction-time simplifications (applied in `if_then_else()` in
 * `scalar_std.h`):
 * - `if_then_else(scalar_zero, a, b) → b`
 * - `if_then_else(scalar_one, a, b) → a`
 * - `if_then_else(scalar_constant{c}, a, b) → a if c != 0 else b`
 *   (also fires for `scalar_negative(scalar_constant{c})` and any
 *   expression that `try_extract_scalar_number` resolves to a numeric
 *   value — the fold is a numeric check, not a structural one)
 * - `if_then_else(cond, a, a) → a` (then and else identical)
 *
 * Differentiation assumes the condition does not depend on the
 * differentiation variable:
 * ```
 *   d/dx if_then_else(cond, a(x), b(x)) = if_then_else(cond, da/dx, db/dx)
 * ```
 * For conditions that do depend on x the derivative strictly has Dirac
 * contributions at the boundary; this is acceptable because the
 * constitutive-model use case (yield function, contact gap) hits the
 * boundary on a measure-zero set in time evolution.
 *
 * Closes part of #135.
 */
class scalar_if_then_else final
    : public ternary_op<scalar_node_base_t<scalar_if_then_else>> {
public:
  using base = ternary_op<scalar_node_base_t<scalar_if_then_else>>;

  using base::base;
  scalar_if_then_else(scalar_if_then_else const &expr)
      : base(static_cast<base const &>(expr)) {}
  scalar_if_then_else(scalar_if_then_else &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  scalar_if_then_else() = delete;
  ~scalar_if_then_else() override = default;
  const scalar_if_then_else &operator=(scalar_if_then_else &&) = delete;
};

} // namespace numsim::cas

#endif // SCALAR_IF_THEN_ELSE_H
