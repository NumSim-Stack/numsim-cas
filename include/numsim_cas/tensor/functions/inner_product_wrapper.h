#ifndef INNER_PRODUCT_WRAPPER_H
#define INNER_PRODUCT_WRAPPER_H

#include <algorithm>
#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <stdexcept>
#include <vector>

namespace numsim::cas {

class inner_product_wrapper final
    : public binary_op<tensor_node_base_t<inner_product_wrapper>,
                       tensor_expression> {
public:
  using base =
      binary_op<tensor_node_base_t<inner_product_wrapper>, tensor_expression>;

  template <typename LHS, typename RHS, typename SeqLHS, typename SeqRHS>
  inner_product_wrapper(LHS &&_lhs, SeqLHS &&_lhs_indices, RHS &&_rhs,
                        SeqRHS &&_rhs_indices)
      : base(std::forward<LHS>(_lhs), std::forward<RHS>(_rhs), _lhs.get().dim(),
             _lhs.get().rank() + _rhs.get().rank() - _lhs_indices.size() -
                 _rhs_indices.size()),
        m_lhs_indices(std::forward<SeqLHS>(_lhs_indices)),
        m_rhs_indices(std::forward<SeqRHS>(_rhs_indices)) {
    //    tensor_expression &lhs{*this->m_lhs};
    //    tensor_expression &rhs{*this->m_rhs};
    //    const auto rank_lhs{lhs.rank()};
    //    const auto rank_rhs{rhs.rank()};
    //    this->m_rank =
    //        rank_lhs + rank_rhs - m_lhs_indices.size() - m_rhs_indices.size();
    //    if (lhs.dim() == rhs.dim()) {
    //      this->m_dim = lhs.dim();
    //    } else {
    //      throw
    //      std::runtime_error("inner_product_wrapper::inner_product_wrapper("
    //                               ") lhs.dim() != rhs.dim()");
    //    }
  }

  inner_product_wrapper(inner_product_wrapper &&data)
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

#endif // INNER_PRODUCT_WRAPPER_H
