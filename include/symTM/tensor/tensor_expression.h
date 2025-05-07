#ifndef TENSOR_EXPRESSION_H
#define TENSOR_EXPRESSION_H

#include <cstdlib>
#include "../expression.h"
#include "../expression_holder.h"
#include "tensor_printer.h"

namespace numsim::cas {

template<typename ValueType>
class tensor_expression : public expression {
public:
  using expr_type = tensor_expression;
  using value_type = ValueType;
  using node_type = tensor_node<value_type>;

  tensor_expression(std::size_t dim, std::size_t rank)
      : m_dim(dim), m_rank(rank) {}

  tensor_expression() = default;
  tensor_expression(tensor_expression const &data):expression(static_cast<expression const&>(data)),m_dim(data.m_dim), m_rank(data.m_rank) {}
  tensor_expression(tensor_expression && data):expression(std::move(static_cast<expression&&>(data))),m_dim(data.m_dim), m_rank(data.m_rank){}
  virtual ~tensor_expression() = default;

  const tensor_expression &operator=(tensor_expression const &) = delete;

  [[nodiscard]] constexpr inline const auto &dim() const noexcept { return m_dim; }

  [[nodiscard]] constexpr inline const auto &rank() const noexcept { return m_rank; }

  constexpr inline auto operator-(){
    //TODO check if this is already negative
    return make_expression<tensor_expression<ValueType>, tensor_negative<ValueType>>(this);
  }

protected:
  std::size_t m_dim;
  std::size_t m_rank;
};

template<typename ValueType>
std::ostream& operator<<(std::ostream & os, expression_holder<tensor_expression<ValueType>> const& expr){
  tensor_printer<ValueType, std::ostream> printer(os);
  printer.apply(expr);
  return os;
}

template<typename ValueType>
struct expression_details<tensor_expression<ValueType>>{
  using variant = tensor_node<ValueType>;

  template<typename Expr>
  static inline auto negative(Expr && expr){
    return make_expression<tensor_negative<ValueType>>(std::forward<Expr>(expr));
  }
};


} // NAMESPACE symTM

#endif // TENSOR_EXPRESSION_H
