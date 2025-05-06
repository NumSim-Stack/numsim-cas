#ifndef TENSOR_TO_SCALAR_DIV_H
#define TENSOR_TO_SCALAR_DIV_H

#include "../../../binary_op.h"
#include "../../../symTM_type_traits.h"
#include "../../tensor_to_scalar_expression.h"

namespace symTM {

template <typename ValueType>
class tensor_to_scalar_div final
    : public binary_op<tensor_to_scalar_div<ValueType>, tensor_to_scalar_expression<ValueType>>
{
public:
  using base = binary_op<tensor_to_scalar_div<ValueType>, tensor_to_scalar_expression<ValueType>>;
  using base_expr = expression_crtp<tensor_to_scalar_div<ValueType>, tensor_to_scalar_expression<ValueType>>;
  using base::base;

  tensor_to_scalar_div():base(){}
  ~tensor_to_scalar_div() = default;
  tensor_to_scalar_div(tensor_to_scalar_div const& data):base(static_cast<base const&>(data)){}
  tensor_to_scalar_div(tensor_to_scalar_div && data):base(std::forward<base>(data)){}

  const tensor_to_scalar_div &operator=(tensor_to_scalar_div &&) = delete;
};

} // namespace symTM

#endif // TENSOR_TO_SCALAR_DIV_H
