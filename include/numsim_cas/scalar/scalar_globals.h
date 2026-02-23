#ifndef SCALAR_GLOBALS_H
#define SCALAR_GLOBALS_H

#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {
const expression_holder<scalar_expression> &get_scalar_zero() noexcept;
const expression_holder<scalar_expression> &get_scalar_one() noexcept;
} // namespace numsim::cas

#endif // SCALAR_GLOBALS_H
