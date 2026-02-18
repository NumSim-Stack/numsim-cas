#ifndef TENSOR_FUNCTIONS_SYMTM_H
#define TENSOR_FUNCTIONS_SYMTM_H

#include <cstdlib>
#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/tensor/data/tensor_data_make_imp.h>
#include <numsim_cas/tensor/sequence.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/visitors/tensor_printer.h>
#include <vector>

namespace numsim::cas {

template <typename T>
constexpr inline auto make_tensor_data(std::size_t dim, std::size_t rank) {
  return make_tensor_data_imp<T>().evaluate(dim, rank);
}

template <typename Expr/*,
          std::enable_if_t<
              std::is_base_of_v<
                  tensor_expression<
                      typename std::remove_cvref_t<Expr>::value_t>,
                  typename std::remove_cvref_t<Expr>::expr_t>,
              bool> = true*/>
constexpr inline auto dev(Expr &&expr) {
  if (expr.get().rank() == 2) {
    return make_expression<tensor_deviatoric>(std::forward<Expr>(expr));
  }
  throw evaluation_error("dev: requires rank-2 tensor");
}

template <typename ExprLHS, typename ExprRHS>
constexpr inline auto inner_product(ExprLHS &&lhs, sequence &&lhs_indices,
                                    ExprRHS &&rhs, sequence &&rhs_indices) {
  // const auto &_lhs{*lhs};
  // inner_product_simplifier<ExprLHS, ExprRHS> simplifier(
  //     std::forward<ExprLHS>(lhs), std::move(lhs_indices),
  //     std::forward<ExprRHS>(rhs), std::move(rhs_indices));
  // return std::visit(simplifier, _lhs);
  return make_expression<inner_product_wrapper>(
      std::forward<ExprLHS>(lhs), std::move(lhs_indices),
      std::forward<ExprRHS>(rhs), std::move(rhs_indices));
}

template <typename ExprLHS, typename ExprRHS>
constexpr inline auto inner_product(ExprLHS &&lhs, sequence const &lhs_indices,
                                    ExprRHS &&rhs,
                                    sequence const &rhs_indices) {
  // assert(call_tensor::rank(lhs) != lhs_indices.size() ||
  //        call_tensor::rank(rhs) != rhs_indices.size());
  // const auto &_lhs{*lhs};
  // inner_product_simplifier<ExprLHS, ExprRHS> simplifier(
  //     std::forward<ExprLHS>(lhs), lhs_indices, std::forward<ExprRHS>(rhs),
  //     rhs_indices);
  // return std::visit(simplifier, _lhs);
  return make_expression<inner_product_wrapper>(
      std::forward<ExprLHS>(lhs), lhs_indices, std::forward<ExprRHS>(rhs),
      rhs_indices);
}

template <typename ExprLHS, typename ExprRHS>
constexpr inline auto otimes(ExprLHS &&lhs, ExprRHS &&rhs) {
  sequence lhs_indices(lhs.get().rank()), rhs_indices(rhs.get().rank());
  std::iota(std::begin(lhs_indices), std::end(lhs_indices),
            std::size_t{0});
  std::iota(std::begin(rhs_indices), std::end(rhs_indices),
            lhs_indices.size());
  return make_expression<outer_product_wrapper>(
      std::forward<ExprLHS>(lhs), std::move(lhs_indices),
      std::forward<ExprRHS>(rhs), std::move(rhs_indices));
}

template <typename ExprLHS, typename ExprRHS>
constexpr inline auto otimes(ExprLHS &&lhs, sequence &&lhs_indices,
                             ExprRHS &&rhs, sequence &&rhs_indices) {
  return make_expression<outer_product_wrapper>(
      std::forward<ExprLHS>(lhs), std::move(lhs_indices),
      std::forward<ExprRHS>(rhs), std::move(rhs_indices));
}

template <typename ExprLHS, typename ExprRHS>
constexpr inline auto otimesu(ExprLHS &&lhs, ExprRHS &&rhs) {
  return make_expression<outer_product_wrapper>(
      std::forward<ExprLHS>(lhs), std::move(sequence{1, 3}),
      std::forward<ExprRHS>(rhs), std::move(sequence{2, 4}));
}

template <typename ExprLHS, typename ExprRHS>
constexpr inline auto otimesl(ExprLHS &&lhs, ExprRHS &&rhs) {
  return make_expression<outer_product_wrapper>(
      std::forward<ExprLHS>(lhs), std::move(sequence{1, 4}),
      std::forward<ExprRHS>(rhs), std::move(sequence{2, 3}));
}

template <typename Expr>
constexpr inline auto permute_indices(Expr &&expr, sequence &&indices) {
  if (is_same<basis_change_imp>(expr)) {
    auto &tensor{expr.template get<basis_change_imp>()};
    const auto &t_indices{tensor.indices()};
    sequence new_order(t_indices.size());
    for (std::size_t i{0}; i < t_indices.size(); ++i) {
      new_order[i] = t_indices[indices[i]];
    }
    return make_expression<basis_change_imp>(tensor.expr(),
                                             std::move(new_order));
  }

  if (is_same<outer_product_wrapper>(expr)) {
    auto &tensor{expr.template get<outer_product_wrapper>()};
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

    // Permute: new[i] = old[perm[i]]  (all 0-based)
    for (std::size_t i{0}; i < indices.size(); ++i) {
      indices_new[i] = indices_old[indices[i]];
    }

    indices_new_lhs.insert(indices_new_lhs.begin(), indices_new.begin(),
                           indices_new.begin() + indices_lhs.size());
    indices_new_rhs.insert(indices_new_rhs.begin(),
                           indices_new.begin() + indices_lhs.size(),
                           indices_new.end());
    return otimes(tensor.expr_lhs(), std::move(indices_new_lhs),
                  tensor.expr_rhs(), std::move(indices_new_rhs));
  }

  // outer_product_wrapper
  return make_expression<basis_change_imp>(std::forward<Expr>(expr),
                                           std::move(indices));
}

template <typename Expr> constexpr inline auto trans(Expr &&expr) {
  return make_expression<basis_change_imp>(std::forward<Expr>(expr),
                                           sequence{2, 1});
}

template <typename Expr> constexpr inline auto inv(Expr &&expr) {
  return make_expression<tensor_inv>(std::forward<Expr>(expr));
}

// template <typename ValueType, typename StreamType>
// constexpr inline void
// print(StreamType &out,
//       expression_holder<tensor_expression> const &expr,
//       Precedence precedence) {
//   tensor_printer<StreamType> eval(out);
//   eval.apply(expr, precedence);
// }

// template <typename ValueType>
// constexpr inline expression_holder<tensor_expression>
// diff(expression_holder<tensor_expression> const &expr,
//      expression_holder<tensor_expression> const &arg) {
//   tensor_differentiation eval(arg);
//   return eval.apply(expr);
// }

// template <typename ValueType>
// inline auto eval(expression_holder<tensor_expression> expr) {
//   tensor_evaluator eval;
//   return eval.apply(expr);
// }

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
