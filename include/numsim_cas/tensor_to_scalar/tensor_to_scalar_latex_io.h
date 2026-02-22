#ifndef TENSOR_TO_SCALAR_LATEX_IO_H
#define TENSOR_TO_SCALAR_LATEX_IO_H

#include <numsim_cas/latex_config.h>
#include <string>

namespace numsim::cas {

class tensor_to_scalar_expression;
template <class ExprBase> class expression_holder;

std::string to_latex(expression_holder<tensor_to_scalar_expression> const &expr,
                     latex_config const &cfg = latex_config::default_config());

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_LATEX_IO_H
