#ifndef TENSOR_DOT_H
#define TENSOR_DOT_H

#include "../unary_op.h"
#include "tensor_to_scalar_expression.h"

namespace numsim::cas {
template <typename ValueType>
class tensor_dot final : public unary_op<tensor_dot<ValueType>,
                                         tensor_to_scalar_expression<ValueType>,
                                         tensor_expression<ValueType>> {
public:
  using base =
      unary_op<tensor_dot<ValueType>, tensor_to_scalar_expression<ValueType>,
               tensor_expression<ValueType>>;

  using base::base;
  tensor_dot(tensor_dot const &expr) : base(static_cast<base const &>(expr)) {}
  tensor_dot(tensor_dot &&expr) : base(std::move(static_cast<base &&>(expr))) {}
  tensor_dot() = delete;
  ~tensor_dot() = default;
  const tensor_dot &operator=(tensor_dot &&) = delete;

private:
};
} // namespace numsim::cas

#endif // TENSOR_DOT_H
