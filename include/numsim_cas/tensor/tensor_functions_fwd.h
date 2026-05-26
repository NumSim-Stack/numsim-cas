#ifndef TENSOR_FUNCTIONS_FWD_H
#define TENSOR_FUNCTIONS_FWD_H

#include "../numsim_cas_type_traits.h"
#include "../printer_base.h"

namespace numsim::cas {

template <typename ExprLHS, typename ExprRHS>
constexpr inline auto inner_product(ExprLHS &&lhs, sequence &&lhs_indices,
                                    ExprRHS &&rhs, sequence &&rhs_indices);

template <typename ExprLHS, typename ExprRHS>
constexpr inline auto inner_product(ExprLHS &&lhs, sequence const &lhs_indices,
                                    ExprRHS &&rhs, sequence const &rhs_indices);

template <typename Expr> constexpr inline auto trans(Expr &&expr);

template <typename Expr> constexpr inline auto inv(Expr &&expr);

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
constexpr inline void print(StreamType &out,
                            expression_holder<tensor_expression> const &expr,
                            Precedence precedence = Precedence::None);

template <typename ValueType>
constexpr inline expression_holder<tensor_expression>
diff(expression_holder<tensor_expression> const &expr,
     expression_holder<tensor_expression> const &arg);

template <typename ValueType>
inline auto eval(expression_holder<tensor_expression> expr);

} // namespace numsim::cas

#endif // TENSOR_FUNCTIONS_FWD_H
