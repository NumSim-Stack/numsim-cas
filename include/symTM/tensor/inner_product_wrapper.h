#ifndef INNER_PRODUCT_WRAPPER_H
#define INNER_PRODUCT_WRAPPER_H

#include "../binary_op.h"
#include "tensor_expression.h"
#include "../numsim_cas_type_traits.h"
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace numsim::cas {

template <typename ValueType>
class inner_product_wrapper final
    : public binary_op<inner_product_wrapper<ValueType>,
                       tensor_expression<ValueType>> {
public:
  using base = binary_op<inner_product_wrapper<ValueType>, tensor_expression<ValueType>>;

  template <typename LHS, typename RHS, typename SeqLHS, typename SeqRHS>
  inner_product_wrapper(LHS &&_lhs, SeqLHS &&_lhs_indices, RHS &&_rhs,
                        SeqRHS &&_rhs_indices)
      : base(std::forward<LHS>(_lhs), std::forward<RHS>(_rhs),call_tensor::dim(this->m_lhs),
                  call_tensor::rank(this->m_lhs) +
                      call_tensor::rank(this->m_rhs) - _lhs_indices.size() -
                      _rhs_indices.size()),
        m_lhs_indices(std::forward<SeqLHS>(_lhs_indices)),
        m_rhs_indices(std::forward<SeqRHS>(_rhs_indices)) {
    //    tensor_expression<ValueType> &lhs{*this->m_lhs};
    //    tensor_expression<ValueType> &rhs{*this->m_rhs};
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
    std::for_each(m_lhs_indices.begin(), m_lhs_indices.end(),
                  [](std::size_t &index) { index -= 1; });
    std::for_each(m_rhs_indices.begin(), m_rhs_indices.end(),
                  [](std::size_t &index) { index -= 1; });
  }

  inner_product_wrapper(inner_product_wrapper &&data)
      : base(std::move(static_cast<base &&>(data)),data.dim(), data.rank()),
        m_lhs_indices(std::move(data.m_lhs_indices)),
        m_rhs_indices(std::move(data.m_rhs_indices)) {}

  [[nodiscard]] const auto &sequence_lhs() const noexcept {
    return m_lhs_indices;
  }

  [[nodiscard]] const auto &sequence_rhs() const noexcept {
    return m_rhs_indices;
  }

protected:
  std::vector<std::size_t> m_lhs_indices;
  std::vector<std::size_t> m_rhs_indices;
};

} // NAMESPACE symTM

#endif // INNER_PRODUCT_WRAPPER_H
