#include <numsim_cas/basic_functions.h>
#include <numsim_cas/scalar/scalar_constant.h>
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

expression_holder<scalar_expression> const &get_scalar_half() noexcept {
  static auto h = make_expression<scalar_constant>(scalar_number{1, 2});
  return h;
}

} // namespace numsim::cas
