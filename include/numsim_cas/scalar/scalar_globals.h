#ifndef SCALAR_GLOBALS_H
#define SCALAR_GLOBALS_H

#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {
// scalar_zero and scalar_one are dedicated leaf node TYPES (with their own
// get_id()) — semantic singletons in the algebra. The factories here
// return shared instances of those types.
//
// DO NOT add get_scalar_half() or any other get_scalar_<rational>() here.
// scalar_constant(scalar_number{n, d}) is a value-bearing node, not a
// dedicated type — promoting it into this singleton family is a category
// mismatch (tried, rolled back in commit ad4d824). Callers that want to
// dedup a hot-path scalar_constant should use a function-local static
// inside their TU; see tensor_differentiation.cpp's inv-diff visitor for
// the precedent.
const expression_holder<scalar_expression> &get_scalar_zero() noexcept;
const expression_holder<scalar_expression> &get_scalar_one() noexcept;
} // namespace numsim::cas

#endif // SCALAR_GLOBALS_H
