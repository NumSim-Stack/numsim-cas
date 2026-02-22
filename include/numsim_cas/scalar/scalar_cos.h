#ifndef SCALAR_COS_H
#define SCALAR_COS_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_cos final : public unary_op<scalar_node_base_t<scalar_cos>> {
public:
  using base = unary_op<scalar_node_base_t<scalar_cos>>;

  using base::base;
  scalar_cos(scalar_cos const &expr) : base(static_cast<base const &>(expr)) {}
<<<<<<< HEAD
  scalar_cos(scalar_cos &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
=======
  scalar_cos(scalar_cos &&expr) noexcept : base(std::move(static_cast<base &&>(expr))) {}
>>>>>>> origin/move_to_virtual
  scalar_cos() = delete;
  ~scalar_cos() override = default;
  const scalar_cos &operator=(scalar_cos &&) = delete;

private:
};
} // namespace numsim::cas
#endif // SCALAR_COS_H
