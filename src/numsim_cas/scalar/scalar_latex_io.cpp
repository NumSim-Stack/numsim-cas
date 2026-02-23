#include <numsim_cas/scalar/scalar_latex_io.h>
#include <numsim_cas/scalar/visitors/scalar_latex_printer.h>
#include <sstream>

namespace numsim::cas {

std::string to_latex(expression_holder<scalar_expression> const &expr) {
  std::stringstream ss;
  scalar_latex_printer<std::stringstream> p(ss);
  p.apply(expr);
  return ss.str();
}

} // namespace numsim::cas
