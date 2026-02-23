#ifndef TENSOR_TRACE_H
#define TENSOR_TRACE_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

class tensor_trace final
    : public unary_op<tensor_to_scalar_node_base_t<tensor_trace>,
                      tensor_expression> {
public:
  using base =
      unary_op<tensor_to_scalar_node_base_t<tensor_trace>, tensor_expression>;

  using base::base;
  tensor_trace(tensor_trace const &expr)
      : base(static_cast<base const &>(expr)) {}
  tensor_trace(tensor_trace &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  tensor_trace() = delete;
  ~tensor_trace() override = default;
  const tensor_trace &operator=(tensor_trace &&) = delete;

private:
};
} // namespace numsim::cas

#endif // TENSOR_TRACE_H
