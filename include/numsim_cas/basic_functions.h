#ifndef BASIC_FUNCTIONS_H
#define BASIC_FUNCTIONS_H

#include "numsim_cas_type_traits.h"
#include "numsim_cas_forward.h"
#include "scalar/scalar_constant.h"

namespace numsim::cas {

template<typename Expr>
[[nodiscard]] inline auto get_id(Expr const& expr)noexcept{
  return std::visit([](auto const&val){return val.get_id();}, *expr);
}

template<typename Type, typename Expr>
[[nodiscard]] inline bool is_same(Expr const& expr)noexcept{
  //const auto id{std::visit([](auto const&val){return val.get_id();}, *expr)};
  //return Type::get_id() == id;
  assert(expr.is_valid());
  return holds_alternative<Type>(*expr);
}

template <typename T, typename... Args>
[[nodiscard]] auto make_expression(Args &&...args) {
  using ExprType = typename get_type_t<T>::expr_type;
  using variant = typename expression_holder_data_type<ExprType, T>::data_type;
  using variant_traits = variant_index<0, T, variant>;
  if constexpr (variant_traits::value){
    return expression_holder<ExprType>(std::make_shared<variant>(
        std::in_place_index<variant_traits::index>,
        std::forward<Args>(args)...));
  }else{
    static_assert(!variant_traits::value, "Type not included in variant");
  }
}

template <typename T, typename Expr>
[[nodiscard]] auto copy_expression(expression_holder<Expr> const& expr) {
  using ExprType = typename get_type_t<T>::expr_type;
  using variant = typename expression_holder_data_type<ExprType, T>::data_type;
  using variant_traits = variant_index<0, T, variant>;

  if constexpr (variant_traits::value){
    return expression_holder<ExprType>(std::make_shared<variant>(
        std::in_place_index<variant_traits::index>, expr.template get<T>()));
  } else {
    static_assert(!variant_traits::value, "Type not included in variant");
  }
}

template <typename T, typename Expr>
[[nodiscard]] auto copy_expression(expression_holder<Expr> && expr) {
  return std::move(expr);
}

template<typename ValueType, typename... Args>
auto make_scalar_variable(Args && ... args){
  return std::make_tuple(make_expression<scalar<ValueType>>(std::forward<Args>(args))...);
}

template<typename ValueType, typename... Args>
auto make_scalar_constant(Args && ... args){
  return std::make_tuple(make_expression<scalar_constant<ValueType>>(std::forward<Args>(args))...);
}

template<typename ValueType, typename Args>
auto make_scalar_constant(Args && args){
  return make_expression<scalar_constant<ValueType>>(std::forward<Args>(args));
}

template<typename ValueType, typename... Args>
auto make_tensor_variable(Args && ... args){
  return std::make_tuple(make_expression<tensor<ValueType>>(std::get<0>(std::forward<Args>(args)),
                                                            std::get<1>(std::forward<Args>(args)),
                                                            std::get<2>(std::forward<Args>(args)))...);
}

}
#endif // BASIC_FUNCTIONS_H
