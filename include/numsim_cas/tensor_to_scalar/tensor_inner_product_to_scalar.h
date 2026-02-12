#ifndef TENSOR_INNER_PRODUCT_TO_SCALAR_H
#define TENSOR_INNER_PRODUCT_TO_SCALAR_H

#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/tensor/sequence.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

class tensor_inner_product_to_scalar final
    : public binary_op<
          tensor_to_scalar_node_base_t<tensor_inner_product_to_scalar>,
          tensor_expression, tensor_expression> {
public:
  using base =
      binary_op<tensor_to_scalar_node_base_t<tensor_inner_product_to_scalar>,
                tensor_expression, tensor_expression>;

  template <typename LHS, typename RHS, typename SeqLHS, typename SeqRHS>
  tensor_inner_product_to_scalar(LHS &&_lhs, SeqLHS &&_lhs_indices, RHS &&_rhs,
                                 SeqRHS &&_rhs_indices)
      : base(std::forward<LHS>(_lhs), std::forward<RHS>(_rhs)),
        m_lhs_indices(std::forward<SeqLHS>(_lhs_indices)),
        m_rhs_indices(std::forward<SeqRHS>(_rhs_indices)) {}

  tensor_inner_product_to_scalar(tensor_inner_product_to_scalar &&data)
      : base(std::move(static_cast<base &&>(data))),
        m_lhs_indices(std::move(data.m_lhs_indices)),
        m_rhs_indices(std::move(data.m_rhs_indices)) {}

  [[nodiscard]] const auto &indices_lhs() const noexcept {
    return m_lhs_indices;
  }

  [[nodiscard]] const auto &indices_rhs() const noexcept {
    return m_rhs_indices;
  }

protected:
  sequence m_lhs_indices;
  sequence m_rhs_indices;
};

} // namespace numsim::cas

#endif // TENSOR_INNER_PRODUCT_TO_SCALAR_H
