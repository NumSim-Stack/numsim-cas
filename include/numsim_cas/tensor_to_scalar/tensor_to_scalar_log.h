#ifndef TENSOR_TO_SCALAR_LOG_H
#define TENSOR_TO_SCALAR_LOG_H

#include "../unary_op.h"

namespace numsim::cas {
template <typename ValueType>
class tensor_to_scalar_log final
    : public unary_op<tensor_to_scalar_log<ValueType>,
                      tensor_to_scalar_expression<ValueType>> {
public:
  using base = unary_op<tensor_to_scalar_log<ValueType>,
                        tensor_to_scalar_expression<ValueType>>;

  using base::base;
  tensor_to_scalar_log(tensor_to_scalar_log const &expr)
      : base(static_cast<base const &>(expr)) {}
  tensor_to_scalar_log(tensor_to_scalar_log &&expr)
      : base(std::move(static_cast<base &&>(expr))) {}
  tensor_to_scalar_log() = delete;
  ~tensor_to_scalar_log() = default;
  const tensor_to_scalar_log &operator=(tensor_to_scalar_log &&) = delete;

private:
};
} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_LOG_H
