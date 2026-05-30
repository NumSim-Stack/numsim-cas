#ifndef SCALAR_MIN_H
#define SCALAR_MIN_H

#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

/**
 * @class scalar_min
 * @brief Two-argument minimum: `min(a, b)`.
 *
 * Symmetric counterpart to `scalar_max`. See `scalar_max.h` for the design
 * rationale (dedicated node vs. derivation from `if_then_else`) and for
 * the `using namespace` clash note (the same ADL caveat applies here
 * vs. `std::min`).
 *
 * Differentiation requires `if_then_else` (#135) to express the piecewise
 * derivative symbolically. Until #135 lands, the diff visitor throws
 * `not_implemented_error`. Once available, the rule is
 * `d/dx min(a, b) = if_then_else(a < b, da/dx, db/dx)`, with the sub-gradient
 * picking one side at the measure-zero boundary.
 *
 * Closes part of #137.
 */
class scalar_min final : public binary_op<scalar_node_base_t<scalar_min>> {
public:
  using base = binary_op<scalar_node_base_t<scalar_min>>;

  using base::base;
  scalar_min(scalar_min const &expr) : base(static_cast<base const &>(expr)) {}
  scalar_min(scalar_min &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  scalar_min() = delete;
  ~scalar_min() override = default;
  const scalar_min &operator=(scalar_min &&) = delete;
};

} // namespace numsim::cas

#endif // SCALAR_MIN_H
