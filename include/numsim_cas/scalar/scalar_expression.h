#ifndef SCALAR_EXPRESSION_H
#define SCALAR_EXPRESSION_H

#include "../expression.h"
#include "../expression_holder.h"
#include "../numsim_cas_type_traits.h"
#include "visitors/scalar_printer.h"

namespace numsim::cas {

template <typename ValueType> class scalar_expression : public expression {
public:
  using expr_type = scalar_expression;
  using value_type = ValueType;
  using node_type = scalar_node<value_type>;

  template <typename... Args>
  scalar_expression(Args &&...args) : expression(std::forward<Args>(args)...) {}
  scalar_expression(scalar_expression const &data)
      : expression(static_cast<expression const &>(data)) {}
  scalar_expression(scalar_expression &&data)
      : expression(std::move(static_cast<expression &&>(data))) {}
  virtual ~scalar_expression() = default;
  const scalar_expression &operator=(scalar_expression const &) = delete;

  constexpr inline auto operator-() {
    // TODO check if this is already negative
    return make_expression<scalar_expression<ValueType>,
                           scalar_negative<ValueType>>(*this);
  }
};

template <typename ValueType>
std::ostream &
operator<<(std::ostream &os,
           expression_holder<scalar_expression<ValueType>> const &expr) {
  scalar_printer<ValueType, std::ostream> printer(os);
  printer.apply(expr);
  return os;
}

template <typename ValueType>
struct expression_details<scalar_expression<ValueType>> {
  using variant = scalar_node<ValueType>;

  template <typename Expr> static inline auto negative(Expr &&expr) {
    return make_expression<scalar_negative<ValueType>>(
        std::forward<Expr>(expr));
  }
};

} // namespace numsim::cas

#endif // SCALAR_EXPRESSION_H
