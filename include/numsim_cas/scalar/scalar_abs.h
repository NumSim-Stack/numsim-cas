#ifndef SCALAR_ABS_H
#define SCALAR_ABS_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {
class scalar_abs final : public unary_op<scalar_node_base_t<scalar_abs>> {
public:
  using base = unary_op<scalar_node_base_t<scalar_abs>>;
  using base::base;
  scalar_abs(scalar_abs const &expr) : base(static_cast<base const &>(expr)) {}
  scalar_abs(scalar_abs &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  scalar_abs() = delete;
  ~scalar_abs() override = default;
  const scalar_abs &operator=(scalar_abs &&) = delete;
};

} // namespace numsim::cas

#endif // SCALAR_ABS_H
