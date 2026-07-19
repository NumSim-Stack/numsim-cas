#ifndef NUMSIM_CAS_TENSOR_ISOTROPIC_FUNCTIONS_H
#define NUMSIM_CAS_TENSOR_ISOTROPIC_FUNCTIONS_H

#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

// Isotropic tensor functions of a symmetric rank-2 tensor A (#227): the
// scalar function is applied to each eigenvalue and recombined on the
// eigenprojections,
//
//   f(A) = Σ_i f(λ_i) E_i,
//
// built purely by composition on the eigen_decomposition facade (#226) —
// no dedicated AST nodes. Differentiation therefore comes for free from
// the generic chain rule, and yields the exact Daleckii–Krein tangent.
//
// A must be a symmetric rank-2 tensor of dimension 2 or 3 (enforced by
// eigen_decomposition). These overload the scalar/t2s exp/log/sqrt, which
// are concept-constrained to their own domains, so a tensor argument
// dispatches here unambiguously.
//
// pow(A, p) is intentionally NOT provided here: pow(tensor, scalar)
// already means the integer matrix power (tensor_pow). The real-exponent
// isotropic power needs that overload reconciled first — tracked
// separately.

[[nodiscard]] expression_holder<tensor_expression>
exp(expression_holder<tensor_expression> const &A);

[[nodiscard]] expression_holder<tensor_expression>
log(expression_holder<tensor_expression> const &A);

[[nodiscard]] expression_holder<tensor_expression>
sqrt(expression_holder<tensor_expression> const &A);

} // namespace numsim::cas

#endif // NUMSIM_CAS_TENSOR_ISOTROPIC_FUNCTIONS_H
