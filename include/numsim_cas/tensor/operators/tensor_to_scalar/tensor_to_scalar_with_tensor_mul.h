#ifndef TENSOR_TO_SCALAR_TENSOR_MUL_H
#define TENSOR_TO_SCALAR_TENSOR_MUL_H

#include "../../../binary_op.h"
#include "../../../tensor/tensor_expression.h"
#include "../../tensor_to_scalar_expression.h"

namespace numsim::cas {

template <typename ValueType>
class tensor_to_scalar_with_tensor_mul final
    : public binary_op<tensor_to_scalar_with_tensor_mul<ValueType>,
                       tensor_expression<ValueType>,
                       tensor_to_scalar_expression<ValueType>,
                       tensor_expression<ValueType>> {
public:
  using base = binary_op<
      tensor_to_scalar_with_tensor_mul<ValueType>, tensor_expression<ValueType>,
      tensor_to_scalar_expression<ValueType>, tensor_expression<ValueType>>;
  using base::base;

  tensor_to_scalar_with_tensor_mul() : base() {}
  ~tensor_to_scalar_with_tensor_mul() = default;
  tensor_to_scalar_with_tensor_mul(tensor_to_scalar_with_tensor_mul const &data)
      : base(static_cast<base const &>(data)) {}
  tensor_to_scalar_with_tensor_mul(tensor_to_scalar_with_tensor_mul &&data)
      : base(std::forward<base>(data)) {}
  const tensor_to_scalar_with_tensor_mul &
  operator=(tensor_to_scalar_with_tensor_mul &&) = delete;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_TENSOR_MUL_H
