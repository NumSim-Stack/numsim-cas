#ifndef SCALAR_GE_H
#define SCALAR_GE_H

#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

// Scalar greater-or-equal comparison `a >= b`. Evaluates to 1.0 / 0.0.
// Constructed via `ge(a, b)`.
class scalar_ge final : public binary_op<scalar_node_base_t<scalar_ge>> {
public:
  using base = binary_op<scalar_node_base_t<scalar_ge>>;

  using base::base;
  scalar_ge(scalar_ge const &expr) : base(static_cast<base const &>(expr)) {}
  scalar_ge(scalar_ge &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  scalar_ge() = delete;
  ~scalar_ge() override = default;
  const scalar_ge &operator=(scalar_ge &&) = delete;
};

} // namespace numsim::cas

#endif // SCALAR_GE_H
