#ifndef SCALAR_EXPRESSION_H
#define SCALAR_EXPRESSION_H

#include <numsim_cas/core/expression.h>
#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/scalar/scalar_visitor_typedef.h>

namespace numsim::cas {

class scalar_expression : public expression {
public:
  using expr_t = scalar_expression;
  using visitor_t = scalar_visitor_t;
  using visitor_const_t = scalar_visitor_const_t;

  template <typename... Args>
  scalar_expression(Args &&...args) : expression(std::forward<Args>(args)...) {}
  scalar_expression(scalar_expression const &data)
      : expression(static_cast<expression const &>(data)) {}
  scalar_expression(scalar_expression &&data)
      : expression(std::move(static_cast<expression &&>(data))) {}
  virtual ~scalar_expression() = default;
  const scalar_expression &operator=(scalar_expression const &) = delete;
};

expression_holder<scalar_expression>
tag_invoke(detail::neg_fn, std::type_identity<scalar_expression>,
           expression_holder<scalar_expression> const &e);

} // namespace numsim::cas

#endif // SCALAR_EXPRESSION_H
