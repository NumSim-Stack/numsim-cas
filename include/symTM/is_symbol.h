#ifndef IS_SYMBOL_H
#define IS_SYMBOL_H

#include "scalar/scalar_expression.h"

namespace numsim::cas {

template <typename ValueType>
class is_scalar_symbol;

template<typename ExprType>
struct is_symbol;

template<typename T>
struct is_symbol<scalar_expression<T>>{
  using type = is_scalar_symbol<T>;
};

template<typename T>
struct is_symbol<tensor_expression<T>>{
  using type = is_scalar_symbol<T>;
};


template <typename ValueType>
class is_scalar_symbol
{
public:
  is_scalar_symbol() {}

  constexpr inline auto operator()(scalar<ValueType> const&){
    return true;
  }

  template<typename T>
  constexpr inline auto operator()(T const&){
    return false;
  }
};

template <typename ValueType>
class is_tensor_symbol
{
public:
  is_tensor_symbol() {}

  constexpr inline auto operator()(tensor<ValueType> const&){
    return true;
  }

  template<typename T>
  constexpr inline auto operator()(T const&){
    return false;
  }
};

}
#endif // IS_SYMBOL_H
