#include <numsim_cas/scalar/scalar_std.h>
namespace numsim::cas {

std::string to_string(expression_holder<scalar_expression> const &expr) {
  std::ostringstream ss;
  ss << expr;
  return ss.str();
}

} // namespace numsim::cas
