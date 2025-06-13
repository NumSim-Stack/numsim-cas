#ifndef TENSOR_NORM_H
#define TENSOR_NORM_H

#include "../unary_op.h"
#include "tensor_to_scalar_expression.h"

namespace numsim::cas {
template <typename ValueType>
class tensor_norm final
    : public unary_op<tensor_norm<ValueType>,
                      tensor_to_scalar_expression<ValueType>,
                      tensor_expression<ValueType>> {
public:
  using base =
      unary_op<tensor_norm<ValueType>, tensor_to_scalar_expression<ValueType>,
               tensor_expression<ValueType>>;

  using base::base;
  tensor_norm(tensor_norm const &expr)
      : base(static_cast<base const &>(expr)) {}
  tensor_norm(tensor_norm &&expr)
      : base(std::move(static_cast<base &&>(expr))) {}
  tensor_norm() = delete;
  ~tensor_norm() = default;
  const tensor_norm &operator=(tensor_norm &&) = delete;

private:
};
} // namespace numsim::cas

#endif // TENSOR_NORM_H
