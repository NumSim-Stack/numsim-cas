#ifndef TENSOR_TO_SCALAR_EXPRESSION_H
#define TENSOR_TO_SCALAR_EXPRESSION_H

#include <numsim_cas/core/expression.h>
#include <numsim_cas/core/make_negative.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_visitor_typedef.h>

namespace numsim::cas {

class tensor_to_scalar_expression : public expression {
public:
  using expr_t = tensor_to_scalar_expression;
  using visitor_t = tensor_to_scalar_visitor_t;
  using visitor_const_t = tensor_to_scalar_visitor_const_t;

  tensor_to_scalar_expression() = default;
  tensor_to_scalar_expression(tensor_to_scalar_expression const &data)
      : expression(static_cast<expression const &>(data)) {}
  tensor_to_scalar_expression(tensor_to_scalar_expression &&data)
      : expression(std::move(static_cast<expression &&>(data))) {}
  virtual ~tensor_to_scalar_expression() = default;

  const tensor_to_scalar_expression &
  operator=(tensor_to_scalar_expression const &) = delete;
};

expression_holder<tensor_to_scalar_expression>
tag_invoke(detail::neg_fn, std::type_identity<tensor_to_scalar_expression>,
           expression_holder<tensor_to_scalar_expression> const &e);

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_EXPRESSION_H
