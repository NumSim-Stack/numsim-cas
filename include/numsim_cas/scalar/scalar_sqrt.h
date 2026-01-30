#ifndef SCALAR_SQRT_H
#define SCALAR_SQRT_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_sqrt final : public unary_op<scalar_node_base_t<scalar_sqrt>> {
public:
  using base = unary_op<scalar_node_base_t<scalar_sqrt>>;

  using base::base;
  scalar_sqrt(scalar_sqrt const &expr)
      : base(static_cast<base const &>(expr)) {}
  scalar_sqrt(scalar_sqrt &&expr)
      : base(std::move(static_cast<base &&>(expr))) {}
  scalar_sqrt() = delete;
  ~scalar_sqrt() = default;
  const scalar_sqrt &operator=(scalar_sqrt &&) = delete;

private:
};
} // namespace numsim::cas

#endif // SCALAR_SQRT_H
