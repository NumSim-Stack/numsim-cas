#ifndef SCALAR_DIFF_H
#define SCALAR_DIFF_H

#include <numsim_cas/core/diff.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/scalar/visitors/scalar_differentiation.h>

namespace numsim::cas {

inline expression_holder<scalar_expression>
tag_invoke(detail::diff_fn, std::type_identity<scalar_expression>,
           std::type_identity<scalar_expression>,
           expression_holder<scalar_expression> const &expr,
           expression_holder<scalar_expression> const &arg) {
  scalar_differentiation d(arg);
  return d.apply(expr);
}

} // namespace numsim::cas

#endif // SCALAR_DIFF_H
