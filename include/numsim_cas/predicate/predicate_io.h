#ifndef PREDICATE_IO_H
#define PREDICATE_IO_H

#include <ostream>

namespace numsim::cas {

class predicate_expression;
template <class ExprBase> class expression_holder;

std::ostream &operator<<(std::ostream &os,
                         expression_holder<predicate_expression> const &expr);

} // namespace numsim::cas

#endif // PREDICATE_IO_H
