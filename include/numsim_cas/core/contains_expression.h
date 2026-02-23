#ifndef CONTAINS_EXPRESSION_H
#define CONTAINS_EXPRESSION_H

#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/numsim_cas_forward.h>

namespace numsim::cas {

// Scalar domain: does haystack contain needle as a sub-expression?
bool contains_expression(expression_holder<scalar_expression> const &haystack,
                         expression_holder<scalar_expression> const &needle);

// Tensor domain: does haystack contain needle as a sub-expression?
bool contains_expression(expression_holder<tensor_expression> const &haystack,
                         expression_holder<tensor_expression> const &needle);

// Cross-domain: does a T2S expression depend on a given tensor variable?
bool depends_on_tensor(
    expression_holder<tensor_to_scalar_expression> const &expr,
    expression_holder<tensor_expression> const &tensor_var);

} // namespace numsim::cas

#endif // CONTAINS_EXPRESSION_H
