#ifndef TENSOR_SCALAR_MUL_H
#define TENSOR_SCALAR_MUL_H

#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

class tensor_scalar_mul final
    : public binary_op<tensor_node_base_t<tensor_scalar_mul>, scalar_expression,
                       tensor_expression> {
public:
  using base = binary_op<tensor_node_base_t<tensor_scalar_mul>,
                         scalar_expression, tensor_expression>;

  template <typename LHS, typename RHS>
  tensor_scalar_mul(LHS &&lhs, RHS &&rhs)
      : base(std::forward<LHS>(lhs), std::forward<RHS>(rhs), rhs.get().dim(),
             rhs.get().rank()),
        m_constant(is_same<scalar_constant>(this->m_lhs)) {}

  template <typename LHS, typename RHS>
  tensor_scalar_mul(LHS const &lhs, RHS const &rhs)
      : base(lhs, rhs, rhs.get().dim(), rhs.get().rank()),
        m_constant(is_same<scalar_constant>(this->m_lhs)) {}

  virtual void update_hash_value() const noexcept override {
    base::m_hash_value = 0;
    if (!m_constant) {
      hash_combine(base::m_hash_value, this->m_lhs.get().hash_value());
    }
    hash_combine(base::m_hash_value, this->m_rhs.get().hash_value());
    base::m_hash_value = this->m_rhs.get().hash_value();
  }

private:
  bool m_constant;
};

} // namespace numsim::cas

#endif // TENSOR_SCALAR_MUL_H
