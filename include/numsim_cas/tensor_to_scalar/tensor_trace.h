#ifndef TENSOR_TRACE_H
#define TENSOR_TRACE_H

#include "../unary_op.h"
#include "tensor_to_scalar_expression.h"

namespace numsim::cas {

class tensor_trace final
    : public unary_op<tensor_trace, tensor_to_scalar_expression,
                      tensor_expression> {
public:
  using base =
      unary_op<tensor_trace, tensor_to_scalar_expression, tensor_expression>;

  using base::base;
  tensor_trace(tensor_trace const &expr)
      : base(static_cast<base const &>(expr)) {}
  tensor_trace(tensor_trace &&expr)
      : base(std::move(static_cast<base &&>(expr))) {}
  tensor_trace() = delete;
  ~tensor_trace() = default;
  const tensor_trace &operator=(tensor_trace &&) = delete;

private:
};
} // namespace numsim::cas

#endif // TENSOR_TRACE_H
