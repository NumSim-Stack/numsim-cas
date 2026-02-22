#ifndef TENSOR_TO_SCALAR_EXP_H
#define TENSOR_TO_SCALAR_EXP_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

class tensor_to_scalar_exp final
    : public unary_op<tensor_to_scalar_node_base_t<tensor_to_scalar_exp>> {
public:
  using base = unary_op<tensor_to_scalar_node_base_t<tensor_to_scalar_exp>>;

  using base::base;
  tensor_to_scalar_exp(tensor_to_scalar_exp const &expr)
      : base(static_cast<base const &>(expr)) {}
  tensor_to_scalar_exp(tensor_to_scalar_exp &&expr)
      : base(std::move(static_cast<base &&>(expr))) {}
  tensor_to_scalar_exp() = delete;
  ~tensor_to_scalar_exp() = default;
  const tensor_to_scalar_exp &operator=(tensor_to_scalar_exp &&) = delete;

private:
};
} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_EXP_H
