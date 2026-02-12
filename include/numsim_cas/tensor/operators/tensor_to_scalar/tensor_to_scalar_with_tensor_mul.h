#ifndef TENSOR_TO_SCALAR_TENSOR_MUL_H
#define TENSOR_TO_SCALAR_TENSOR_MUL_H

#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

class tensor_to_scalar_with_tensor_mul final
    : public binary_op<tensor_node_base_t<tensor_to_scalar_with_tensor_mul>,
                       tensor_expression, tensor_to_scalar_expression> {
public:
  using base = binary_op<tensor_node_base_t<tensor_to_scalar_with_tensor_mul>,
                         tensor_expression, tensor_to_scalar_expression>;
  using base::base;

  tensor_to_scalar_with_tensor_mul(tensor_to_scalar_with_tensor_mul const &data)
      : base(static_cast<base const &>(data)) {}
  tensor_to_scalar_with_tensor_mul(tensor_to_scalar_with_tensor_mul &&data)
      : base(std::forward<base>(data)) {}
  const tensor_to_scalar_with_tensor_mul &
  operator=(tensor_to_scalar_with_tensor_mul &&) = delete;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_TENSOR_MUL_H
