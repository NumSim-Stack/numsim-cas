#ifndef TENSOR_FUNCTIONS_SYMTM_H
#define TENSOR_FUNCTIONS_SYMTM_H

#include <vector>
#include <cstdlib>
#include "../numsim_cas_type_traits.h"
#include "tensor_expression.h"
#include "make_tensor_data_imp.h"

namespace numsim::cas {

template <typename T>
constexpr inline auto make_tensor_data(std::size_t dim, std::size_t rank) {
  return make_tensor_data_imp<T>().evaluate(dim, rank);
}

template<typename ExprLHS,typename SeqLHS,
          typename ExprRHS, typename SeqRHS/*,
          std::enable_if_t<std::is_base_of_v<tensor_expression<typename remove_cvref_t<ExprLHS>::value_type>, get_type_t<ExprLHS>>,bool> = true,
          std::enable_if_t<std::is_base_of_v<tensor_expression<typename remove_cvref_t<ExprRHS>::value_type>, get_type_t<ExprRHS>>,bool> = true,
          std::enable_if_t<std::is_same_v<std::vector<std::size_t>, SeqLHS>, bool> = true,
          std::enable_if_t<std::is_same_v<std::vector<std::size_t>, SeqRHS>, bool> = true*/>
constexpr inline auto
inner_product(ExprLHS &&lhs, SeqLHS &&lhs_indices, ExprRHS &&rhs, SeqRHS &&rhs_indices){
  using ValueType = typename remove_cvref_t<ExprLHS>::value_type;
  return make_expression<inner_product_wrapper<ValueType>>(
      std::forward<ExprLHS>(lhs), std::forward<SeqLHS>(lhs_indices),
      std::forward<ExprRHS>(rhs), std::forward<SeqRHS>(rhs_indices));
}

template<typename ExprLHS,typename SeqLHS,
          typename ExprRHS, typename SeqRHS/*,
          std::enable_if_t<std::is_base_of_v<tensor_expression<typename remove_cvref_t<ExprLHS>::value_type>, get_type_t<ExprLHS>>,bool> = true,
          std::enable_if_t<std::is_base_of_v<tensor_expression<typename remove_cvref_t<ExprRHS>::value_type>, get_type_t<ExprRHS>>,bool> = true,
          std::enable_if_t<std::is_same_v<std::vector<std::size_t>, SeqLHS>, bool> = true,
          std::enable_if_t<std::is_same_v<std::vector<std::size_t>, SeqRHS>, bool> = true*/>
constexpr inline auto
outer_product(ExprLHS &&lhs, SeqLHS &&lhs_indices, ExprRHS &&rhs, SeqRHS &&rhs_indices){
  using ValueType = typename remove_cvref_t<ExprLHS>::value_type;
  return make_expression<outer_product_wrapper<ValueType>>(
      std::forward<ExprLHS>(lhs), std::forward<SeqLHS>(lhs_indices),
      std::forward<ExprRHS>(rhs), std::forward<SeqRHS>(rhs_indices));
}

template<typename Expr,typename Seq/*,
          std::enable_if_t<std::is_base_of_v<tensor_expression<typename remove_cvref_t<Expr>::value_type>, get_type_t<Expr>>,bool> = true,
          std::enable_if_t<std::is_same_v<std::vector<std::size_t>, Seq>, bool> = true*/>
constexpr inline auto
basis_change(Expr &&expr, Seq &&indices){
  using ValueType = typename remove_cvref_t<Expr>::value_type;
  return make_expression<basis_change_imp<ValueType>>(
      std::forward<Expr>(expr), std::forward<Seq>(indices));
}

}

#endif // TENSOR_FUNCTIONS_SYMTM_H
