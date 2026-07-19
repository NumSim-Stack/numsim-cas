#include <numsim_cas/tensor/tensor_isotropic_functions.h>

#include <cstddef>
#include <vector>

#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/eigen_decomposition.h>
#include <numsim_cas/scalar/scalar_constant.h>
#include <numsim_cas/tensor/isotropic_kind.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>

namespace numsim::cas {

namespace {

expression_holder<tensor_expression>
build_isotropic(expression_holder<tensor_expression> const &A,
                isotropic_kind kind) {
  if (!A.is_valid() || A.get().rank() != 2)
    throw invalid_expression_error(
        "isotropic tensor function: requires a rank-2 tensor");
  auto const dim = A.get().dim();
  if (dim != 2 && dim != 3)
    throw invalid_expression_error(
        "isotropic tensor function: only dimension 2 or 3 is supported");
  return make_expression<tensor_isotropic_function>(A, kind);
}

} // namespace

expression_holder<tensor_expression>
exp(expression_holder<tensor_expression> const &A) {
  return build_isotropic(A, isotropic_kind::exp);
}

expression_holder<tensor_expression>
log(expression_holder<tensor_expression> const &A) {
  return build_isotropic(A, isotropic_kind::log);
}

expression_holder<tensor_expression>
sqrt(expression_holder<tensor_expression> const &A) {
  return build_isotropic(A, isotropic_kind::sqrt);
}

expression_holder<tensor_expression>
isotropic_tangent(expression_holder<tensor_expression> const &A,
                  isotropic_kind kind) {
  // ∂f/∂A = Σ_{i,j} [f; λ_i, λ_j] (E_i ⊙ E_j), with the minor-symmetric
  // product E_i ⊙ E_j = ½(otimesu + otimesl). The i==j terms carry
  // [f;λ_i,λ_i]=f'(λ_i) with E_i ⊙ E_i = E_i⊗E_i; the i≠j terms carry the
  // divided difference [f;λ_i,λ_j] — coincidence-safe and differentiable.
  eigen_decomposition eig(A);
  const std::size_t dim = A.get().dim();
  auto const half = make_expression<scalar_constant>(scalar_number{1, 2});
  expression_holder<tensor_expression> tangent;
  for (std::size_t i = 0; i < dim; ++i) {
    auto Ei = eig.basis(i);
    for (std::size_t j = 0; j < dim; ++j) {
      auto Ej = eig.basis(j);
      auto ddij = make_expression<tensor_to_scalar_divided_difference>(
          A, kind, std::vector<std::size_t>{i, j});
      auto coeff = ddij * half;                      // t2s
      auto sym = otimesu(Ei, Ej) + otimesl(Ei, Ej);  // rank-4
      auto term = std::move(sym) * std::move(coeff); // tensor * t2s
      tangent = tangent.is_valid() ? tangent + term : term;
    }
  }
  return tangent;
}

} // namespace numsim::cas
