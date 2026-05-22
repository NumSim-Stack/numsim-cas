#ifndef SCALAR_NE_H
#define SCALAR_NE_H

#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

// Scalar numerical inequality `a != b`. Evaluates to 1.0 / 0.0.
// Constructed via `ne(a, b)`.
class scalar_ne final : public binary_op<scalar_node_base_t<scalar_ne>> {
public:
  using base = binary_op<scalar_node_base_t<scalar_ne>>;

  using base::base;
  scalar_ne(scalar_ne const &expr) : base(static_cast<base const &>(expr)) {}
  scalar_ne(scalar_ne &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  scalar_ne() = delete;
  ~scalar_ne() override = default;
  const scalar_ne &operator=(scalar_ne &&) = delete;
};

} // namespace numsim::cas

#endif // SCALAR_NE_H
