#include <numsim_cas/predicate/predicate_io.h>
#include <numsim_cas/predicate/visitors/predicate_printer.h>
#include <ostream>

namespace numsim::cas {

std::ostream &operator<<(std::ostream &os,
                         expression_holder<predicate_expression> const &expr) {
  predicate_printer<std::ostream> p(os);
  p.apply(expr);
  return os;
}

} // namespace numsim::cas
