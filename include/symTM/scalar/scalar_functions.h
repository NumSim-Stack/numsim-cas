#ifndef SCALAR_FUNCTIONS_H
#define SCALAR_FUNCTIONS_H

#include <ranges>
#include "../expression_holder.h"
#include "../unary_op.h"
#include "../binary_op.h"
#include "scalar_expression.h"

namespace symTM {
namespace detail {

template <typename ValueType>
class contains_symbol
{
public:
  contains_symbol() {}

  template<typename T>
  constexpr inline bool operator()(T const&){
    return false;
  }

  constexpr inline bool operator()(scalar<ValueType> const& visitable){
    return true;
  }

  template<typename Derived>
  constexpr inline bool operator()(unary_op<Derived, scalar_expression<ValueType>> const& visitable){
    return std::visit(*this, visitable.expr().get());
  }

  template<typename Derived>
  constexpr inline bool operator()(binary_op<Derived, scalar_expression<ValueType>> const& visitable){
    return std::visit(*this, visitable.expr_lhs().get()) || std::visit(*this, visitable.expr_rhs().get());
  }

  template<typename Derived>
  constexpr inline bool operator()(n_ary_tree<scalar_expression<ValueType>, Derived> const& visitable){
    for (auto &child : visitable.hash_map() | std::views::values) {
      if(std::visit(*this, child.get())){
        return true;
      }
    }
    return false;
  }

};
}

template<typename ValueType>
bool contains_symbol(expression_holder<scalar_expression<ValueType>> const& expr){
  return std::visit(detail::contains_symbol<ValueType>(), *expr);
}

}
#endif // SCALAR_FUNCTIONS_H
