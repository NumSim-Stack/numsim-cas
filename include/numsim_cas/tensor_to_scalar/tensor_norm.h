#ifndef TENSOR_NORM_H
#define TENSOR_NORM_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

class tensor_norm final
    : public unary_op<tensor_to_scalar_node_base_t<tensor_norm>,
                      tensor_expression> {
public:
  using base =
      unary_op<tensor_to_scalar_node_base_t<tensor_norm>, tensor_expression>;

  using base::base;
  tensor_norm(tensor_norm const &expr)
      : base(static_cast<base const &>(expr)) {}
  tensor_norm(tensor_norm &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  tensor_norm() = delete;
  ~tensor_norm() override = default;
  const tensor_norm &operator=(tensor_norm &&) = delete;

private:
};
} // namespace numsim::cas

#endif // TENSOR_NORM_H
