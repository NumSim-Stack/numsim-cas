#ifndef SCALAR_ACOS_H
#define SCALAR_ACOS_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {
class scalar_acos final : public unary_op<scalar_node_base_t<scalar_acos>> {
public:
  using base = unary_op<scalar_node_base_t<scalar_acos>>;

  using base::base;
  scalar_acos(scalar_acos const &expr)
      : base(static_cast<base const &>(expr)) {}
  scalar_acos(scalar_acos &&expr)
      : base(std::move(static_cast<base &&>(expr))) {}
  scalar_acos() = delete;
  ~scalar_acos() = default;
  const scalar_acos &operator=(scalar_acos &&) = delete;

private:
};

} // namespace numsim::cas

#endif // SCALAR_ACOS_H
