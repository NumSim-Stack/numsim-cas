#ifndef TENSOR_TO_SCALAR_SCALAR_ADD_H
#define TENSOR_TO_SCALAR_SCALAR_ADD_H

#include "../../../binary_op.h"
#include "../../../numsim_cas_type_traits.h"
#include "../../../operators.h"
#include "../../tensor_to_scalar_expression.h"

namespace numsim::cas {

class tensor_to_scalar_with_scalar_add final
    : public binary_op<tensor_to_scalar_with_scalar_add,
                       tensor_to_scalar_expression, scalar_expression,
                       tensor_to_scalar_expression> {
public:
  using base =
      binary_op<tensor_to_scalar_with_scalar_add, tensor_to_scalar_expression,
                scalar_expression, tensor_to_scalar_expression>;
  using base::base;

  tensor_to_scalar_with_scalar_add() : base() {}
  ~tensor_to_scalar_with_scalar_add() = default;
  tensor_to_scalar_with_scalar_add(tensor_to_scalar_with_scalar_add const &data)
      : base(static_cast<base const &>(data)) {}
  tensor_to_scalar_with_scalar_add(tensor_to_scalar_with_scalar_add &&data)
      : base(std::forward<base>(data)) {}
  const tensor_to_scalar_with_scalar_add &
  operator=(tensor_to_scalar_with_scalar_add &&) = delete;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_SCALAR_ADD_H
