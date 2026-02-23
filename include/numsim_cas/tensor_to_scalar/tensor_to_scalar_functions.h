#ifndef TENSOR_TO_SCALAR_FUNCTIONS_H
#define TENSOR_TO_SCALAR_FUNCTIONS_H

#include <numsim_cas/tensor/sequence.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

[[nodiscard]] expression_holder<tensor_to_scalar_expression> dot_product(
    expression_holder<tensor_expression> const &lhs, sequence &&lhs_indices,
    expression_holder<tensor_expression> const &rhs, sequence &&rhs_indices);

[[nodiscard]] expression_holder<tensor_to_scalar_expression>
dot(expression_holder<tensor_expression> const &expr);

[[nodiscard]] expression_holder<tensor_to_scalar_expression>
trace(expression_holder<tensor_expression> const &expr);

[[nodiscard]] expression_holder<tensor_to_scalar_expression>
norm(expression_holder<tensor_expression> const &expr);

[[nodiscard]] expression_holder<tensor_to_scalar_expression>
det(expression_holder<tensor_expression> const &expr);

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_FUNCTIONS_H
