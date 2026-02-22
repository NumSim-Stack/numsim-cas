#ifndef SCALAR_LOG_H
#define SCALAR_LOG_H

#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_log final : public unary_op<scalar_node_base_t<scalar_log>> {
public:
  using base = unary_op<scalar_node_base_t<scalar_log>>;

  using base::base;
  scalar_log(scalar_log const &expr) : base(static_cast<base const &>(expr)) {}
<<<<<<< HEAD
  scalar_log(scalar_log &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
=======
  scalar_log(scalar_log &&expr) noexcept : base(std::move(static_cast<base &&>(expr))) {}
>>>>>>> origin/move_to_virtual
  scalar_log() = delete;
  ~scalar_log() override = default;
  const scalar_log &operator=(scalar_log &&) = delete;

private:
};
} // namespace numsim::cas

#endif // SCALAR_LOG_H
