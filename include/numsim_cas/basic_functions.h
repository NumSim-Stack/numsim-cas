#ifndef BASIC_FUNCTIONS_H
#define BASIC_FUNCTIONS_H

#include "numsim_cas_forward.h"
#include "numsim_cas_type_traits.h"
#include "scalar/scalar_constant.h"
#include <ranges>

namespace numsim::cas {

template <typename Type, typename Expr>
[[nodiscard]] inline bool is_same(Expr const &expr) noexcept {
  assert(expr.is_valid());
  return Type::get_id() == expr.get().id();
}

template <typename Type, typename Expr>
[[nodiscard]] inline std::optional<std::reference_wrapper<const Type>>
is_same_r(Expr const &expr) noexcept {
  assert(expr.is_valid());
  if (Type::get_id() == expr.get().id()) {
    return std::cref(expr.template get<Type>());
  } else {
    return {};
  }
}

template <typename Type> inline auto get_first() {}

template <typename Type, typename Base>
inline auto get_all(n_ary_tree<Base> const &tree) {
  using expr_holder_t = typename Base::expr_holder_t;
  std::vector<expr_holder_t> result;
  for (const auto &expr : tree.hash_map() | std::views::values) {
    if (is_same<Type>(expr)) {
      result.push_back(expr);
    }
  }
  return result;
}

template <typename T, typename... Args>
[[nodiscard]] auto make_expression(Args &&...args) {
  return expression_holder<typename T::expr_t>(
      std::make_shared<T>(std::forward<Args>(args)...));
}

template <typename T, typename Expr>
[[nodiscard]] auto copy_expression(expression_holder<Expr> const &expr) {
  using ExprType = typename get_type_t<T>::expr_type;
  using variant = typename expression_holder_data_type<ExprType, T>::data_type;
  using variant_traits = variant_index<0, T, variant>;

  if constexpr (variant_traits::value) {
    return expression_holder<ExprType>(std::make_shared<variant>(
        std::in_place_index<variant_traits::index>, expr.template get<T>()));
  } else {
    static_assert(!variant_traits::value, "Type not included in variant");
  }
}

template <typename T, typename Expr>
[[nodiscard]] auto copy_expression(expression_holder<Expr> &&expr) {
  return std::move(expr);
}

template <typename... Args> auto make_scalar_variable(Args &&...args) {
  return std::make_tuple(make_expression<scalar>(std::forward<Args>(args))...);
}

template <typename... Args> auto make_scalar_constant(Args &&...args) {
  return std::make_tuple(
      make_expression<scalar_constant>(std::forward<Args>(args))...);
}

template <typename Args> auto make_scalar_constant(Args &&args) {
  return make_expression<scalar_constant>(std::forward<Args>(args));
}

template <typename Expr>
auto make_scalar_named_expression(std::string &&name, Expr &&expr) {
  return make_expression<scalar_named_expression>(std::move(name),
                                                  std::forward<Expr>(expr));
}

template <typename... Args> auto make_tensor_variable(Args &&...args) {
  return std::make_tuple(
      make_expression<tensor>(std::get<0>(std::forward<Args>(args)),
                              std::get<1>(std::forward<Args>(args)),
                              std::get<2>(std::forward<Args>(args)))...);
}

} // namespace numsim::cas

#endif // BASIC_FUNCTIONS_H
