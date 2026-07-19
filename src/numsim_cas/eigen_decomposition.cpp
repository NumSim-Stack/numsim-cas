#include <numsim_cas/eigen_decomposition.h>

#include <utility>

#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/scalar/scalar_globals.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_std.h>

namespace numsim::cas {

eigen_decomposition::eigen_decomposition(
    expression_holder<tensor_expression> expr)
    : m_expr(std::move(expr)) {
  if (!m_expr.is_valid() || m_expr.get().rank() != 2)
    throw invalid_expression_error(
        "eigen_decomposition: requires a valid rank-2 tensor");
  // tmech's eigendecomposition supports dim 2 and 3; reject other dims at
  // construction rather than with a cryptic error at evaluation.
  auto const dim = m_expr.get().dim();
  if (dim != 2 && dim != 3)
    throw invalid_expression_error(
        "eigen_decomposition: only dimension 2 or 3 is supported");
}

expression_holder<tensor_to_scalar_expression>
eigen_decomposition::value(std::size_t index) const {
  if (index >= m_expr.get().dim())
    throw invalid_expression_error(
        "eigen_decomposition::value: index out of range for tensor dimension");
  return make_expression<tensor_to_scalar_eigenvalue>(m_expr, index);
}

expression_holder<tensor_expression>
eigen_decomposition::basis(std::size_t index) const {
  if (index >= m_expr.get().dim())
    throw invalid_expression_error(
        "eigen_decomposition::basis: index out of range for tensor dimension");
  return make_expression<tensor_eigenprojection>(m_expr, index);
}

expression_holder<tensor_expression>
eigen_decomposition::normal(std::size_t index) const {
  if (index >= m_expr.get().dim())
    throw invalid_expression_error(
        "eigen_decomposition::normal: index out of range for tensor dimension");
  return make_expression<tensor_eigenvector>(m_expr, index);
}

expression_holder<tensor_expression>
eigen_decomposition::basis_derivative(std::size_t a) const {
  if (a >= m_expr.get().dim())
    throw invalid_expression_error("eigen_decomposition::basis_derivative: "
                                   "index out of range for tensor dimension");
  const std::size_t dim = m_expr.get().dim();
  auto const Ea = basis(a);
  auto const la = value(a);
  auto const two = make_expression<tensor_to_scalar_scalar_wrapper>(
      make_expression<scalar_constant>(2));

  // ∂E_a/∂A = Σ_{b≠a} 1/(λ_a − λ_b) [ E_a ⊙ E_b + E_b ⊙ E_a ], with the
  // minor-symmetric product X ⊙ Y = ½(otimesu(X,Y) + otimesl(X,Y)). The ½
  // is folded into the coefficient: 1/(2(λ_a−λ_b)).
  //
  // The symmetric product (rather than bare otimesu) makes the result
  // minor-symmetric — the correct derivative of the evaluated function,
  // since E_a(A) = E_a(sym A) (the evaluator symmetrises its input), so E_a
  // depends on A only through sym(A). It agrees with the raw otimesu form
  // on symmetric increments but, unlike it, also matches finite differences
  // for non-symmetric increments and yields an FE-ready tangent.
  //
  // dim ∈ {2,3} (facade ctor), so there is always ≥ 1 term.
  expression_holder<tensor_expression> result;
  for (std::size_t b = 0; b < dim; ++b) {
    if (b == a)
      continue;
    auto const Eb = basis(b);
    auto coeff = pow(two * (la - value(b)), -get_scalar_one()); // 1/(2(λa−λb))
    auto sym4 =
        otimesu(Ea, Eb) + otimesl(Ea, Eb) + otimesu(Eb, Ea) + otimesl(Eb, Ea);
    auto term = sym4 * coeff;
    result = result.is_valid() ? result + term : term;
  }
  return result;
}

} // namespace numsim::cas
