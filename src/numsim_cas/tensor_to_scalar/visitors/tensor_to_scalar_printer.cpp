#include <numsim_cas/scalar/visitors/scalar_printer.h>
#include <numsim_cas/tensor/visitors/tensor_printer.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_printer.h>

namespace numsim::cas {
template <typename Stream>
void tensor_to_scalar_printer<Stream>::apply(
    expression_holder<tensor_expression> const &expr,
    Precedence parent_precedence) noexcept {
  tensor_printer<std::ostream> p(m_out);
  p.apply(expr, parent_precedence);
}

template <typename Stream>
void tensor_to_scalar_printer<Stream>::apply(
    expression_holder<scalar_expression> const &expr,
    Precedence parent_precedence) noexcept {
  scalar_printer<std::ostream> p(m_out);
  p.apply(expr, parent_precedence);
}

template class tensor_to_scalar_printer<std::ostream>;
template class tensor_to_scalar_printer<std::stringstream>;
} // namespace numsim::cas
