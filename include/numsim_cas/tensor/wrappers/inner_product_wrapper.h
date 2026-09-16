#ifndef INNER_PRODUCT_WRAPPER_H
#define INNER_PRODUCT_WRAPPER_H

#include <algorithm>
#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/tensor_shape_validation.h>
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
    detail::validate_contraction("inner_product", base::expr_lhs().get(),
                                 m_lhs_indices, base::expr_rhs().get(),
                                 m_rhs_indices);
  }

  inner_product_wrapper(inner_product_wrapper &&data) noexcept
      : base(std::move(static_cast<base &&>(data))),
        m_lhs_indices(std::move(data.m_lhs_indices)),
        m_rhs_indices(std::move(data.m_rhs_indices)) {}

  [[nodiscard]] const auto &indices_lhs() const noexcept {
    return m_lhs_indices;
  }

  [[nodiscard]] const auto &indices_rhs() const noexcept {
    return m_rhs_indices;
  }

  // #266 — fold the contraction indices into the hash (mirrors
  // outer_product_wrapper). Without this, the default binary_op hash
  // ignores them, so two inner_products differing only in their
  // contraction sequences hash-collide (cache aliasing + blind lock-ins).
  void update_hash_value() const override {
    hash_combine(base::m_hash_value, base::get_id());
    numsim::cas::hash_combine(base::m_hash_value,
                              base::expr_lhs().get().hash_value());
    numsim::cas::hash_combine(base::m_hash_value,
                              base::expr_rhs().get().hash_value());
    numsim::cas::hash_combine(base::m_hash_value, indices_lhs());
    numsim::cas::hash_combine(base::m_hash_value, indices_rhs());
  }

protected:
  sequence m_lhs_indices;
  sequence m_rhs_indices;
};

} // namespace numsim::cas

#endif // INNER_PRODUCT_WRAPPER_H
