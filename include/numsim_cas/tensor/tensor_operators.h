#ifndef TENSOR_OPERATORS_SYMTM_H
#define TENSOR_OPERATORS_SYMTM_H

#include "operators/tensor_to_scalar/tensor_to_scalar_with_tensor_mul.h"
#include "tensor_expression.h"
#include "tensor_functions.h"

namespace numsim::cas {

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
  }

  template <typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto mul(ExprTypeLHS &&lhs,
                                                 ExprTypeRHS &&rhs) {
    return binary_mul_tensor_with_scalar_simplify(
        std::forward<ExprTypeRHS>(rhs), std::forward<ExprTypeLHS>(lhs));
  }
};

template <typename ValueType>
struct operator_overload<
    expression_holder<tensor_to_scalar_expression<ValueType>>,
    expression_holder<tensor_expression<ValueType>>> {

  template <typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto mul(ExprTypeLHS &&lhs,
                                                 ExprTypeRHS &&rhs) {
    return make_expression<tensor_to_scalar_with_tensor_mul<ValueType>>(
        std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
  }
};

template <typename ValueType>
struct operator_overload<
    expression_holder<tensor_expression<ValueType>>,
    expression_holder<tensor_to_scalar_expression<ValueType>>> {

  //  template <typename ExprTypeLHS, typename ExprTypeRHS>
  //  [[nodiscard]] static constexpr inline auto div(ExprTypeLHS &&lhs,
  //                                                 ExprTypeRHS &&rhs) {
  //    return binary_div_tensor_simplify(std::forward<ExprTypeLHS>(lhs),
  //                                      std::forward<ExprTypeRHS>(rhs));
  //  }

  template <typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto mul(ExprTypeLHS &&lhs,
                                                 ExprTypeRHS &&rhs) {
    return make_expression<tensor_to_scalar_with_tensor_mul<ValueType>>(
        std::forward<ExprTypeRHS>(rhs), std::forward<ExprTypeLHS>(lhs));
  }
};

} // namespace numsim::cas

#endif // TENSOR_OPERATORS_SYMTM_H
