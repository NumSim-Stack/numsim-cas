#ifndef PERMUTE_INDICES_H
#define PERMUTE_INDICES_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <vector>

namespace numsim::cas {

/**
 * @class permute_indices_wrapper
 * @brief Permutes the index ordering of a tensor expression.
 *
 * Represents an index permutation of a tensor. The classic special case is
 * transpose (`indices == {2, 1}` for a rank-2 tensor), but arbitrary
 * permutations of any rank are supported.
 *
 * @tparam ValueType The type of the tensor expression.
 */
class permute_indices_wrapper final
    : public unary_op<tensor_node_base_t<permute_indices_wrapper>> {
public:
  using base = unary_op<tensor_node_base_t<permute_indices_wrapper>>;

  /**
   * @brief Constructs a permute_indices_wrapper object.
   * @tparam Expr The type of the tensor expression.
   * @tparam Indices The type of the indices array.
   * @param expr The tensor expression to permute.
   * @param indices The permutation of indices.
   */
  template <typename Expr, typename Indices>
  permute_indices_wrapper(Expr &&expr, Indices &&indices)
      : base(std::forward<Expr>(expr), expr.get().dim(), expr.get().rank()),
        m_indices(std::forward<Indices>(indices)) {}

  /**
   * @brief Move constructor.
   * @param data The permute_indices_wrapper object to move from.
   */
  explicit permute_indices_wrapper(permute_indices_wrapper &&data) noexcept
      : base(static_cast<base>(data)), m_indices(std::move(data.m_indices)) {}

  /**
   * @brief Retrieves the permutation indices.
   * @return A constant reference to the indices.
   */
  [[nodiscard]] constexpr inline auto const &indices() const noexcept {
    return m_indices;
  }

protected:
  /**
   * @brief Stores the permutation indices.
   */
  sequence m_indices;
};

} // namespace numsim::cas

#endif // PERMUTE_INDICES_H
