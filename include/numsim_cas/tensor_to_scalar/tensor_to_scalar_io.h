#ifndef TENSOR_TO_SCALAR_IO_H
#define TENSOR_TO_SCALAR_IO_H

#include <ostream>

namespace numsim::cas {

class tensor_to_scalar_expression;
template <class ExprBase> class expression_holder;

std::ostream &
operator<<(std::ostream &os,
           expression_holder<tensor_to_scalar_expression> const &expr);
} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_IO_H
