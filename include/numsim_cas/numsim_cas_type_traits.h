#ifndef NUMSIM_CAS_TRAITS_H
#define NUMSIM_CAS_TRAITS_H

#include "numsim_cas_forward.h"
#include "tensor/data/tensor_data.h"
#include "tensor/data/tensor_data_eval.h"
#include <concepts>
#include <map>
#include <memory>
#include <set>
#include <variant>

namespace numsim::cas {

namespace detail {
struct expression_comparator {
  template <typename LHS, typename RHS>
  bool operator()(const LHS &lhs, const RHS &rhs) const {
    return lhs.get().hash_value() < rhs.get().hash_value();
  }
};
} // namespace detail

template <typename... Args> using variant = std::variant<Args...>;
using std::get;
using std::holds_alternative;
using std::visit;

template <typename ExprType>
using expr_set = std::set<ExprType>;

template <typename ExprType>
using expr_map = std::map<ExprType, ExprType>;

template <typename T>
using tensor_data_ptr = std::unique_ptr<tensor_data_base<T>>;

struct visitor_output {};
struct visitor_evaluate {};
struct visitor_derivative {};

template <typename Derived, typename Base> class expression_crtp;

struct scalar_expr_less {
  template <typename Expr>
  bool operator()(expression_holder<Expr> const &a,
                  expression_holder<Expr> const &b) const noexcept {
    auto const &ea = a.get();
    auto const &eb = b.get();

    if (ea.hash_value() != eb.hash_value())
      return ea.hash_value() < eb.hash_value();

    if (ea.id() != eb.id())
      return ea.id() < eb.id();

    return std::addressof(ea) < std::addressof(eb);
  }
};

template <typename ExprType> using expr_ordered_map = std::map<ExprType, ExprType>;
template <typename ExprType> using expr_vector = std::vector<ExprType>;

template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

using type_id = unsigned int;

template <typename ExprTypeLHS, typename ExprTypeRHS> struct operator_overload;

template <typename Expr> class expression_holder;

template <typename Derived, typename ValueType, std::size_t, std::size_t,
          std::size_t>
class tensor_data_eval;

template <typename T> struct get_type {
  using type = T;
};

template <typename Derived, typename ValueType>
using tensor_data_eval_up_unary = tensor_data_eval<Derived, ValueType, 3, 8, 1>;

template <typename Derived, typename ValueType>
using tensor_data_eval_up_binary =
    tensor_data_eval<Derived, ValueType, 3, 8, 2>;

// expression base
class expression;
template <typename ExprType> class n_ary_tree;

// scalar
class scalar_expression;

// scalar_fundamentals := {symbol, 0, 1, constant}
class scalar;
class scalar_zero;
class scalar_one;
class scalar_constant;

// scalar_basic_operators := {+,-,*,negative}
class scalar_add;
class scalar_sub;
class scalar_mul;
class scalar_negative;

// scalar_trigonometric_functions := {cos, sin, tan, asin, acos, atan}
class scalar_sin;
class scalar_cos;
class scalar_tan;
class scalar_asin;
class scalar_acos;
class scalar_atan;

// scalar_special_math_functions := {pow, sqrt, log, ln, e^expr, sign, abs}
class scalar_pow;
class scalar_sqrt;
class scalar_log;
class scalar_ln;
class scalar_exp;
class scalar_sign;
class scalar_abs;

class scalar_named_expression;

// tensor valued scalar functions
class tensor_to_scalar_expression;
class tensor_trace;
class tensor_dot;
class tensor_det;
class tensor_norm;
class tensor_to_scalar_negative;
class tensor_to_scalar_add;
class tensor_to_scalar_mul;
class tensor_to_scalar_div;
class tensor_to_scalar_pow;
class tensor_inner_product_to_scalar;
class tensor_to_scalar_zero;
class tensor_to_scalar_one;
class tensor_to_scalar_log;
class tensor_to_scalar_scalar_wrapper;

template <typename ExprBase, typename T> struct expression_holder_data_type {
  using data_type = ExprBase;
};

template <typename ExprBase, typename T> struct expression_holder_data_type;

template <typename T> struct get_type<std::shared_ptr<T>> {
  using type = T;
};
template <typename T> struct get_type<expression_holder<T>> {
  using type = T;
};

template <typename T>
using get_type_t = typename get_type<std::remove_cvref_t<T>>::type;

template <typename T>
concept has_variant_type = requires { typename T::variant_type; };
template <typename T>
concept is_variant_type_index = requires { typename T::type; };

template <typename T, typename Variant> struct variant_index_inner;

template <typename T, typename... Types>
struct variant_index_inner<T, std::variant<T, Types...>>
    : std::integral_constant<std::size_t, 0> {};

template <typename T, typename U, typename... Types>
struct variant_index_inner<T, std::variant<U, Types...>>
    : std::integral_constant<
          std::size_t,
          1 + variant_index_inner<T, std::variant<Types...>>::value> {};

template <typename T, typename Variant> struct variant_index_outer;

template <typename T, typename... SubVariants>
struct variant_index_outer<T, std::variant<SubVariants...>> {
private:
  template <std::size_t Index, typename... Rest> struct helper;

