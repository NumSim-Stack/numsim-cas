#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_latex_io.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_latex_printer.h>
#include <sstream>

namespace numsim::cas {

std::string to_latex(expression_holder<tensor_to_scalar_expression> const &expr,
                     latex_config const &cfg) {
  std::stringstream ss;
  tensor_to_scalar_latex_printer<std::stringstream> p(ss, cfg);
  p.apply(expr);
  return ss.str();
}

} // namespace numsim::cas
