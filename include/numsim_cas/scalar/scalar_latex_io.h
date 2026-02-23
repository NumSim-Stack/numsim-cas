#ifndef SCALAR_LATEX_IO_H
#define SCALAR_LATEX_IO_H

#include <string>

namespace numsim::cas {

class scalar_expression;
template <class ExprBase> class expression_holder;

std::string to_latex(expression_holder<scalar_expression> const &expr);

} // namespace numsim::cas

#endif // SCALAR_LATEX_IO_H
