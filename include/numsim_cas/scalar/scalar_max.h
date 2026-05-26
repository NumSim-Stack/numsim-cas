#ifndef SCALAR_MAX_H
#define SCALAR_MAX_H

#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

/**
 * @class scalar_max
 * @brief Two-argument maximum: `max(a, b)`.
 *
 * Models `std::max(a, b)` as a CAS node. Useful for:
 * - **Macauley bracket** `<x>+ = max(x, 0)` (damage activation, ramp yield).
 * - **Clamping** `max(lo, min(hi, x))` (bounded internal variables).
 * - **Stress measures** like `max(|σ_1|, |σ_2|, |σ_3|)`.
 *
 * Equivalent in expressive power to `if_then_else(a > b, a, b)` (#135) but
 * a dedicated node gives:
 * - cleaner printed form (`max(a, b)` vs `a > b ? a : b`),
 * - pattern-matchable construction-time simplifications (`max(x, x) → x`,
 *   `max(c1, c2) → numeric`, idempotence with self-composition),
 * - direct codegen target (`std::max(a, b)`).
 *
 * Differentiation uses the sub-gradient at the boundary `a == b`. Will be
 * upgraded to `if_then_else(a > b, da/dx, db/dx)` once #135 lands.
 *
 * Closes part of #137.
 */
class scalar_max final : public binary_op<scalar_node_base_t<scalar_max>> {
public:
  using base = binary_op<scalar_node_base_t<scalar_max>>;

  using base::base;
  scalar_max(scalar_max const &expr) : base(static_cast<base const &>(expr)) {}
  scalar_max(scalar_max &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  scalar_max() = delete;
  ~scalar_max() override = default;
  const scalar_max &operator=(scalar_max &&) = delete;
};

} // namespace numsim::cas

#endif // SCALAR_MAX_H
