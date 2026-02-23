#ifndef TENSOR_TO_SCALAR_DIFF_H
#define TENSOR_TO_SCALAR_DIFF_H

#include <numsim_cas/core/diff.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_differentiation.h>

namespace numsim::cas {

inline expression_holder<tensor_expression>
tag_invoke(detail::diff_fn, std::type_identity<tensor_to_scalar_expression>,
           std::type_identity<tensor_expression>,
           expression_holder<tensor_to_scalar_expression> const &expr,
           expression_holder<tensor_expression> const &arg) {
  tensor_to_scalar_differentiation d(arg);
  return d.apply(expr);
}

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_DIFF_H
