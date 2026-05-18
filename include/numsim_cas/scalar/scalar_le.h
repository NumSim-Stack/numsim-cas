#ifndef SCALAR_LE_H
#define SCALAR_LE_H

#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

// Scalar less-or-equal comparison `a <= b`. Evaluates to 1.0 / 0.0.
// Constructed via `le(a, b)`.
class scalar_le final : public binary_op<scalar_node_base_t<scalar_le>> {
public:
  using base = binary_op<scalar_node_base_t<scalar_le>>;

  using base::base;
  scalar_le(scalar_le const &expr) : base(static_cast<base const &>(expr)) {}
  scalar_le(scalar_le &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  scalar_le() = delete;
  ~scalar_le() override = default;
  const scalar_le &operator=(scalar_le &&) = delete;
};

} // namespace numsim::cas

#endif // SCALAR_LE_H
