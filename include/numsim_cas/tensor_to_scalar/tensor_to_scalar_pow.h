#ifndef TENSOR_TO_SCALAR_POW_H
#define TENSOR_TO_SCALAR_POW_H

#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

class tensor_to_scalar_pow final
    : public binary_op<tensor_to_scalar_node_base_t<tensor_to_scalar_pow>> {
public:
  using base = binary_op<tensor_to_scalar_node_base_t<tensor_to_scalar_pow>>;

  using base::base;
  tensor_to_scalar_pow(tensor_to_scalar_pow const &expr)
      : base(static_cast<base const &>(expr)) {}
  tensor_to_scalar_pow(tensor_to_scalar_pow &&expr)
      : base(std::move(static_cast<base &&>(expr))) {}
  tensor_to_scalar_pow() = delete;
  ~tensor_to_scalar_pow() = default;
  const tensor_to_scalar_pow &operator=(tensor_to_scalar_pow &&) = delete;
};

// class tensor_to_scalar_pow_with_scalar_exponent final
//     : public binary_op<tensor_to_scalar_node_base_t<tensor_to_scalar_pow>,
//                        tensor_to_scalar_expression, scalar_expression> {
// public:
//   using base = binary_op<tensor_to_scalar_node_base_t<tensor_to_scalar_pow>,
//                          tensor_to_scalar_expression, scalar_expression>;

//   using base::base;
//   tensor_to_scalar_pow_with_scalar_exponent(
//       tensor_to_scalar_pow_with_scalar_exponent const &expr)
//       : base(static_cast<base const &>(expr)) {}
//   tensor_to_scalar_pow_with_scalar_exponent(
//       tensor_to_scalar_pow_with_scalar_exponent &&expr)
//       : base(std::move(static_cast<base &&>(expr))) {}
//   tensor_to_scalar_pow_with_scalar_exponent() = delete;
//   ~tensor_to_scalar_pow_with_scalar_exponent() = default;
//   const tensor_to_scalar_pow_with_scalar_exponent &
//   operator=(tensor_to_scalar_pow_with_scalar_exponent &&) = delete;

// private:
// };
} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_POW_H
