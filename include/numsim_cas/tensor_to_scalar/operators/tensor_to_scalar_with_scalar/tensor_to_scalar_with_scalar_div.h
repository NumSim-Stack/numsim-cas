#ifndef TENSOR_TO_SCALAR_WITH_SCALAR_DIV_H
#define TENSOR_TO_SCALAR_WITH_SCALAR_DIV_H

#include "../../../binary_op.h"
#include "../../../numsim_cas_type_traits.h"
#include "../../../scalar/scalar_expression.h"
#include "../../tensor_to_scalar_expression.h"

namespace numsim::cas {

template <typename ValueType>
class tensor_to_scalar_with_scalar_div final
    : public binary_op<tensor_to_scalar_with_scalar_div<ValueType>,
                       tensor_to_scalar_expression<ValueType>,
                       tensor_to_scalar_expression<ValueType>,
                       scalar_expression<ValueType>> {
public:
  using base = binary_op<tensor_to_scalar_with_scalar_div<ValueType>,
                         tensor_to_scalar_expression<ValueType>,
                         tensor_to_scalar_expression<ValueType>,
                         scalar_expression<ValueType>>;
  using base::base;

  tensor_to_scalar_with_scalar_div() : base() {}
  ~tensor_to_scalar_with_scalar_div() = default;
  tensor_to_scalar_with_scalar_div(tensor_to_scalar_with_scalar_div const &data)
      : base(static_cast<base const &>(data)) {}
  tensor_to_scalar_with_scalar_div(tensor_to_scalar_with_scalar_div &&data)
      : base(std::forward<base>(data)) {}
  const tensor_to_scalar_with_scalar_div &
  operator=(tensor_to_scalar_with_scalar_div &&) = delete;
};

template <typename ValueType>
class scalar_with_tensor_to_scalar_div final
    : public binary_op<scalar_with_tensor_to_scalar_div<ValueType>,
                       tensor_to_scalar_expression<ValueType>,
                       scalar_expression<ValueType>,
                       tensor_to_scalar_expression<ValueType>> {
public:
  using base = binary_op<scalar_with_tensor_to_scalar_div<ValueType>,
                         tensor_to_scalar_expression<ValueType>,
                         scalar_expression<ValueType>,
                         tensor_to_scalar_expression<ValueType>>;
  using base::base;

  scalar_with_tensor_to_scalar_div() : base() {}
  ~scalar_with_tensor_to_scalar_div() = default;
  scalar_with_tensor_to_scalar_div(scalar_with_tensor_to_scalar_div const &data)
      : base(static_cast<base const &>(data)) {}
  scalar_with_tensor_to_scalar_div(scalar_with_tensor_to_scalar_div &&data)
      : base(std::forward<base>(data)) {}
  const scalar_with_tensor_to_scalar_div &
  operator=(scalar_with_tensor_to_scalar_div &&) = delete;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_WITH_SCALAR_DIV_H
