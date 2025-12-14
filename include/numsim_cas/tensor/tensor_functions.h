#ifndef TENSOR_FUNCTIONS_SYMTM_H
#define TENSOR_FUNCTIONS_SYMTM_H

#include "../numsim_cas_type_traits.h"
#include "data/tensor_data_make_imp.h"
#include "simplifier/tensor_inner_product_simplifier.h"
#include "simplifier/tensor_simplifier_add.h"
#include "simplifier/tensor_simplifier_mul.h"
#include "simplifier/tensor_simplifier_sub.h"
#include "simplifier/tensor_with_scalar_simplifier_div.h"
#include "simplifier/tensor_with_scalar_simplifier_mul.h"
#include "tensor_globals.h"
#include "visitors/tensor_differentiation.h"
#include "visitors/tensor_evaluator.h"
#include "visitors/tensor_printer.h"

#include <algorithm>
#include <cstdlib>
#include <ranges>
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

template <typename Expr,
          std::enable_if_t<
              std::is_base_of_v<
                  numsim::cas::tensor_expression<
                      typename numsim::cas::remove_cvref_t<Expr>::value_type>,
                  typename numsim::cas::remove_cvref_t<Expr>::expr_type>,
              bool> = true>
constexpr inline auto dev(Expr &&expr) {
  using ValueType = typename remove_cvref_t<Expr>::value_type;
  if (expr.get().rank() == 2) {
    return make_expression<tensor_deviatoric<ValueType>>(
        std::forward<Expr>(expr));
  }
  throw std::runtime_error("");
}

template <typename ExprLHS, typename ExprRHS>
constexpr inline auto inner_product(ExprLHS &&lhs, sequence &&lhs_indices,
                                    ExprRHS &&rhs, sequence &&rhs_indices) {
  // using ValueType = typename remove_cvref_t<ExprLHS>::value_type;
  // assert(call_tensor::rank(lhs) != lhs_indices.size() ||
  //        call_tensor::rank(rhs) != rhs_indices.size());
  const auto &_lhs{*lhs};
  inner_product_simplifier<ExprLHS, ExprRHS> simplifier(
      std::forward<ExprLHS>(lhs), std::move(lhs_indices),
      std::forward<ExprRHS>(rhs), std::move(rhs_indices));
  return std::visit(simplifier, _lhs);
  //  return make_expression<inner_product_wrapper<ValueType>>(
  //      std::forward<ExprLHS>(lhs), std::move(lhs_indices),
  //      std::forward<ExprRHS>(rhs), std::move(rhs_indices));
}

template <typename ExprLHS, typename ExprRHS>
constexpr inline auto inner_product(ExprLHS &&lhs, sequence const &lhs_indices,
                                    ExprRHS &&rhs,
                                    sequence const &rhs_indices) {
  assert(call_tensor::rank(lhs) != lhs_indices.size() ||
         call_tensor::rank(rhs) != rhs_indices.size());
  const auto &_lhs{*lhs};
  inner_product_simplifier<ExprLHS, ExprRHS> simplifier(
      std::forward<ExprLHS>(lhs), lhs_indices, std::forward<ExprRHS>(rhs),
      rhs_indices);
  return std::visit(simplifier, _lhs);
}

template <typename ExprLHS, typename ExprRHS>
constexpr inline auto otimes(ExprLHS &&lhs, ExprRHS &&rhs) {
  using ValueType = typename remove_cvref_t<ExprLHS>::value_type;
  sequence lhs_indices(lhs.get().rank()), rhs_indices(rhs.get().rank());
  std::iota(std::begin(lhs_indices), std::end(lhs_indices), 1);
  std::iota(std::begin(rhs_indices), std::end(rhs_indices),
            lhs_indices.size() + 1);
  return make_expression<outer_product_wrapper<ValueType>>(
      std::forward<ExprLHS>(lhs), std::move(lhs_indices),
      std::forward<ExprRHS>(rhs), std::move(rhs_indices));
}

template <typename ExprLHS, typename ExprRHS>
constexpr inline auto otimes(ExprLHS &&lhs, sequence &&lhs_indices,
                             ExprRHS &&rhs, sequence &&rhs_indices) {
  using ValueType = typename remove_cvref_t<ExprLHS>::value_type;
  return make_expression<outer_product_wrapper<ValueType>>(
      std::forward<ExprLHS>(lhs), std::move(lhs_indices),
      std::forward<ExprRHS>(rhs), std::move(rhs_indices));
}

template <typename ExprLHS, typename ExprRHS>
constexpr inline auto otimesu(ExprLHS &&lhs, ExprRHS &&rhs) {
  using ValueType = typename remove_cvref_t<ExprLHS>::value_type;
  return make_expression<outer_product_wrapper<ValueType>>(
      std::forward<ExprLHS>(lhs), std::move(sequence{1, 3}),
      std::forward<ExprRHS>(rhs), std::move(sequence{2, 4}));
}

template <typename ExprLHS, typename ExprRHS>
constexpr inline auto otimesl(ExprLHS &&lhs, ExprRHS &&rhs) {
  using ValueType = typename remove_cvref_t<ExprLHS>::value_type;
  return make_expression<outer_product_wrapper<ValueType>>(
      std::forward<ExprLHS>(lhs), std::move(sequence{1, 4}),
      std::forward<ExprRHS>(rhs), std::move(sequence{2, 3}));
}

