#ifndef SCALAR_SIN_H
#define SCALAR_SIN_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_sin final : public unary_op<scalar_node_base_t<scalar_sin>> {
public:
  using base = unary_op<scalar_node_base_t<scalar_sin>>;

  using base::base;
  scalar_sin(scalar_sin const &expr) : base(static_cast<base const &>(expr)) {}
<<<<<<< HEAD
  scalar_sin(scalar_sin &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
=======
  scalar_sin(scalar_sin &&expr) noexcept : base(std::move(static_cast<base &&>(expr))) {}
>>>>>>> origin/move_to_virtual
  scalar_sin() = delete;
  ~scalar_sin() override = default;
  const scalar_sin &operator=(scalar_sin &&) = delete;

private:
};
} // namespace numsim::cas

#endif // SCALAR_SIN_H
