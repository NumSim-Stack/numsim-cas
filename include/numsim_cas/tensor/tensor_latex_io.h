#ifndef TENSOR_LATEX_IO_H
#define TENSOR_LATEX_IO_H

#include <numsim_cas/latex_config.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <string>

namespace numsim::cas {

template <class ExprBase> class expression_holder;

std::string to_latex(expression_holder<tensor_expression> const &expr,
                     latex_config const &cfg = latex_config::default_config());

} // namespace numsim::cas

#endif // TENSOR_LATEX_IO_H
