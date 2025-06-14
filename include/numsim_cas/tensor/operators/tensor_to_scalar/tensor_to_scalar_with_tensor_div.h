#ifndef TENSOR_TO_SCALAR_TENSOR_DIV_H
#define TENSOR_TO_SCALAR_TENSOR_DIV_H

#include "../../../binary_op.h"
#include "../../../tensor/tensor_expression.h"
#include "../../tensor_to_scalar_expression.h"

namespace numsim::cas {

template <typename ValueType>
class tensor_to_scalar_with_tensor_div final
    : public binary_op<tensor_to_scalar_with_tensor_div<ValueType>,
                       tensor_expression<ValueType>,
                       tensor_to_scalar_expression<ValueType>> {
public:
  using base = binary_op<tensor_to_scalar_with_tensor_div<ValueType>,
                         tensor_expression<ValueType>,
                         tensor_to_scalar_expression<ValueType>>;
  using base::base;

  tensor_to_scalar_with_tensor_div() : base() {}
  ~tensor_to_scalar_with_tensor_div() = default;
  tensor_to_scalar_with_tensor_div(tensor_to_scalar_with_tensor_div const &data)
      : base(static_cast<base const &>(data)) {}
  tensor_to_scalar_with_tensor_div(tensor_to_scalar_with_tensor_div &&data)
      : base(std::forward<base>(data)) {}
  const tensor_to_scalar_with_tensor_div &
  operator=(tensor_to_scalar_with_tensor_div &&) = delete;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_TENSOR_DIV_H
