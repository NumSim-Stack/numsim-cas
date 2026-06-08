#ifndef TENSOR_TO_SCALAR_DIFF_H
#define TENSOR_TO_SCALAR_DIFF_H

#include <numsim_cas/core/diff.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_differentiation.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_differentiation_wrt_scalar.h>

namespace numsim::cas {

inline expression_holder<tensor_expression>
tag_invoke(detail::diff_fn, std::type_identity<tensor_to_scalar_expression>,
           std::type_identity<tensor_expression>,
           expression_holder<tensor_to_scalar_expression> const &expr,
           expression_holder<tensor_expression> const &arg) {
  tensor_to_scalar_differentiation d(arg);
  return d.apply(expr);
}

// diff(t2s, scalar) → t2s. Closes #285.
// Implementation in tensor_to_scalar_differentiation_wrt_scalar.cpp.
expression_holder<tensor_to_scalar_expression>
tag_invoke(detail::diff_fn, std::type_identity<tensor_to_scalar_expression>,
           std::type_identity<scalar_expression>,
           expression_holder<tensor_to_scalar_expression> const &expr,
           expression_holder<scalar_expression> const &arg);

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_DIFF_H
