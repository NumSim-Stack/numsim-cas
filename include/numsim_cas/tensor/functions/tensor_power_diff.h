#ifndef TENSOR_POWER_DIFF_H
#define TENSOR_POWER_DIFF_H

#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

class tensor_power_diff final
    : public binary_op<tensor_node_base_t<tensor_power_diff>, tensor_expression,
                       scalar_expression> {
public:
  using base = binary_op<tensor_node_base_t<tensor_power_diff>,
                         tensor_expression, scalar_expression>;

  template <typename ExprTensor, typename ExprScalar>
  tensor_power_diff(ExprTensor &&tensor, ExprScalar &&scalar)
      : base(std::forward<ExprTensor>(tensor), std::forward<ExprScalar>(scalar),
             tensor.get().dim(), tensor.get().rank()) {}
  tensor_power_diff(tensor_power_diff const &expr)
      : base(static_cast<base const &>(expr)) {}
  tensor_power_diff(tensor_power_diff &&expr)
      : base(std::move(static_cast<base &&>(expr))) {}
  tensor_power_diff() = delete;
  ~tensor_power_diff() = default;
  const tensor_power_diff &operator=(tensor_power_diff &&) = delete;

  inline void update_hash() {
    base::m_hash_value = this->m_lhs.get().hash_value();
  }

private:
};

} // namespace numsim::cas

#endif // TENSOR_POWER_DIFF_H
