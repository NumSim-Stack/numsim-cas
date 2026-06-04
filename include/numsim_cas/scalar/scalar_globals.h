#ifndef SCALAR_GLOBALS_H
#define SCALAR_GLOBALS_H

#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {
const expression_holder<scalar_expression> &get_scalar_zero() noexcept;
const expression_holder<scalar_expression> &get_scalar_one() noexcept;
// Rational 1/2 — used by symmetrization kernels (e.g. β-1 inv-diff). Lazy
// magic-static; shared across all callers. Same lifetime contract as
// get_scalar_zero/get_scalar_one.
const expression_holder<scalar_expression> &get_scalar_half() noexcept;
} // namespace numsim::cas

#endif // SCALAR_GLOBALS_H
