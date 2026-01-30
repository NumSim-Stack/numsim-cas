#ifndef RESULT_EXPRESSION_H
#define RESULT_EXPRESSION_H

#include <numsim_cas/core/literal.h>
#include <numsim_cas/numsim_cas_forward.h> // must declare scalar_expression/tensor_expression/...
#include <type_traits>

namespace numsim::cas {

template <class Base> class expression_holder;

namespace detail {

template <class T> struct is_expression_holder : std::false_type {};

template <class Base>
struct is_expression_holder<expression_holder<Base>> : std::true_type {};

template <class T>
inline constexpr bool is_expression_holder_v =
    is_expression_holder<std::decay_t<T>>::value;

template <class T> struct holder_base;

template <class Base> struct holder_base<expression_holder<Base>> {
  using type = Base;
};

template <class T>
using holder_base_t = typename holder_base<std::decay_t<T>>::type;

// domain “rank” (extend if you add more domains)
template <class Base> struct expr_rank;

template <>
struct expr_rank<scalar_expression> : std::integral_constant<int, 1> {};
template <>
struct expr_rank<tensor_to_scalar_expression> : std::integral_constant<int, 2> {
};
template <>
struct expr_rank<tensor_expression> : std::integral_constant<int, 3> {};

// map rank -> base type
template <int Rank> struct rank_to_base;
template <> struct rank_to_base<1> {
  using type = scalar_expression;
};
template <> struct rank_to_base<2> {
  using type = tensor_to_scalar_expression;
};
template <> struct rank_to_base<3> {
  using type = tensor_expression;
};

template <int Rank> using rank_to_base_t = typename rank_to_base<Rank>::type;

} // namespace detail

template <class LHS, class RHS, class = void>
struct result_expression; // primary undefined if unsupported

template <class Base, class T>
requires std::is_arithmetic_v<std::remove_cvref_t<T>>
struct result_expression<expression_holder<Base>, T> {
  using type = expression_holder<Base>;
};

template <class T, class Base>
requires std::is_arithmetic_v<std::remove_cvref_t<T>>
struct result_expression<T, expression_holder<Base>> {
  using type = expression_holder<Base>;
};

// expression + expression -> max rank domain
template <class LB, class RB>
struct result_expression<expression_holder<LB>, expression_holder<RB>> {
  static constexpr int rank =
      (detail::expr_rank<LB>::value > detail::expr_rank<RB>::value)
          ? detail::expr_rank<LB>::value
          : detail::expr_rank<RB>::value;
  using type = expression_holder<detail::rank_to_base_t<rank>>;
};

template <class LHS, class RHS>
using result_expression_t =
    typename result_expression<detail::decay_t<LHS>,
                               detail::decay_t<RHS>>::type;

} // namespace numsim::cas

#endif // RESULT_EXPRESSION_H
