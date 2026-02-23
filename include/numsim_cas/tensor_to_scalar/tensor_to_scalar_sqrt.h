#ifndef TENSOR_TO_SCALAR_SQRT_H
#define TENSOR_TO_SCALAR_SQRT_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

class tensor_to_scalar_sqrt final
    : public unary_op<tensor_to_scalar_node_base_t<tensor_to_scalar_sqrt>> {
public:
  using base = unary_op<tensor_to_scalar_node_base_t<tensor_to_scalar_sqrt>>;

  using base::base;
  tensor_to_scalar_sqrt(tensor_to_scalar_sqrt const &expr)
      : base(static_cast<base const &>(expr)) {}
  tensor_to_scalar_sqrt(tensor_to_scalar_sqrt &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  tensor_to_scalar_sqrt() = delete;
  ~tensor_to_scalar_sqrt() override = default;
  const tensor_to_scalar_sqrt &operator=(tensor_to_scalar_sqrt &&) = delete;

private:
};
} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_SQRT_H
