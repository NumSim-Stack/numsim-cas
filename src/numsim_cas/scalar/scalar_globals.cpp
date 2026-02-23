#include <numsim_cas/basic_functions.h>
#include <numsim_cas/scalar/scalar_globals.h>
#include <numsim_cas/scalar/scalar_one.h>
#include <numsim_cas/scalar/scalar_zero.h>

namespace numsim::cas {

expression_holder<scalar_expression> const &get_scalar_zero() noexcept {
  static auto z = make_expression<scalar_zero>();
  return z;
}

expression_holder<scalar_expression> const &get_scalar_one() noexcept {
  static auto o = make_expression<scalar_one>();
  return o;
}

} // namespace numsim::cas
