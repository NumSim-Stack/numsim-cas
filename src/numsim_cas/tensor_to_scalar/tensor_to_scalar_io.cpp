#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_io.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_printer.h>

namespace numsim::cas {
std::ostream &
operator<<(std::ostream &os,
           expression_holder<tensor_to_scalar_expression> const &expr) {
  tensor_to_scalar_printer<std::ostream> p(os);
  p.apply(expr);
  return os;
}
} // namespace numsim::cas
