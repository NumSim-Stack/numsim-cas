#ifndef SCALAR_GT_H
#define SCALAR_GT_H

#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

// Scalar greater-than comparison `a > b`. Evaluates to 1.0 / 0.0.
// Constructed via `gt(a, b)`.
class scalar_gt final : public binary_op<scalar_node_base_t<scalar_gt>> {
public:
  using base = binary_op<scalar_node_base_t<scalar_gt>>;

  using base::base;
  scalar_gt(scalar_gt const &expr) : base(static_cast<base const &>(expr)) {}
  scalar_gt(scalar_gt &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  scalar_gt() = delete;
  ~scalar_gt() override = default;
  const scalar_gt &operator=(scalar_gt &&) = delete;
};

} // namespace numsim::cas

#endif // SCALAR_GT_H
