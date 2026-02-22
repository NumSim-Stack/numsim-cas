#include <numsim_cas/scalar/visitors/scalar_latex_printer.h>
#include <numsim_cas/tensor/visitors/tensor_latex_printer.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_latex_printer.h>

namespace numsim::cas {

template <typename Stream>
void tensor_to_scalar_latex_printer<Stream>::apply(
    expression_holder<tensor_expression> const &expr,
    Precedence parent_precedence) {
  tensor_latex_printer<Stream> p(m_out, m_config);
  p.apply(expr, parent_precedence);
}

template <typename Stream>
void tensor_to_scalar_latex_printer<Stream>::apply(
    expression_holder<scalar_expression> const &expr,
    Precedence parent_precedence) {
  scalar_latex_printer<Stream> p(m_out, m_config);
  p.apply(expr, parent_precedence);
}

template class tensor_to_scalar_latex_printer<std::ostream>;
template class tensor_to_scalar_latex_printer<std::stringstream>;

} // namespace numsim::cas
