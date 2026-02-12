#ifndef TENSOR_POW_H
#define TENSOR_POW_H

#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

class tensor_pow final
    : public binary_op<tensor_node_base_t<tensor_pow>, tensor_expression,
                       scalar_expression> {
public:
  using base = binary_op<tensor_node_base_t<tensor_pow>, tensor_expression,
                         scalar_expression>;

  template <typename ExprTensor, typename ExprScalar>
  tensor_pow(ExprTensor &&tensor, ExprScalar &&scalar)
      : base(std::forward<ExprTensor>(tensor), std::forward<ExprScalar>(scalar),
             tensor.get().dim(), tensor.get().rank()) {}
  tensor_pow(tensor_pow const &expr) : base(static_cast<base const &>(expr)) {}
  tensor_pow(tensor_pow &&expr) : base(static_cast<base>(expr)) {}
  tensor_pow() = delete;
  ~tensor_pow() = default;
  const tensor_pow &operator=(tensor_pow &&) = delete;

  inline void update_hash() {
    //    base::m_hash_value = 0;
    //    if(!is_same<scalar_constant>(this->m_rhs)){
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
