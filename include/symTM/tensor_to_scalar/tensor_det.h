#ifndef TENSOR_DET_H
#define TENSOR_DET_H

#include "../unary_op.h"
#include "tensor_to_scalar_expression.h"

namespace symTM {
template <typename ValueType>
class tensor_det final : public unary_op<tensor_det<ValueType>, tensor_to_scalar_expression<ValueType>, tensor_expression<ValueType>>
{
public:
  using base = unary_op<tensor_det<ValueType>, tensor_to_scalar_expression<ValueType>, tensor_expression<ValueType>>;

  using base::base;
  tensor_det(tensor_det const& expr):base(static_cast<base const&>(expr)) {}
  tensor_det(tensor_det && expr):base(std::move(static_cast<base &&>(expr))) {}
  tensor_det() = delete;
  ~tensor_det() = default;
  const tensor_det &operator=(tensor_det &&) = delete;
private:
};
}

#endif // TENSOR_DET_H