  template <std::size_t Index, typename First, typename... Rest>
  struct helper<Index, First, Rest...> {
    static constexpr bool found =
        variant_index_inner<T, typename First::variant_type>::value <
        std::variant_size<typename First::variant_type>::value;
    static constexpr std::size_t value =
        found ? Index : helper<Index + 1, Rest...>::value;
  };

  template <std::size_t Index> struct helper<Index> {
    static constexpr std::size_t value = -1;
  };

public:
  static constexpr std::size_t value = helper<0, SubVariants...>::value;
};

template <std::size_t Index, typename T, typename Variant> struct variant_index;

template <std::size_t Index, typename T>
struct variant_index<Index, T, std::variant<>> : public std::false_type {};

template <std::size_t Index, typename T, typename First, typename... Args>
struct variant_index<Index, T, std::variant<First, Args...>>
    : public variant_index<Index + 1, T, std::variant<Args...>> {};

template <std::size_t Index, typename T, typename... Args>
struct variant_index<Index, T, std::variant<T, Args...>>
    : public std::true_type {
  [[maybe_unused]] static constexpr std::size_t index = Index;
};

namespace detail {
template <class AlwaysVoid, template <class...> class Op, class... Args>
struct detector {
  using value_t = std::false_type;
};

template <template <class...> class Op, class... Args>
struct detector<std::void_t<Op<Args...>>, Op, Args...> {
  using value_t = std::true_type;
};
} // namespace detail

template <template <class...> class Op, class... Args>
using is_detected = typename detail::detector<void, Op, Args...>::value_t;

template <template <class...> class Op, class... Args>
const auto is_detected_v = is_detected<Op, Args...>::value;

template <typename Type, typename... Arguments>
using has_coefficient = decltype(std::declval<Type>().coeff());

template <typename Type, typename... Arguments>
using has_hash_map = decltype(std::declval<Type>().hash_map());

template <typename Type, typename... Arguments>
using has_update_hash = decltype(std::declval<Type>().update_hash());

template <class T>
concept isScalarOne = std::is_same_v<std::remove_cvref_t<T>, scalar_one>;
template <class T>
concept isScalarZero = std::is_same_v<std::remove_cvref_t<T>, scalar_zero>;
template <class T>
concept isScalarConstant =
    std::is_same_v<std::remove_cvref_t<T>, scalar_constant>;

} // NAMESPACE numsim::cas

namespace numsim::cas {
namespace detail {

template <class LBase, class RBase> struct promote_expr_base;

template <> struct promote_expr_base<scalar_expression, scalar_expression> {
  using type = scalar_expression;
};
template <>
struct promote_expr_base<tensor_to_scalar_expression,
                         tensor_to_scalar_expression> {
  using type = tensor_to_scalar_expression;
};
template <> struct promote_expr_base<tensor_expression, tensor_expression> {
  using type = tensor_expression;
};

template <>
struct promote_expr_base<tensor_to_scalar_expression, tensor_expression> {
  using type = tensor_expression;
};
template <>
struct promote_expr_base<tensor_expression, tensor_to_scalar_expression> {
  using type = tensor_expression;
};

template <>
struct promote_expr_base<tensor_to_scalar_expression, scalar_expression> {
  using type = tensor_to_scalar_expression;
};
template <>
struct promote_expr_base<scalar_expression, tensor_to_scalar_expression> {
  using type = tensor_to_scalar_expression;
};

template <> struct promote_expr_base<tensor_expression, scalar_expression> {
  using type = tensor_expression;
};
template <> struct promote_expr_base<scalar_expression, tensor_expression> {
  using type = tensor_expression;
};

template <class LBase, class RBase>
using promote_expr_base_t = typename promote_expr_base<LBase, RBase>::type;

template <class T>
inline constexpr bool is_number_v = std::is_arithmetic_v<std::decay_t<T>>;

} // namespace detail

template <class LHS, class RHS> struct result_expression; // primary

// expr_holder<B> * number => expr_holder<B>
template <class B, class N>
requires detail::is_number_v<N>
struct result_expression<expression_holder<B>, N> {
  using type = expression_holder<B>;
};

// number * expr_holder<B> => expr_holder<B>
template <class N, class B>
requires detail::is_number_v<N>
struct result_expression<N, expression_holder<B>> {
  using type = expression_holder<B>;
};

// expr_holder<L> * expr_holder<R> => expr_holder<promoted>
template <class LBase, class RBase>
struct result_expression<expression_holder<LBase>, expression_holder<RBase>> {
  using type = expression_holder<detail::promote_expr_base_t<LBase, RBase>>;
};

template <class LHS, class RHS>
using result_expression_t =
    typename result_expression<std::decay_t<LHS>, std::decay_t<RHS>>::type;

} // namespace numsim::cas

#endif // NUMSIM_CAS_TRAITS_H
