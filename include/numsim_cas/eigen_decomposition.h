#ifndef NUMSIM_CAS_EIGEN_DECOMPOSITION_H
#define NUMSIM_CAS_EIGEN_DECOMPOSITION_H

#include <cstddef>

#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

// #226 — symbolic spectral decomposition of a symmetric rank-2 tensor A.
// A lightweight builder that holds A and mints the individual spectral
// quantities as symbolic nodes (materialised only at evaluation, via
// tmech's eigendecomposition). All indices are 0-based into the
// ascending-sorted eigenvalues, so value(i), basis(i) and normal(i) refer
// to the same eigenpair.
//
//   eigen_decomposition eig(A);
//   eig.value(i);   // λ_i            — eigenvalue        (scalar)
//   eig.basis(i);   // E_i = n_i ⊗ n_i — eigenprojection   (rank-2 tensor)
//   eig.normal(i);  // n_i            — eigenvector        (rank-1 tensor)
//
// basis(i) is the sign-free quantity (E_i is unique); normal(i) carries the
// usual ± eigenvector sign ambiguity — prefer basis(i) for differentiation.
//
// Repeated eigenvalues: value(i) is always well defined, but within a
// degenerate eigenspace the per-index basis(i)/normal(i) split is arbitrary
// (any orthonormal basis of the subspace is valid). Only basis-invariant
// combinations are unique — the sum of basis(i) over a repeated eigenvalue
// (the full eigenspace projector), Σ_i basis(i) = I, and A = Σ_i value(i)
// basis(i). tmech supplies a valid orthonormal eigenbasis regardless, so
// these identities hold; do not rely on an individual basis(i)/normal(i)
// when its eigenvalue is repeated.
class eigen_decomposition {
public:
  explicit eigen_decomposition(expression_holder<tensor_expression> expr);

  [[nodiscard]] expression_holder<tensor_to_scalar_expression>
  value(std::size_t index) const;

  [[nodiscard]] expression_holder<tensor_expression>
  basis(std::size_t index) const;

  [[nodiscard]] expression_holder<tensor_expression>
  normal(std::size_t index) const;

  // The fourth-order derivative ∂E_i/∂A of the i-th eigenprojection with
  // respect to A (Miehe/Simo spectral derivative, distinct eigenvalues):
  //
  //   ∂E_a/∂A = Σ_{b≠a} 1/(λ_a − λ_b) [ E_a ⊠ E_b + E_b ⊠ E_a ],
  //   (X ⊠ Y)_ijkl = X_ik Y_jl  (tmech otimesu)
  //
  // A rank-4 tensor, symbolic (the 1/(λ_a−λ_b) factors are eigenvalue
  // nodes). It acts on symmetric increments; at a repeated eigenvalue the
  // factor diverges (the eigenprojection is not differentiable there).
  // This is what a consistent tangent of an isotropic tensor function
  // f(A) = Σ f(λ_i) E_i needs (#227).
  [[nodiscard]] expression_holder<tensor_expression>
  basis_derivative(std::size_t index) const;

  [[nodiscard]] expression_holder<tensor_expression> const &
  expr() const noexcept {
    return m_expr;
  }

private:
  expression_holder<tensor_expression> m_expr;
};

} // namespace numsim::cas

#endif // NUMSIM_CAS_EIGEN_DECOMPOSITION_H
