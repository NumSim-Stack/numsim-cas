#include <numsim_cas/scalar/scalar_abs.h>

namespace numsim::cas {

scalar_abs::scalar_abs(scalar_abs const &expr)
    : base(static_cast<base const &>(expr)) {}
scalar_abs::scalar_abs(scalar_abs &&expr)
    : base(std::move(static_cast<base &&>(expr))) {}

} // namespace numsim::cas
