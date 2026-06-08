#ifndef TENSOR_DIFF_H
#define TENSOR_DIFF_H

#include <numsim_cas/core/diff.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/visitors/tensor_differentiation.h>
#include <numsim_cas/tensor/visitors/tensor_differentiation_wrt_scalar.h>

namespace numsim::cas {

inline expression_holder<tensor_expression>
tag_invoke(detail::diff_fn, std::type_identity<tensor_expression>,
           std::type_identity<tensor_expression>,
           expression_holder<tensor_expression> const &expr,
           expression_holder<tensor_expression> const &arg) {
  tensor_differentiation d(arg);
  return d.apply(expr);
}

// diff(tensor T, scalar s) → tensor of same rank as T. Closes #275.
// Implementation in tensor_differentiation_wrt_scalar.cpp.
expression_holder<tensor_expression>
tag_invoke(detail::diff_fn, std::type_identity<tensor_expression>,
           std::type_identity<scalar_expression>,
           expression_holder<tensor_expression> const &expr,
           expression_holder<scalar_expression> const &arg);

} // namespace numsim::cas

#endif // TENSOR_DIFF_H
