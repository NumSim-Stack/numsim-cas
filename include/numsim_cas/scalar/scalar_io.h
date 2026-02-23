#ifndef SCALAR_IO_H
#define SCALAR_IO_H

#include <ostream>

namespace numsim::cas {

class scalar_expression;
template <class ExprBase> class expression_holder;

std::ostream &operator<<(std::ostream &os,
                         expression_holder<scalar_expression> const &expr);
} // namespace numsim::cas

#endif // SCALAR_IO_H
