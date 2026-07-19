#ifndef NUMSIM_CAS_TENSOR_ISOTROPIC_FUNCTIONS_H
#define NUMSIM_CAS_TENSOR_ISOTROPIC_FUNCTIONS_H

#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

// Isotropic tensor functions of a symmetric rank-2 tensor A (#227):
// f(A) = Σ f(λ_i) E_i, applied spectrally. These build the
// tensor_isotropic_function node, so evaluation is robust at coincident
// eigenvalues (basis-invariant sum) and differentiation dispatches to the
// coincidence-safe Daleckii–Krein tangent (#326).
//
// A must be a symmetric rank-2 tensor of dimension 2 or 3. These overload
// the scalar/t2s exp/log/sqrt, which are concept-constrained to their own
// domains, so a tensor argument dispatches here unambiguously.
//
// pow(A, p) is not provided: pow(tensor, scalar) already means the integer
// matrix power; the real-exponent isotropic power is a separate follow-up.

[[nodiscard]] expression_holder<tensor_expression>
exp(expression_holder<tensor_expression> const &A);

[[nodiscard]] expression_holder<tensor_expression>
log(expression_holder<tensor_expression> const &A);

[[nodiscard]] expression_holder<tensor_expression>
sqrt(expression_holder<tensor_expression> const &A);

} // namespace numsim::cas

#endif // NUMSIM_CAS_TENSOR_ISOTROPIC_FUNCTIONS_H
