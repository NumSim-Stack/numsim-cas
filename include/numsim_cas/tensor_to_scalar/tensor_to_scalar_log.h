#ifndef TENSOR_TO_SCALAR_LOG_H
#define TENSOR_TO_SCALAR_LOG_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

class tensor_to_scalar_log final
    : public unary_op<tensor_to_scalar_node_base_t<tensor_to_scalar_log>> {
public:
  using base = unary_op<tensor_to_scalar_node_base_t<tensor_to_scalar_log>>;

  using base::base;
  tensor_to_scalar_log(tensor_to_scalar_log const &expr)
      : base(static_cast<base const &>(expr)) {}
  tensor_to_scalar_log(tensor_to_scalar_log &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  tensor_to_scalar_log() = delete;
  ~tensor_to_scalar_log() override = default;
  const tensor_to_scalar_log &operator=(tensor_to_scalar_log &&) = delete;

private:
};
} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_LOG_H
