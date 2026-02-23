#ifndef BASIS_CHANGE_H
#define BASIS_CHANGE_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <vector>

namespace numsim::cas {

/**
 * @class basis_change_imp
 * @brief Implements basis change for tensor expressions.
 *
 * This class performs a basis transformation on a tensor expression.
 *
 * @tparam ValueType The type of the tensor expression.
 */
class basis_change_imp final
    : public unary_op<tensor_node_base_t<basis_change_imp>> {
public:
  using base = unary_op<tensor_node_base_t<basis_change_imp>>;

  /**
   * @brief Constructs a basis_change_imp object.
   * @tparam Expr The type of the tensor expression.
   * @tparam Indices The type of the indices array.
   * @param expr The tensor expression to transform.
   * @param indices The indices for basis transformation.
   */
  template <typename Expr, typename Indices>
  basis_change_imp(Expr &&expr, Indices &&indices)
      : base(std::forward<Expr>(expr), expr.get().dim(), expr.get().rank()),
        m_indices(std::forward<Indices>(indices)) {}

  /**
   * @brief Move constructor.
   * @param data The basis_change_imp object to move from.
   */
  explicit basis_change_imp(basis_change_imp &&data) noexcept
      : base(static_cast<base>(data)), m_indices(std::move(data.m_indices)) {}

  /**
   * @brief Retrieves the transformation indices.
   * @return A constant reference to the indices.
   */
  [[nodiscard]] constexpr inline auto const &indices() const noexcept {
    return m_indices;
  }

protected:
  /**
   * @brief Stores the transformation indices.
   */
  sequence m_indices;
};

} // namespace numsim::cas

#endif // BASIS_CHANGE_H
