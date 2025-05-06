#ifndef OUTER_PRODUCT_WRAPPER_H
#define OUTER_PRODUCT_WRAPPER_H

#include "../binary_op.h"
#include "../symTM_type_traits.h"
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace numsim::cas {

template <typename ValueType>
class outer_product_wrapper final
    : public binary_op<outer_product_wrapper<ValueType>,
                             tensor_expression<ValueType>> {
public:
  using base = binary_op<outer_product_wrapper<ValueType>, tensor_expression<ValueType>>;

  template <typename LHS, typename RHS, typename SeqLHS, typename SeqRHS>
  outer_product_wrapper(LHS &&_lhs, SeqLHS &&_lhs_indices, RHS &&_rhs,
                        SeqRHS &&_rhs_indices)
      : base(std::forward<LHS>(_lhs), std::forward<RHS>(_rhs), call_tensor::dim(_lhs),
                  call_tensor::rank(_lhs) + call_tensor::rank(_rhs)),
        m_lhs_indices(std::forward<SeqLHS>(_lhs_indices)),
        m_rhs_indices(std::forward<SeqRHS>(_rhs_indices)) {
    init();
  }

  const auto &sequence_lhs() const noexcept { return m_lhs_indices; }
  const auto &sequence_rhs() const noexcept { return m_rhs_indices; }

protected:
  void init() {
    std::for_each(m_lhs_indices.begin(), m_lhs_indices.end(),
                  [](std::size_t &index) { index -= 1; });
    std::for_each(m_rhs_indices.begin(), m_rhs_indices.end(),
                  [](std::size_t &index) { index -= 1; });
    tensor_expression<ValueType> &lhs{*this->m_lhs};
    tensor_expression<ValueType> &rhs{*this->m_rhs};
  }

  std::vector<std::size_t> m_lhs_indices;
  std::vector<std::size_t> m_rhs_indices;
};

} // NAMESPACE symTM

#endif // OUTER_PRODUCT_WRAPPER_H
