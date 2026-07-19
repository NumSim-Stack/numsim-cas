#include <numsim_cas/tensor/tensor_isotropic_functions.h>

#include <cstddef>

#include <numsim_cas/eigen_decomposition.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_std.h>

namespace numsim::cas {

namespace {

// f(A) = Σ_i f(λ_i) E_i. `scalar_fn` maps the i-th eigenvalue node (a
// tensor_to_scalar scalar) to f(λ_i); the term f(λ_i)·E_i is a
// tensor × tensor_to_scalar product (→ tensor), summed over the spectrum.
template <typename ScalarFn>
expression_holder<tensor_expression>
spectral_compose(expression_holder<tensor_expression> const &A,
                 ScalarFn scalar_fn) {
  eigen_decomposition eig(A);
  const std::size_t dim = A.get().dim();
  expression_holder<tensor_expression> result;
  for (std::size_t i = 0; i < dim; ++i) {
    auto term = eig.basis(i) * scalar_fn(eig.value(i));
    result = result.is_valid() ? result + term : term;
  }
  return result;
}

} // namespace

expression_holder<tensor_expression>
exp(expression_holder<tensor_expression> const &A) {
  return spectral_compose(A, [](auto const &lambda) { return exp(lambda); });
}

expression_holder<tensor_expression>
log(expression_holder<tensor_expression> const &A) {
  return spectral_compose(A, [](auto const &lambda) { return log(lambda); });
}

expression_holder<tensor_expression>
sqrt(expression_holder<tensor_expression> const &A) {
  return spectral_compose(A, [](auto const &lambda) { return sqrt(lambda); });
}

} // namespace numsim::cas
