#ifndef TENSOR_OPERATORS_SYMTM_H
#define TENSOR_OPERATORS_SYMTM_H

#include "simplifier/tensor_simplifier_add.h"
#include "simplifier/tensor_simplifier_mul.h"
#include "simplifier/tensor_simplifier_sub.h"
#include "simplifier/tensor_with_scalar_simplifier_div.h"
#include "simplifier/tensor_with_scalar_simplifier_mul.h"
#include "tensor_expression.h"

namespace numsim::cas {

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline auto
binary_add_tensor_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
  const auto &_lhs{*lhs};
  return visit(
      simplifier::tensor_detail::add_base<ExprTypeLHS, ExprTypeRHS>(
          std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
      _lhs);
}

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline auto
binary_sub_tensor_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
  return visit(
      tensor_detail::simplifier::sub_base<ExprTypeLHS, ExprTypeRHS>(
          std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
      *lhs);
}

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline auto
binary_mul_tensor_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
  return visit(
      tensor_detail::simplifier::mul_base<ExprTypeLHS, ExprTypeRHS>(
          std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
      *lhs);
}

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline auto
binary_mul_tensor_with_scalar_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
  // lhs := scalar_expression,
  // rhs := tensor_expression
  return visit(
      tensor_with_scalar_detail::simplifier::mul_base<ExprTypeLHS, ExprTypeRHS>(
          std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
      *lhs);
}

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline auto
binary_div_tensor_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
  return visit(
      tensor_with_scalar_detail::simplifier::div_base<ExprTypeLHS, ExprTypeRHS>(
          std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
      *lhs);
}

template <typename ValueType>
struct operator_overload<expression_holder<tensor_expression<ValueType>>,
                         expression_holder<tensor_expression<ValueType>>> {

  template <typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto add(ExprTypeLHS &&lhs,
                                                 ExprTypeRHS &&rhs) {
    return binary_add_tensor_simplify(std::forward<ExprTypeLHS>(lhs),
                                      std::forward<ExprTypeRHS>(rhs));
  }

  template <typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto mul(ExprTypeLHS &&lhs,
                                                 ExprTypeRHS &&rhs) {
    return binary_mul_tensor_simplify(std::forward<ExprTypeLHS>(lhs),
                                      std::forward<ExprTypeRHS>(rhs));
  }

  template <typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto sub(ExprTypeLHS &&lhs,
                                                 ExprTypeRHS &&rhs) {
    return binary_sub_tensor_simplify(std::forward<ExprTypeLHS>(lhs),
                                      std::forward<ExprTypeRHS>(rhs));
  }
};

template <typename ValueType>
struct operator_overload<expression_holder<scalar_expression<ValueType>>,
                         expression_holder<tensor_expression<ValueType>>> {

  template <typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto mul(ExprTypeLHS &&lhs,
                                                 ExprTypeRHS &&rhs) {
    return binary_mul_tensor_with_scalar_simplify(
        std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
    // return
    // make_expression<tensor_scalar_mul<ValueType>>(std::forward<ExprTypeLHS>(lhs),
    // std::forward<ExprTypeRHS>(rhs));
  }
};

template <typename ValueType>
struct operator_overload<expression_holder<tensor_expression<ValueType>>,
                         expression_holder<scalar_expression<ValueType>>> {

  template <typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto div(ExprTypeLHS &&lhs,
                                                 ExprTypeRHS &&rhs) {
    return binary_div_tensor_simplify(std::forward<ExprTypeLHS>(lhs),
                                      std::forward<ExprTypeRHS>(rhs));
    // return
    // make_expression<tensor_scalar_div<ValueType>>(std::forward<ExprTypeLHS>(lhs),
    // std::forward<ExprTypeRHS>(rhs));
  }

  template <typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto mul(ExprTypeLHS &&lhs,
                                                 ExprTypeRHS &&rhs) {
    return binary_mul_tensor_with_scalar_simplify(
        std::forward<ExprTypeRHS>(rhs), std::forward<ExprTypeLHS>(lhs));
    // return
    // make_expression<tensor_scalar_mul<ValueType>>(std::forward<ExprTypeRHS>(rhs),
    // std::forward<ExprTypeLHS>(lhs));
  }
};

} // namespace numsim::cas

#endif // TENSOR_OPERATORS_SYMTM_H
