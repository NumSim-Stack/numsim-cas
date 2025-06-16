#ifndef TENSOR_FUNCTIONS_SYMTM_H
#define TENSOR_FUNCTIONS_SYMTM_H

#include "../numsim_cas_type_traits.h"
#include "../tensor_to_scalar/tensor_inner_product_to_scalar.h"
#include "data/tensor_data_make_imp.h"
#include <cstdlib>
#include <vector>

namespace numsim::cas {

template <typename T>
constexpr inline auto make_tensor_data(std::size_t dim, std::size_t rank) {
  return make_tensor_data_imp<T>().evaluate(dim, rank);
}

template <typename ExprLHS, typename ExprRHS>
constexpr inline auto inner_product(ExprLHS &&lhs, sequence &&lhs_indices,
                                    ExprRHS &&rhs, sequence &&rhs_indices) {
  using ValueType = typename remove_cvref_t<ExprLHS>::value_type;
  const auto rank_lhs{call_tensor::rank(lhs)};
  const auto rank_rhs{call_tensor::rank(rhs)};
  const auto size_lhs{lhs_indices.size()};
  const auto size_rhs{rhs_indices.size()};
  if ((rank_lhs + rank_rhs) == (size_lhs + size_rhs)) {
    // tensor_to_scalar_expression
    return make_expression<tensor_inner_product_to_scalar<ValueType>>(
        std::forward<ExprLHS>(lhs), std::move(lhs_indices),
        std::forward<ExprRHS>(rhs), std::move(rhs_indices));
  } else {
    // tensor_expression
    return make_expression<inner_product_wrapper<ValueType>>(
        std::forward<ExprLHS>(lhs), std::move(lhs_indices),
        std::forward<ExprRHS>(rhs), std::move(rhs_indices));
  }
}

template <typename ExprLHS, typename ExprRHS>
constexpr inline auto outer_product(ExprLHS &&lhs, sequence &&lhs_indices,
                                    ExprRHS &&rhs, sequence &&rhs_indices) {
  using ValueType = typename remove_cvref_t<ExprLHS>::value_type;
  return make_expression<outer_product_wrapper<ValueType>>(
      std::forward<ExprLHS>(lhs), std::move(lhs_indices),
      std::forward<ExprRHS>(rhs), std::move(rhs_indices));
}

template <typename Expr>
constexpr inline auto permute_indices(Expr &&expr, sequence &&indices) {
  using ValueType = typename remove_cvref_t<Expr>::value_type;
  return make_expression<basis_change_imp<ValueType>>(std::forward<Expr>(expr),
                                                      std::move(indices));
}

} // namespace numsim::cas

#endif // TENSOR_FUNCTIONS_SYMTM_H
