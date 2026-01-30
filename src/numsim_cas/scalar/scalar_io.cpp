#include <numsim_cas/scalar/scalar_io.h>
#include <numsim_cas/scalar/visitors/scalar_printer.h>
#include <ostream>

namespace numsim::cas {
std::ostream &operator<<(std::ostream &os,
                         expression_holder<scalar_expression> const &expr) {
  scalar_printer<std::ostream> p(os);
  p.apply(expr);
  return os;
}
} // namespace numsim::cas
