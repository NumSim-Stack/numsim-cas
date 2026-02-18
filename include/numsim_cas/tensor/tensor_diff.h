#ifndef TENSOR_DIFF_H
#define TENSOR_DIFF_H

#include <numsim_cas/core/diff.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/visitors/tensor_differentiation.h>

namespace numsim::cas {

inline expression_holder<tensor_expression>
tag_invoke(detail::diff_fn, std::type_identity<tensor_expression>,
           std::type_identity<tensor_expression>,
           expression_holder<tensor_expression> const &expr,
           expression_holder<tensor_expression> const &arg) {
  tensor_differentiation d(arg);
  return d.apply(expr);
}

} // namespace numsim::cas

#endif // TENSOR_DIFF_H
