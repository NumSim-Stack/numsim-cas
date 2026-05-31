#ifndef BASIC_FUNCTIONS_H
#define BASIC_FUNCTIONS_H

#include "numsim_cas_forward.h"
#include "numsim_cas_type_traits.h"
#include "scalar/scalar_constant.h"
#include <ranges>

namespace numsim::cas {

template <typename Type, typename Expr>
[[nodiscard]] inline bool is_same(Expr const &expr) noexcept {
  if (!expr.is_valid())
    return false;
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

template <typename Type, typename Base>
inline auto get_all(n_ary_tree<Base> const &tree) {
  using expr_holder_t = typename Base::expr_holder_t;
  std::vector<expr_holder_t> result;
  for (const auto &expr : tree.symbol_map() | std::views::values) {
    if (is_same<Type>(expr)) {
      result.push_back(expr);
    }
  }
  return result;
}

template <typename Type, typename Base>
inline auto get_all(n_ary_vector<Base> const &tree) {
  using expr_holder_t = typename Base::expr_holder_t;
  std::vector<expr_holder_t> result;
  for (const auto &expr : tree.data()) {
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

} // namespace numsim::cas

// scalar_make_constant supplies the arithmetic→canonical-form
// `tag_invoke(make_constant_fn, type_identity<scalar_expression>, T)`
// dispatch used by `make_scalar_constant` for arithmetic args (#184
// canonical-form fix). Included AFTER `make_expression` is declared
// (it depends on that), but BEFORE the `make_scalar_constant`
// overloads that depend on it.
#include "scalar/scalar_make_constant.h"

namespace numsim::cas {

namespace detail {
// Single-value factory shared by the 1-arg and N-arg `make_scalar_constant`
// overloads. Arithmetic types route through the canonicalisation in
// `scalar_make_constant.h`'s `tag_invoke(make_constant_fn, …)` — same path
// used by `int * holder` operators — so `make_scalar_constant(int)` and
// `int * holder`-style construction produce structurally identical nodes
// (#184). Non-arithmetic types (e.g. `scalar_number`) fall through to
// direct `scalar_constant` construction unchanged.
template <typename T> auto make_one_scalar_constant(T &&v) {
  if constexpr (std::is_arithmetic_v<std::remove_cvref_t<T>>) {
    return tag_invoke(make_constant_fn{},
                      std::type_identity<scalar_expression>{},
                      std::forward<T>(v));
  } else {
    return make_expression<scalar_constant>(std::forward<T>(v));
  }
}
} // namespace detail

template <typename A, typename B, typename... Rest>
auto make_scalar_constant(A &&a, B &&b, Rest &&...rest) {
  return std::make_tuple(
      detail::make_one_scalar_constant(std::forward<A>(a)),
      detail::make_one_scalar_constant(std::forward<B>(b)),
      detail::make_one_scalar_constant(std::forward<Rest>(rest))...);
}

template <typename Args> auto make_scalar_constant(Args &&args) {
  return detail::make_one_scalar_constant(std::forward<Args>(args));
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
