#ifndef SCALAR_TAN_H
#define SCALAR_TAN_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_tan final : public unary_op<scalar_node_base_t<scalar_tan>> {
public:
  using base = unary_op<scalar_node_base_t<scalar_tan>>;
  using base::base;
  scalar_tan(scalar_tan const &expr) : base(static_cast<base const &>(expr)) {}
  scalar_tan(scalar_tan &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  scalar_tan() = delete;
  ~scalar_tan() override = default;
  const scalar_tan &operator=(scalar_tan &&) = delete;

private:
};
} // namespace numsim::cas

#endif // SCALAR_TAN_H
