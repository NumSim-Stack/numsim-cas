#ifndef TENSOR_SCALAR_MUL_H
#define TENSOR_SCALAR_MUL_H

#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/scalar/scalar_functions.h>
#include <numsim_cas/tensor/structural_propagation.h>
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
             rhs.get().rank()) {
    // α·A has the same structural classification as A for any α.
    structural_propagation::preserve_unary(*this, this->m_rhs.get());
  }

  template <typename LHS, typename RHS>
  tensor_scalar_mul(LHS const &lhs, RHS const &rhs)
      : base(lhs, rhs, rhs.get().dim(), rhs.get().rank()) {
    structural_propagation::preserve_unary(*this, this->m_rhs.get());
  }

  void update_hash_value() const noexcept override {
    if (is_scalar_constant(this->m_lhs)) { // #284: singleton-aware
      base::m_hash_value = this->m_rhs.get().hash_value();
    } else {
      base::m_hash_value = 0;
      hash_combine(base::m_hash_value, this->m_lhs.get().hash_value());
      hash_combine(base::m_hash_value, this->m_rhs.get().hash_value());
    }
  }

  // c*T is a like term of T and of any c'*T (add-side merging, #340)
  [[nodiscard]] bool
  like_term_of(expression const &rhs) const noexcept override {
    if (this->hash_value() != rhs.hash_value())
      return false;
    if (rhs.id() == this->id()) {
      auto const &r = static_cast<tensor_scalar_mul const &>(rhs);
      return this->expr_rhs() == r.expr_rhs();
    }
    return this->expr_rhs().get() == rhs;
  }
};

} // namespace numsim::cas

#endif // TENSOR_SCALAR_MUL_H
