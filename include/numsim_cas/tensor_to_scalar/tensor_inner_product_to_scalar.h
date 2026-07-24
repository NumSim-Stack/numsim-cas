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

  tensor_inner_product_to_scalar(tensor_inner_product_to_scalar &&data) noexcept
      : base(std::move(static_cast<base &&>(data))),
        m_lhs_indices(std::move(data.m_lhs_indices)),
        m_rhs_indices(std::move(data.m_rhs_indices)) {}

  [[nodiscard]] const auto &indices_lhs() const noexcept {
    return m_lhs_indices;
  }

  [[nodiscard]] const auto &indices_rhs() const noexcept {
    return m_rhs_indices;
  }

  // #343 — the contraction sequences are part of the node's identity
  // (mirrors inner_product_wrapper's #266 fix): A:B and A:B^T must not
  // hash or compare equal.
  void update_hash_value() const noexcept override {
    base::m_hash_value = 0;
    hash_combine(base::m_hash_value, base::get_id());
    hash_combine(base::m_hash_value, base::expr_lhs().get().hash_value());
    hash_combine(base::m_hash_value, base::expr_rhs().get().hash_value());
    hash_combine(base::m_hash_value, m_lhs_indices);
    hash_combine(base::m_hash_value, m_rhs_indices);
  }

  friend bool operator==(tensor_inner_product_to_scalar const &lhs,
                         tensor_inner_product_to_scalar const &rhs) {
    return lhs.m_lhs_indices == rhs.m_lhs_indices &&
           lhs.m_rhs_indices == rhs.m_rhs_indices &&
           static_cast<base const &>(lhs) == static_cast<base const &>(rhs);
  }

  friend bool operator<(tensor_inner_product_to_scalar const &lhs,
                        tensor_inner_product_to_scalar const &rhs) {
    if (static_cast<base const &>(lhs) < static_cast<base const &>(rhs))
      return true;
    if (static_cast<base const &>(rhs) < static_cast<base const &>(lhs))
      return false;
    if (lhs.m_lhs_indices != rhs.m_lhs_indices)
      return lhs.m_lhs_indices < rhs.m_lhs_indices;
    return lhs.m_rhs_indices < rhs.m_rhs_indices;
  }

protected:
  sequence m_lhs_indices;
  sequence m_rhs_indices;
};

} // namespace numsim::cas

#endif // TENSOR_INNER_PRODUCT_TO_SCALAR_H
