#ifndef TENSOR_TO_SCALAR_NEGATIVE_H
#define TENSOR_TO_SCALAR_NEGATIVE_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

class tensor_to_scalar_negative final
    : public unary_op<tensor_to_scalar_node_base_t<tensor_to_scalar_negative>> {
public:
  using base =
      unary_op<tensor_to_scalar_node_base_t<tensor_to_scalar_negative>>;

  using base::base;
  tensor_to_scalar_negative(tensor_to_scalar_negative &&data) noexcept
      : base(std::move(static_cast<base &&>(data))) {}
  tensor_to_scalar_negative(tensor_to_scalar_negative const &data)
      : base(static_cast<base const &>(data)) {}
  tensor_to_scalar_negative() = delete;
  ~tensor_to_scalar_negative() override = default;
  const tensor_to_scalar_negative &
  operator=(tensor_to_scalar_negative &&) = delete;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_NEGATIVE_H
