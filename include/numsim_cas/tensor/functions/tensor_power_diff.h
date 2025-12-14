#ifndef TENSOR_POWER_DIFF_H
#define TENSOR_POWER_DIFF_H

#include "../../binary_op.h"

namespace numsim::cas {

template <typename ValueType>
class tensor_power_diff final
    : public binary_op<tensor_power_diff<ValueType>,
                       tensor_expression<ValueType>,
                       tensor_expression<ValueType>, scalar_expression<int>> {
public:
  using base =
      binary_op<tensor_power_diff<ValueType>, tensor_expression<ValueType>,
                tensor_expression<ValueType>, scalar_expression<int>>;

  template <typename ExprTensor, typename ExprScalar>
  tensor_power_diff(ExprTensor &&tensor, ExprScalar &&scalar)
      : base(std::forward<ExprTensor>(tensor), std::forward<ExprScalar>(scalar),
             call_tensor::dim(tensor), call_tensor::rank(tensor)) {}
  tensor_power_diff(std::size_t dim, std::size_t rank) : base(dim, rank) {}
  tensor_power_diff(tensor_power_diff const &expr)
      : base(static_cast<base const &>(expr), call_tensor::dim(expr),
             call_tensor::rank(expr)) {}
  tensor_power_diff(tensor_power_diff &&expr)
      : base(std::move(static_cast<base &&>(expr)), call_tensor::dim(expr),
             call_tensor::rank(expr)) {}
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
