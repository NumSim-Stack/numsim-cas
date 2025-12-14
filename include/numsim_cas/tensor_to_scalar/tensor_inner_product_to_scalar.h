#ifndef TENSOR_INNER_PRODUCT_TO_SCALAR_H
#define TENSOR_INNER_PRODUCT_TO_SCALAR_H

#include "../binary_op.h"
#include "../numsim_cas_type_traits.h"
#include "../tensor/tensor_expression.h"
#include "tensor_to_scalar_expression.h"
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace numsim::cas {

template <typename ValueType>
class tensor_inner_product_to_scalar final
    : public binary_op<tensor_inner_product_to_scalar<ValueType>,
                       tensor_to_scalar_expression<ValueType>,
                       tensor_expression<ValueType>> {
public:
  using base = binary_op<tensor_inner_product_to_scalar<ValueType>,
                         tensor_to_scalar_expression<ValueType>,
                         tensor_expression<ValueType>>;

  template <typename LHS, typename RHS, typename SeqLHS, typename SeqRHS>
  tensor_inner_product_to_scalar(LHS &&_lhs, SeqLHS &&_lhs_indices, RHS &&_rhs,
                                 SeqRHS &&_rhs_indices)
      : base(std::forward<LHS>(_lhs), std::forward<RHS>(_rhs)),
        m_lhs_indices(std::forward<SeqLHS>(_lhs_indices)),
        m_rhs_indices(std::forward<SeqRHS>(_rhs_indices)) {
    std::for_each(m_lhs_indices.begin(), m_lhs_indices.end(),
                  [](std::size_t &index) { index -= 1; });
    std::for_each(m_rhs_indices.begin(), m_rhs_indices.end(),
                  [](std::size_t &index) { index -= 1; });
  }

  tensor_inner_product_to_scalar(tensor_inner_product_to_scalar &&data)
      : base(std::move(static_cast<base &&>(data)), data.dim(), data.rank()),
        m_lhs_indices(std::move(data.m_lhs_indices)),
        m_rhs_indices(std::move(data.m_rhs_indices)) {}

  [[nodiscard]] const auto &indices_lhs() const noexcept {
    return m_lhs_indices;
  }

  [[nodiscard]] const auto &indices_rhs() const noexcept {
    return m_rhs_indices;
  }

protected:
  std::vector<std::size_t> m_lhs_indices;
  std::vector<std::size_t> m_rhs_indices;
};

} // namespace numsim::cas

#endif // TENSOR_INNER_PRODUCT_TO_SCALAR_H
