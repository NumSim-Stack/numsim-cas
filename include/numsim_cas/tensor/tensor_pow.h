#ifndef TENSOR_POW_H
#define TENSOR_POW_H

#include "../binary_op.h"

namespace numsim::cas {
template <typename ValueType>
class tensor_pow final
    : public binary_op<tensor_pow<ValueType>, tensor_expression<ValueType>,
                       scalar_expression<ValueType>> {
public:
  using base = binary_op<tensor_pow<ValueType>, tensor_expression<ValueType>,
                         scalar_expression<ValueType>>;

  template <typename ExprTensor, typename ExprScalar>
  tensor_pow(ExprTensor &&tensor, ExprScalar &&scalar)
      : base(std::forward<ExprTensor>(tensor), std::forward<ExprScalar>(scalar),
             call_tensor::dim(tensor), call_tensor::rank(tensor)) {}
  tensor_pow(std::size_t dim, std::size_t rank) : base(dim, rank) {}
  tensor_pow(tensor_pow const &expr)
      : base(static_cast<base const &>(expr), call_tensor::dim(expr),
             call_tensor::rank(expr)) {}
  tensor_pow(tensor_pow &&expr)
      : base(std::move(static_cast<base &&>(expr)), call_tensor::dim(expr),
             call_tensor::rank(expr)) {}
  tensor_pow() = delete;
  ~tensor_pow() = default;
  const tensor_pow &operator=(tensor_pow &&) = delete;

  inline void update_hash() {
    //    base::m_hash_value = 0;
    //    if(!is_same<scalar_constant<ValueType>>(this->m_rhs)){
    //      hash_combine(base::m_hash_value, this->m_lhs.get().hash_value());
    //      hash_combine(base::m_hash_value, this->m_rhs.get().hash_value());
    //    }else{
    //      base::m_hash_value = this->m_lhs.get().hash_value();
    //    }
    base::m_hash_value = this->m_lhs.get().hash_value();
  }

private:
};
} // namespace numsim::cas

#endif // TENSOR_POW_H
