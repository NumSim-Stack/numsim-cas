#ifndef TENSOR_ALGEBRA_KIND_H
#define TENSOR_ALGEBRA_KIND_H

namespace numsim::cas {

// Algebraic properties of a rank-2 tensor's VALUES, orthogonal to the
// projector-space classification carried by tensor_space.
//
// - Orthogonal:           R^T R = R R^T = I. Rotations, reflections.
// - PositiveDefinite:     x^T A x > 0 for all x != 0. Implies symmetric.
//                         Cauchy-Green tensors, mass / stiffness matrices.
// - PositiveSemidefinite: x^T A x >= 0 for all x. Implies symmetric.
//                         Hessians at minima, contact penalty matrices.
//
// Stored as a flat enum (not std::optional) — None is the default sentinel
// for "no algebraic property known".
enum class AlgKind { None, Orthogonal, PositiveDefinite, PositiveSemidefinite };

} // namespace numsim::cas

#endif // TENSOR_ALGEBRA_KIND_H
