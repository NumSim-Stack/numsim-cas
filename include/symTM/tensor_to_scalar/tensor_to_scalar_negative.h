#ifndef TENSOR_TO_SCALAR_NEGATIVE_H
#define TENSOR_TO_SCALAR_NEGATIVE_H

#include "../unary_op.h"

namespace symTM {

template<typename ValueType>
class tensor_to_scalar_negative final : public unary_op<tensor_to_scalar_negative<ValueType>, tensor_to_scalar_expression<ValueType>>{
public:
  using base = unary_op<tensor_to_scalar_negative<ValueType>, tensor_to_scalar_expression<ValueType>>;

  using base::base;
  tensor_to_scalar_negative(tensor_to_scalar_negative&&data):base(std::move(static_cast<base&&>(data))){}
  tensor_to_scalar_negative(tensor_to_scalar_negative const &data):base(static_cast<base const&>(data)){}
  tensor_to_scalar_negative() = delete;
  ~tensor_to_scalar_negative() = default;
  const tensor_to_scalar_negative &operator=(tensor_to_scalar_negative &&) = delete;
};

} // NAMESPACE symTM

#endif // TENSOR_TO_SCALAR_NEGATIVE_H
