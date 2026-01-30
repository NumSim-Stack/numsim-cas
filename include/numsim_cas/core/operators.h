#ifndef OPERATORS_H
#define OPERATORS_H

#include <type_traits>
#include <utility>

#include <numsim_cas/core/binary_ops.h>
#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/core/make_constant.h>
#include <numsim_cas/core/promote_expr.h>
#include <numsim_cas/numsim_cas_type_traits.h>

namespace numsim::cas::detail {

template <class T> struct is_expression_holder : std::false_type {};

template <class Base>
struct is_expression_holder<expression_holder<Base>> : std::true_type {};

template <class T>
inline constexpr bool is_expression_holder_v =
    is_expression_holder<decay_t<T>>::value;

// to_expr_result: maps operands to canonical types (holder or number_literal)
template <class T, class = void>
struct to_expr_result {}; // no ::type => SFINAE

template <class Base> struct to_expr_result<expression_holder<Base>, void> {
  using type = expression_holder<Base>;
};

template <class T>
using to_expr_result_t = typename to_expr_result<decay_t<T>>::type;

template <class T>
concept cas_operand = requires { typename to_expr_result_t<T>; };

template <class L, class R>
concept cas_binary_op =
    (is_expression_holder_v<L> || is_expression_holder_v<R>)&&(
        is_expression_holder_v<L> ||
        std::is_arithmetic_v<std::remove_cvref_t<
            L>>)&&(is_expression_holder_v<R> ||
                   std::is_arithmetic_v<std::remove_cvref_t<R>>);

} // namespace numsim::cas::detail

namespace numsim::cas {

namespace detail {

template <class Holder>
using holder_base_t = typename std::remove_cvref_t<Holder>::expr_type;

template <class H, class T> constexpr auto to_holder_like(T &&v) {
  using Base = holder_base_t<H>;
  return make_constant(std::type_identity<Base>{}, std::forward<T>(v));
}

} // namespace detail

template <class L, class R>
requires detail::cas_binary_op<L, R>
constexpr auto operator+(L &&lhs, R &&rhs) -> result_expression_t<L, R> {
  if constexpr (detail::is_expression_holder_v<L> &&
                detail::is_expression_holder_v<R>) {
    return detail::binary_add(std::forward<L>(lhs), std::forward<R>(rhs));
  } else if constexpr (detail::is_expression_holder_v<L> &&
                       detail::is_arithmetic_v<R>) {
    auto r2 = detail::to_holder_like<L>(std::forward<R>(rhs));
    return detail::binary_add(std::forward<L>(lhs), std::move(r2));
  } else {
    auto l2 = detail::to_holder_like<R>(std::forward<L>(lhs));
    return detail::binary_add(std::move(l2), std::forward<R>(rhs));
  }
}

template <class L, class R>
requires detail::cas_binary_op<L, R>
constexpr auto operator-(L &&lhs, R &&rhs) -> result_expression_t<L, R> {
  if constexpr (detail::is_expression_holder_v<L> &&
                detail::is_expression_holder_v<R>) {
    return detail::binary_sub(std::forward<L>(lhs), std::forward<R>(rhs));
  } else if constexpr (detail::is_expression_holder_v<L> &&
                       detail::is_arithmetic_v<R>) {
    auto r2 = detail::to_holder_like<L>(std::forward<R>(rhs));
    return detail::binary_sub(std::forward<L>(lhs), std::move(r2));
  } else {
    auto l2 = detail::to_holder_like<R>(std::forward<L>(lhs));
    return detail::binary_sub(std::move(l2), std::forward<R>(rhs));
  }
}

template <class L, class R>
requires detail::cas_binary_op<L, R>
constexpr auto operator*(L &&lhs, R &&rhs) -> result_expression_t<L, R> {
  if constexpr (detail::is_expression_holder_v<L> &&
                detail::is_expression_holder_v<R>) {
    return detail::binary_mul(std::forward<L>(lhs), std::forward<R>(rhs));
  } else if constexpr (detail::is_expression_holder_v<L> &&
                       detail::is_arithmetic_v<R>) {
    auto r2 = detail::to_holder_like<L>(std::forward<R>(rhs));
    return detail::binary_mul(std::forward<L>(lhs), std::move(r2));
  } else {
    auto l2 = detail::to_holder_like<R>(std::forward<L>(lhs));
    return detail::binary_mul(std::move(l2), std::forward<R>(rhs));
  }
}

template <class L, class R>
requires detail::cas_binary_op<L, R>
constexpr auto operator/(L &&lhs, R &&rhs) -> result_expression_t<L, R> {
  if constexpr (detail::is_expression_holder_v<L> &&
                detail::is_expression_holder_v<R>) {
    return detail::binary_div(std::forward<L>(lhs), std::forward<R>(rhs));
  } else if constexpr (detail::is_expression_holder_v<L> &&
                       detail::is_arithmetic_v<R>) {
    auto r2 = detail::to_holder_like<L>(std::forward<R>(rhs));
    return detail::binary_div(std::forward<L>(lhs), std::move(r2));
  } else {
    auto l2 = detail::to_holder_like<R>(std::forward<L>(lhs));
    return detail::binary_div(std::move(l2), std::forward<R>(rhs));
  }
}

} // namespace numsim::cas

#endif // OPERATORS_H
