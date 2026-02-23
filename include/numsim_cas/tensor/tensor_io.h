#ifndef TENSOR_IO_H
#define TENSOR_IO_H

#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

template <class ExprBase> class expression_holder;

std::ostream &operator<<(std::ostream &os,
                         expression_holder<tensor_expression> const &expr);
} // namespace numsim::cas

#endif // TENSOR_IO_H
