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
  tensor_to_scalar_expression(tensor_to_scalar_expression &&data) noexcept
      : expression(std::move(static_cast<expression &&>(data))) {}
  ~tensor_to_scalar_expression() override = default;

  const tensor_to_scalar_expression &
  operator=(tensor_to_scalar_expression const &) = delete;
};

template <class T>
concept tensor_to_scalar_expr_holder =
    requires { typename std::remove_cvref_t<T>::expr_type; } &&
    std::is_base_of_v<tensor_to_scalar_expression,
                      typename std::remove_cvref_t<T>::expr_type>;

expression_holder<tensor_to_scalar_expression>
tag_invoke(detail::neg_fn, std::type_identity<tensor_to_scalar_expression>,
           expression_holder<tensor_to_scalar_expression> const &e);

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_EXPRESSION_H
