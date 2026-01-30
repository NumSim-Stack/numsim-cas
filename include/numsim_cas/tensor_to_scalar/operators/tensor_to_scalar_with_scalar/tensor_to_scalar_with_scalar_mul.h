#ifndef TENSOR_TO_SCALAR_SCALAR_MUL_H
#define TENSOR_TO_SCALAR_SCALAR_MUL_H

#include "../../../binary_op.h"
#include "../../../numsim_cas_type_traits.h"
#include "../../../operators.h"
#include "../../../scalar/scalar_expression.h"
#include "../../tensor_to_scalar_expression.h"

namespace numsim::cas {

class tensor_to_scalar_with_scalar_mul final
    : public binary_op<tensor_to_scalar_with_scalar_mul,
                       tensor_to_scalar_expression, scalar_expression,
                       tensor_to_scalar_expression> {
public:
  using base =
      binary_op<tensor_to_scalar_with_scalar_mul, tensor_to_scalar_expression,
                scalar_expression, tensor_to_scalar_expression>;
  using base::base;

  tensor_to_scalar_with_scalar_mul() : base() {}
  ~tensor_to_scalar_with_scalar_mul() = default;
  tensor_to_scalar_with_scalar_mul(tensor_to_scalar_with_scalar_mul const &data)
      : base(static_cast<base const &>(data)) {}
  tensor_to_scalar_with_scalar_mul(tensor_to_scalar_with_scalar_mul &&data)
      : base(std::forward<base>(data)) {}
  const tensor_to_scalar_with_scalar_mul &
  operator=(tensor_to_scalar_with_scalar_mul &&) = delete;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_SCALAR_MUL_H