template <typename Expr>
constexpr inline auto permute_indices(Expr &&expr, sequence &&indices) {
  using ValueType = typename remove_cvref_t<Expr>::value_type;

  if (is_same<basis_change_imp<ValueType>>(expr)) {
    std::cout << "basis_change_imp" << std::endl;
    auto &tensor{expr.template get<basis_change_imp<ValueType>>()};
    const auto &t_indices{tensor.indices()};
    sequence new_order(t_indices.size());
    for (std::size_t i{0}; i < t_indices.size(); ++i) {
      new_order[i] = t_indices[indices[i] - 1];
    }
    // std::ranges::transform(indices, new_order,
    //                        [&](std::size_t from){ return
    //                        std::move(t_indices[from-1]);});
    for (const auto &el : indices) {
      std::cout << el << " ";
    }
    std::cout << std::endl;
    for (const auto &el : t_indices) {
      std::cout << el << " ";
    }
    std::cout << std::endl;
    for (const auto &el : new_order) {
      std::cout << el << " ";
    }
    std::cout << std::endl;
  }

  if (is_same<outer_product_wrapper<ValueType>>(expr)) {
    std::cout << "outer_product_wrapper" << std::endl;
    auto &tensor{expr.template get<outer_product_wrapper<ValueType>>()};
    const auto &indices_lhs{tensor.indices_lhs()};
    const auto &indices_rhs{tensor.indices_rhs()};
    sequence indices_old;
    indices_old.reserve(indices_lhs.size() + indices_rhs.size());
    indices_old.insert(indices_old.end(), indices_lhs.begin(),
                       indices_lhs.end());
    indices_old.insert(indices_old.end(), indices_rhs.begin(),
                       indices_rhs.end());
    sequence indices_new(indices_lhs.size() + indices_rhs.size());
    sequence indices_new_lhs, indices_new_rhs;
    indices_new_lhs.reserve(indices_lhs.size());
    indices_new_rhs.reserve(indices_rhs.size());

    // I_ik I_jl
    // basis_change(outer_product(I, {1,3}, I, {2,4}), [3,2,1,4])
    // {}
    // old {1,3,2,4}
    // new {2,3,1,4}

    for (std::size_t i{0}; i < indices.size(); ++i) {
      indices_new[i] = 1 + indices_old[indices[i] - 1];
    }

    indices_new_lhs.insert(indices_new_lhs.begin(), indices_new.begin(),
                           indices_new.begin() + indices_lhs.size());
    indices_new_rhs.insert(indices_new_rhs.begin(),
                           indices_new.begin() + indices_lhs.size(),
                           indices_new.end());
    return otimes(tensor.expr_lhs(), std::move(indices_new_lhs),
                  tensor.expr_rhs(), std::move(indices_new_rhs));
    //
    //    const auto& t_indices{tensor.indices()};
    //    sequence new_order(t_indices.size());
    //    for(std::size_t i{0}; i<t_indices.size(); ++i){
    //      new_order[i] = t_indices[indices[i]-1];
    //    }
    //    //std::ranges::transform(indices, new_order,
    //    //                       [&](std::size_t from){ return
    //    std::move(t_indices[from-1]);}); for(const auto& el : indices){
    //      std::cout<<el<<" ";
    //    }
    //    std::cout<<std::endl;
    //    for(const auto& el : t_indices){
    //      std::cout<<el<<" ";
    //    }
    //    std::cout<<std::endl;
    //    for(const auto& el : new_order){
    //      std::cout<<el<<" ";
    //    }
    //    std::cout<<std::endl;
  }

  // outer_product_wrapper
  return make_expression<basis_change_imp<ValueType>>(std::forward<Expr>(expr),
                                                      std::move(indices));
}

template <typename Expr> constexpr inline auto trans(Expr &&expr) {
  using ValueType = typename remove_cvref_t<Expr>::value_type;
  return make_expression<basis_change_imp<ValueType>>(std::forward<Expr>(expr),
                                                      sequence{2, 1});
}

template <typename Expr> constexpr inline auto inv(Expr &&expr) {
  using ValueType = typename remove_cvref_t<Expr>::value_type;
  return make_expression<tensor_inv<ValueType>>(std::forward<Expr>(expr));
}

template <typename ValueType, typename StreamType>
constexpr inline void
print(StreamType &out,
      expression_holder<tensor_expression<ValueType>> const &expr,
      Precedence precedence) {
  tensor_printer<ValueType, StreamType> eval(out);
  eval.apply(expr, precedence);
}

template <typename ValueType>
constexpr inline expression_holder<tensor_expression<ValueType>>
diff(expression_holder<tensor_expression<ValueType>> const &expr,
     expression_holder<tensor_expression<ValueType>> const &arg) {
  tensor_differentiation<ValueType> eval(arg);
  return eval.apply(expr);
}

template <typename ValueType>
inline auto eval(expression_holder<tensor_expression<ValueType>> expr) {
  tensor_evaluator<ValueType> eval;
  return eval.apply(expr);
}

// template <typename T>
// [[nodiscard]] inline auto
// identity_tensor(expression_holder<tensor_expression<T>> const &expr) {
//   const auto &I{get_identity_tensor<T>(expr.get().dim())};
//   const auto &tensor{expr.get()};

//   if (tensor.rank() == 1) {
//     return I;
//   }

//   if (expr.get().rank() == 2) {
//     return otimesu(I, I);
//   }

//   auto outer_expr{make_expression<simple_outer_product<T>>(tensor.dim(),
//                                                            tensor.rank() *
//                                                            2)};
//   auto &outer{outer_expr.template get<simple_outer_product<T>>()};
//   sequence basis(tensor.rank() * 2);
//   for (std::size_t i{0}; i < tensor.rank(); ++i) {
//     outer.push_back(I);
//     basis[(i * 2)] = i + 1;
//     basis[(i * 2) + 1] = tensor.rank() + i + 1;
//   }
//   return permute_indices(std::move(outer_expr), std::move(basis));
// }
} // namespace numsim::cas

#endif // TENSOR_FUNCTIONS_SYMTM_H
