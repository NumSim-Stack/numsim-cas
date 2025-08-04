#ifndef TENSOR_FUNCTIONS_SYMTM_H
#define TENSOR_FUNCTIONS_SYMTM_H

#include "../numsim_cas_type_traits.h"
#include "../tensor_to_scalar/tensor_inner_product_to_scalar.h"
#include "data/tensor_data_make_imp.h"
#include "simplifier/tensor_simplifier_add.h"
#include "simplifier/tensor_simplifier_mul.h"
#include "simplifier/tensor_simplifier_sub.h"
#include "simplifier/tensor_with_scalar_simplifier_div.h"
#include "simplifier/tensor_with_scalar_simplifier_mul.h"
#include <cstdlib>
#include <vector>

namespace numsim::cas {

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_add_tensor_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
  const auto &_lhs{*lhs};
  return visit(
      simplifier::tensor_detail::add_base<ExprTypeLHS, ExprTypeRHS>(
          std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
      _lhs);
}

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_sub_tensor_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
  return visit(
      tensor_detail::simplifier::sub_base<ExprTypeLHS, ExprTypeRHS>(
          std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
      *lhs);
}

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_mul_tensor_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
  return visit(
      tensor_detail::simplifier::mul_base<ExprTypeLHS, ExprTypeRHS>(
          std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
      *lhs);
}

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_mul_tensor_with_scalar_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
  // lhs := scalar_expression,
  // rhs := tensor_expression
  return visit(
      tensor_with_scalar_detail::simplifier::mul_base<ExprTypeLHS, ExprTypeRHS>(
          std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
      *lhs);
}

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_div_tensor_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
  return visit(
      tensor_with_scalar_detail::simplifier::div_base<ExprTypeLHS, ExprTypeRHS>(
          std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
      *lhs);
}

template <typename T>
constexpr inline auto make_tensor_data(std::size_t dim, std::size_t rank) {
  return make_tensor_data_imp<T>().evaluate(dim, rank);
}

template <typename ExprLHS, typename ExprRHS>
constexpr inline auto inner_product(ExprLHS &&lhs, sequence &&lhs_indices,
                                    ExprRHS &&rhs, sequence &&rhs_indices) {
  using ValueType = typename remove_cvref_t<ExprLHS>::value_type;
  assert(call_tensor::rank(lhs) != lhs_indices.size() ||
         call_tensor::rank(rhs) != rhs_indices.size());
  // tensor_expression
  return make_expression<inner_product_wrapper<ValueType>>(
      std::forward<ExprLHS>(lhs), std::move(lhs_indices),
      std::forward<ExprRHS>(rhs), std::move(rhs_indices));
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
