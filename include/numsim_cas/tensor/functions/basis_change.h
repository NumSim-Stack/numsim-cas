#ifndef BASIS_CHANGE_H
#define BASIS_CHANGE_H

#include "../../numsim_cas_type_traits.h"
#include "../../unary_op.h"
#include <algorithm>
#include <stdexcept>
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
    : public unary_op<basis_change_imp, tensor_expression> {
public:
  using base = unary_op<basis_change_imp, tensor_expression>;

  /**
   * @brief Constructs a basis_change_imp object.
   * @tparam Expr The type of the tensor expression.
   * @tparam Indices The type of the indices array.
   * @param expr The tensor expression to transform.
   * @param indices The indices for basis transformation.
   */
  template <typename Expr, typename Indices>
  basis_change_imp(Expr &&expr, Indices &&indices)
      : base(std::forward<Expr>(expr), call_tensor::dim(expr),
             call_tensor::rank(expr)),
        m_indices(std::forward<Indices>(indices)) {
    std::for_each(std::begin(m_indices), std::end(m_indices),
                  [](auto &index) { index -= 1ul; });
  }

  /**
   * @brief Move constructor.
   * @param data The basis_change_imp object to move from.
   */
  explicit basis_change_imp(basis_change_imp &&data)
      : base(std::move(static_cast<base &&>(data)), data.dim(), data.rank()),
        m_indices(std::move(data.m_indices)) {}

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
  std::vector<std::size_t> m_indices;
};

} // namespace numsim::cas

#endif // BASIS_CHANGE_H
