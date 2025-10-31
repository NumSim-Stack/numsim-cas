#ifndef TENSOR_TO_SCALAR_FUNCTIONS_FWD_H
#define TENSOR_TO_SCALAR_FUNCTIONS_FWD_H

#include "tensor_to_scalar_expression.h"

namespace numsim::cas {

template <typename ExprLHS, typename ExprRHS>
constexpr inline auto dot_product(ExprLHS &&lhs, sequence &&lhs_indices,
                                  ExprRHS &&rhs, sequence &&rhs_indices);

template <typename Expr> constexpr inline auto dot(Expr &&expr);

template <typename ValueType, typename StreamType>
constexpr inline void
print(StreamType &out,
      expression_holder<tensor_to_scalar_expression<ValueType>> const &expr,
      Precedence precedence = Precedence::None);

template <typename ValueType>
inline auto
diff(expression_holder<tensor_to_scalar_expression<ValueType>> const &expr,
     expression_holder<tensor_expression<ValueType>> const &arg);

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_FUNCTIONS_FWD_H
