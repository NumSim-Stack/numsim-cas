#include <numsim_cas/tensor/tensor_latex_io.h>
#include <numsim_cas/tensor/visitors/tensor_latex_printer.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_latex_printer.h>
#include <sstream>

namespace numsim::cas {

std::string to_latex(expression_holder<tensor_expression> const &expr,
                     latex_config const &cfg) {
  std::stringstream ss;
  tensor_latex_printer<std::stringstream> p(ss, cfg);
  p.apply(expr);
  return ss.str();
}

// Cross-domain apply for tensor_to_scalar_expression
template <typename StreamType>
void tensor_latex_printer<StreamType>::apply(
    expression_holder<tensor_to_scalar_expression> const &expr,
    Precedence parent_precedence) {
  tensor_to_scalar_latex_printer<StreamType> p(m_out, m_config);
  p.apply(expr, parent_precedence);
}

template class tensor_latex_printer<std::ostream>;
template class tensor_latex_printer<std::stringstream>;

} // namespace numsim::cas
