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

// Principal invariants of a rank-2 tensor A — the coefficients of A's
// characteristic polynomial det(A - λI). These are the three scalars
// that characterise A up to similarity:
//
//   I1(A) = tr(A)
//   I2(A) = (tr(A)^2 - tr(A^2)) / 2
//   I3(A) = det(A)
//
// Implemented as compositions of existing primitives (trace, det, the
// tensor-tensor product). The eigenvalue / eigenvector half of #226
// needs new AST nodes — these are the cheap deliverable.
//
// Rank-2 enforced via the underlying trace/det rank-2 asserts.
[[nodiscard]] expression_holder<tensor_to_scalar_expression>
first_invariant(expression_holder<tensor_expression> const &expr);

[[nodiscard]] expression_holder<tensor_to_scalar_expression>
second_invariant(expression_holder<tensor_expression> const &expr);

[[nodiscard]] expression_holder<tensor_to_scalar_expression>
third_invariant(expression_holder<tensor_expression> const &expr);

// The `index`-th eigenvalue of a symmetric rank-2 tensor (#226), ordered
// ascending. Stays symbolic (an opaque scalar node) until evaluation, so
// the chain rule composes over it. `index` is 0-based and must be less
// than the tensor's dimension.
[[nodiscard]] expression_holder<tensor_to_scalar_expression>
eigenvalue(expression_holder<tensor_expression> const &expr, std::size_t index);

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_FUNCTIONS_H
