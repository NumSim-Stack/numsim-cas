#ifndef TENSOR_TO_SCALAR_FUNCTIONS_H
#define TENSOR_TO_SCALAR_FUNCTIONS_H

#include "tensor_to_scalar_expression.h"
#include "visitors/tensor_to_scalar_differentiation.h"
#include "visitors/tensor_to_scalar_printer.h"

namespace numsim::cas {

template <typename ExprLHS, typename ExprRHS>
constexpr inline auto dot_product(ExprLHS &&lhs, sequence &&lhs_indices,
                                  ExprRHS &&rhs, sequence &&rhs_indices) {
  using ValueType = typename remove_cvref_t<ExprLHS>::value_type;
  assert(call_tensor::rank(lhs) == lhs_indices.size() ||
         call_tensor::rank(rhs) == rhs_indices.size());
  // tensor_to_scalar_expression
  return make_expression<tensor_inner_product_to_scalar<ValueType>>(
      std::forward<ExprLHS>(lhs), std::move(lhs_indices),
      std::forward<ExprRHS>(rhs), std::move(rhs_indices));
}

template <typename ValueType, typename StreamType>
constexpr inline void
print(StreamType &out,
      expression_holder<tensor_to_scalar_expression<ValueType>> const &expr,
      Precedence precedence) {
  tensor_to_scalar_printer<ValueType, StreamType> eval(out);
  eval.apply(expr, precedence);
}

template <typename ValueType>
inline auto
diff(expression_holder<tensor_to_scalar_expression<ValueType>> const &expr,
     expression_holder<tensor_expression<ValueType>> const &arg) {
  tensor_to_scalar_differentiation<ValueType> visitor(arg);
  return visitor.apply(expr);
}
} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_FUNCTIONS_H
