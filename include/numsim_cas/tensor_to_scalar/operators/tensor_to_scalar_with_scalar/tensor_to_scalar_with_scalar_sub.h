#ifndef TENSOR_TO_SCALAR_SCALAR_SUB_H
#define TENSOR_TO_SCALAR_SCALAR_SUB_H

#include "../../../binary_op.h"
#include "../../../numsim_cas_type_traits.h"
#include "../../../scalar/scalar_expression.h"
#include "../../tensor_to_scalar_expression.h"

namespace numsim::cas {

template <typename ValueType>
class tensor_to_scalar_with_scalar_sub final
    : public binary_op<tensor_to_scalar_with_scalar_sub<ValueType>,
                       scalar_expression<ValueType>,
                       tensor_to_scalar_expression<ValueType>> {
public:
  using base = binary_op<tensor_to_scalar_with_scalar_sub<ValueType>,
                         scalar_expression<ValueType>,
                         tensor_to_scalar_expression<ValueType>>;
  using base::base;

  tensor_to_scalar_with_scalar_sub() : base() {}
  ~tensor_to_scalar_with_scalar_sub() = default;
  tensor_to_scalar_with_scalar_sub(tensor_to_scalar_with_scalar_sub const &data)
      : base(static_cast<base const &>(data)) {}
  tensor_to_scalar_with_scalar_sub(tensor_to_scalar_with_scalar_sub &&data)
      : base(std::forward<base>(data)) {}
  const tensor_to_scalar_with_scalar_sub &
  operator=(tensor_to_scalar_with_scalar_sub &&) = delete;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_SCALAR_SUB_H
