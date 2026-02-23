#ifndef SCALAR_EXP_H
#define SCALAR_EXP_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_exp final : public unary_op<scalar_node_base_t<scalar_exp>> {
public:
  using base = unary_op<scalar_node_base_t<scalar_exp>>;

  using base::base;
  scalar_exp(scalar_exp const &expr) : base(static_cast<base const &>(expr)) {}
  scalar_exp(scalar_exp &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  scalar_exp() = delete;
  ~scalar_exp() override = default;
  const scalar_exp &operator=(scalar_exp &&) = delete;

private:
};
} // namespace numsim::cas

#endif // SCALAR_EXP_H
