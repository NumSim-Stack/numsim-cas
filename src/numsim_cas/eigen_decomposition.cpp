#include <numsim_cas/eigen_decomposition.h>

#include <utility>

#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>

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

} // namespace numsim::cas
