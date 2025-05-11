#ifndef TENSOR_TO_SCALAR_EXPRESSION_H
#define TENSOR_TO_SCALAR_EXPRESSION_H

#include <cstdlib>
#include "../numsim_cas_type_traits.h"
#include "../expression.h"
#include "../expression_holder.h"
#include "visitors/tensor_scalar_printer.h"

namespace numsim::cas {

template<typename ValueType>
class tensor_to_scalar_expression : public expression {
public:
  using expr_type = tensor_to_scalar_expression;
  using value_type = ValueType;
  using node_type = tensor_to_scalar_node<value_type>;

  tensor_to_scalar_expression() = default;
  tensor_to_scalar_expression(tensor_to_scalar_expression const &data):expression(static_cast<expression const&>(data)){}
  tensor_to_scalar_expression(tensor_to_scalar_expression && data):expression(std::move(static_cast<expression&&>(data))){}
  virtual ~tensor_to_scalar_expression() = default;

  const tensor_to_scalar_expression &operator=(tensor_to_scalar_expression const &) = delete;

  constexpr inline auto operator-(){
    //TODO check if this is already negative
    return make_expression<tensor_to_scalar_expression<ValueType>, tensor_negative<ValueType>>(this);
  }
};

template<typename ValueType>
std::ostream& operator<<(std::ostream & os, expression_holder<tensor_to_scalar_expression<ValueType>> const& expr){
  tensor_to_scalar_printer<ValueType, std::ostream> printer(os);
  printer.apply(expr);
  return os;
}

template<typename ValueType>
struct expression_details<tensor_to_scalar_expression<ValueType>>{
  using variant = tensor_to_scalar_node<ValueType>;

  template<typename Expr>
  static inline auto negative(Expr && expr){
    return make_expression<tensor_to_scalar_negative<ValueType>>(std::forward<Expr>(expr));
  }
};

} // NAMESPACE symTM

#endif // TENSOR_TO_SCALAR_EXPRESSION_H
