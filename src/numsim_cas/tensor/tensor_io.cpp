#include <numsim_cas/tensor/tensor_io.h>
#include <numsim_cas/tensor/visitors/tensor_printer.h>
#include <ostream>

namespace numsim::cas {
std::ostream &operator<<(std::ostream &os,
                         expression_holder<tensor_expression> const &expr) {
  tensor_printer<std::ostream> p(os);
  p.apply(expr);
  return os;
}
} // namespace numsim::cas
