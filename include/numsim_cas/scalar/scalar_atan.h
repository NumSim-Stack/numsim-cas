#ifndef SCALAR_ATAN_H
#define SCALAR_ATAN_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_atan final : public unary_op<scalar_node_base_t<scalar_atan>> {
public:
  using base = unary_op<scalar_node_base_t<scalar_atan>>;

  using base::base;
  scalar_atan(scalar_atan const &expr)
      : base(static_cast<base const &>(expr)) {}
  scalar_atan(scalar_atan &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  scalar_atan() = delete;
  ~scalar_atan() override = default;
  const scalar_atan &operator=(scalar_atan &&) = delete;

private:
};
} // namespace numsim::cas

#endif // SCALAR_ATAN_H
