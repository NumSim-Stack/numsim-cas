#ifndef TENSOR_SCALAR_DIV_H
#define TENSOR_SCALAR_DIV_H

#include "../../../binary_op.h"
#include "../../../numsim_cas_type_traits.h"
#include <stdexcept>

namespace numsim::cas {

class tensor_scalar_div final
    : public binary_op<tensor_scalar_div, tensor_expression, tensor_expression,
                       scalar_expression> {
public:
  using base = binary_op<tensor_scalar_div, tensor_expression,
                         tensor_expression, scalar_expression>;

  template <typename LHS, typename RHS>
  tensor_scalar_div(LHS &&lhs, RHS &&rhs)
      : base(std::forward<LHS>(lhs), std::forward<RHS>(rhs),
             call_tensor::dim(lhs), call_tensor::rank(lhs)) {}
};

} // namespace numsim::cas

#endif // TENSOR_SCALAR_DIV_H
