#ifndef TENSOR_FUNCTIONS_FWD_H
#define TENSOR_FUNCTIONS_FWD_H

#include "../numsim_cas_type_traits.h"
#include "../printer_base.h"

namespace numsim::cas {

template <typename ValueType>
[[nodiscard]] constexpr inline expression_holder<tensor_expression<ValueType>>
diff(expression_holder<tensor_expression<ValueType>> const &expr,
     expression_holder<tensor_expression<ValueType>> const &arg);

template <typename Expr> constexpr inline auto trans(Expr &&expr);

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_add_tensor_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_sub_tensor_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_mul_tensor_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_mul_tensor_with_scalar_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_div_tensor_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

template <typename ValueType, typename StreamType>
constexpr inline void
print(StreamType &out,
      expression_holder<tensor_expression<ValueType>> const &expr,
      Precedence precedence = Precedence::None);

template <typename ValueType>
constexpr inline expression_holder<tensor_expression<ValueType>>
diff(expression_holder<tensor_expression<ValueType>> const &expr,
     expression_holder<tensor_expression<ValueType>> const &arg);

template <typename ValueType>
inline auto eval(expression_holder<tensor_expression<ValueType>> expr);

} // namespace numsim::cas

#endif // TENSOR_FUNCTIONS_FWD_H
