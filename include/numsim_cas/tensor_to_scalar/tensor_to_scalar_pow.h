#ifndef TENSOR_TO_SCALAR_POW_H
#define TENSOR_TO_SCALAR_POW_H

#include "../binary_op.h"
#include "../scalar/scalar_expression.h"
#include "tensor_to_scalar_expression.h"

namespace numsim::cas {
template <typename ValueType>
class tensor_to_scalar_pow final
    : public binary_op<tensor_to_scalar_pow<ValueType>,
                       tensor_to_scalar_expression<ValueType>> {
public:
  using base = binary_op<tensor_to_scalar_pow<ValueType>,
                         tensor_to_scalar_expression<ValueType>>;

  using base::base;
  tensor_to_scalar_pow(tensor_to_scalar_pow const &expr)
      : base(static_cast<base const &>(expr)) {}
  tensor_to_scalar_pow(tensor_to_scalar_pow &&expr)
      : base(std::move(static_cast<base &&>(expr))) {}
  tensor_to_scalar_pow() = delete;
  ~tensor_to_scalar_pow() = default;
  const tensor_to_scalar_pow &operator=(tensor_to_scalar_pow &&) = delete;
};

template <typename ValueType>
class tensor_to_scalar_pow_with_scalar_exponent final
    : public binary_op<tensor_to_scalar_pow_with_scalar_exponent<ValueType>,
                       tensor_to_scalar_expression<ValueType>,
                       scalar_expression<ValueType>> {
public:
  using base = binary_op<tensor_to_scalar_pow_with_scalar_exponent<ValueType>,
                         tensor_to_scalar_expression<ValueType>,
                         scalar_expression<ValueType>>;

  using base::base;
  tensor_to_scalar_pow_with_scalar_exponent(
      tensor_to_scalar_pow_with_scalar_exponent const &expr)
      : base(static_cast<base const &>(expr)) {}
  tensor_to_scalar_pow_with_scalar_exponent(
      tensor_to_scalar_pow_with_scalar_exponent &&expr)
      : base(std::move(static_cast<base &&>(expr))) {}
  tensor_to_scalar_pow_with_scalar_exponent() = delete;
  ~tensor_to_scalar_pow_with_scalar_exponent() = default;
  const tensor_to_scalar_pow_with_scalar_exponent &
  operator=(tensor_to_scalar_pow_with_scalar_exponent &&) = delete;

private:
};
} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_POW_H
