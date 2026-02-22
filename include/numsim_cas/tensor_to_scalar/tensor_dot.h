#ifndef TENSOR_DOT_H
#define TENSOR_DOT_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

class tensor_dot final
    : public unary_op<tensor_to_scalar_node_base_t<tensor_dot>,
                      tensor_expression> {
public:
  using base =
      unary_op<tensor_to_scalar_node_base_t<tensor_dot>, tensor_expression>;

  using base::base;
  tensor_dot(tensor_dot const &expr) : base(static_cast<base const &>(expr)) {}
  tensor_dot(tensor_dot &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  tensor_dot() = delete;
  ~tensor_dot() override = default;
  const tensor_dot &operator=(tensor_dot &&) = delete;
};
} // namespace numsim::cas

#endif // TENSOR_DOT_H
