#ifndef TENSOR_POW_H
#define TENSOR_POW_H

#include <numsim_cas/basic_functions.h>
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
             tensor.get().dim(), tensor.get().rank()) {
    // Symmetric and Volumetric are closed under exponentiation.
    // Deviatoric/Harmonic have Symmetric perm, so A^n is still symmetric
    // but trace-freeness is lost (tr(D^2) ≠ 0) — downgrade to Symmetric.
    // Skew^n alternates sym/skew with parity — can't determine, so drop.
    if (auto const &sp = this->m_lhs.get().space()) {
      if (std::holds_alternative<Symmetric>(sp->perm)) {
        if (std::holds_alternative<AnyTraceTag>(sp->trace) ||
            std::holds_alternative<VolumetricTag>(sp->trace))
          this->set_space(*sp);
        else
          this->set_space({Symmetric{}, AnyTraceTag{}});
      }
    }
  }
  tensor_pow(tensor_pow const &expr) : base(static_cast<base const &>(expr)) {}
  tensor_pow(tensor_pow &&expr) : base(static_cast<base &&>(expr)) {}
  tensor_pow() = delete;
  ~tensor_pow() = default;
  const tensor_pow &operator=(tensor_pow &&) = delete;

  virtual void update_hash_value() const noexcept override {
    if (is_same<scalar_constant>(this->m_rhs)) {
      base::m_hash_value = this->m_lhs.get().hash_value();
    } else {
      base::m_hash_value = 0;
      hash_combine(base::m_hash_value, this->m_lhs.get().hash_value());
      hash_combine(base::m_hash_value, this->m_rhs.get().hash_value());
    }
  }

private:
};
} // namespace numsim::cas

#endif // TENSOR_POW_H
