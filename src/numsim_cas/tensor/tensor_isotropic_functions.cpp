#include <numsim_cas/tensor/tensor_isotropic_functions.h>

#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/tensor/isotropic_kind.h>
#include <numsim_cas/tensor/tensor_definitions.h>

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

} // namespace numsim::cas
