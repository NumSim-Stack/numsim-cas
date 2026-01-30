#ifndef NUMSIM_CAS_TRAITS_H
#define NUMSIM_CAS_TRAITS_H

#include "numsim_cas_forward.h"
#include "tensor/data/tensor_data.h"
#include "tensor/data/tensor_data_eval.h"
#include <concepts>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <variant>

// #if !defined(SYMTM_USE_VARIANT) || !defined(SYMTM_USE_POLY)
// #if defined(__clang__)
// #if __clang_major__ >= 14
// #define SYMTM_USE_VARIANT
// #else
// #define SYMTM_USE_POLY
// #endif
// #elif defined(__GNUC__) || defined(__GNUG__)
// #if __GNUC__ >= 12
// #define SYMTM_USE_VARIANT
// #else
// #define SYMTM_USE_POLY
// #endif
// #elif defined(_MSC_VER)
// #define SYMTM_USE_POLY
// #endif
// #endif

namespace numsim::cas {

using sequence = std::vector<std::size_t>;

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
using expr_set = std::set<ExprType>; //, detail::expression_comparator>;

template <typename ExprType>
using expr_map =
    std::map<ExprType, ExprType>; //, detail::expression_comparator>;

template <typename T>
using tensor_data_ptr = std::unique_ptr<tensor_data_base<T>>;

// template<typename...Args>
// using variant = boost::variant2::variant<Args...>;
// using boost::variant2::holds_alternative;
// using boost::variant2::visit;
// using boost::variant2::get;

struct visitor_output {};
struct visitor_evaluate {};
struct visitor_derivative {};

// template <typename VisitorType, typename ExprBase>
// struct get_visitor
//{
//   static_assert(false, "get_visitor not defined");
// };

template <typename Derived, typename Base> class expression_crtp;

// template <typename VariantType, typename ExprType>
// class variant_wrapper : public expression_crtp<variant_wrapper<VariantType,
// ExprType>, ExprType>
//{
// public:
//   using base_expr =
//       expression_crtp<variant_wrapper<VariantType, ExprType>, ExprType>;

//  template<typename ...Args>
//  variant_wrapper(Args &&... args):m_data(std::forward<Args>(args)...) {}
//  variant_wrapper(variant_wrapper && data):m_data(std::move(data.m_data)){}
//  variant_wrapper(variant_wrapper const& data):m_data(data.m_data){}
//  variant_wrapper(VariantType && data):m_data(std::move(data)){}
//  variant_wrapper(VariantType const& data):m_data(data){}
// private:
//  VariantType m_data;
//};

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

template <typename ExprType> using umap = std::map<ExprType, ExprType>;
template <typename ExprType> using expr_vector = std::vector<ExprType>;

// template<typename _Visitor, typename... _Variants>
// constexpr std::__detail::__variant::__visit_result_t<_Visitor, _Variants...>
// visit(_Visitor&& __visitor, _Variants&&... __variants){
//   return std::visit(std::forward<_Visitor>(__visitor),
//   std::forward<_Variants>(__variants)...);
// }

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
  using type = T; // std::remove_pointer_t<std::remove_reference_t>;
};

template <typename Derived, typename ValueType>
using tensor_data_eval_up_unary = tensor_data_eval<Derived, ValueType, 3, 8, 1>;

template <typename Derived, typename ValueType>
using tensor_data_eval_up_binary =
    tensor_data_eval<Derived, ValueType, 3, 8, 2>;

// template <typename T, typename Expr> [[nodiscard]] auto make_expression(Expr
// &&expr) {
//   using ExprType = typename get_type_t<Expr>::expr_type;
//   return expression_holder<ExprType>(std::make_unique(std::move(expr)));
// }

// expression base
class expression;
template <typename ExprType> class n_ary_tree;

// tensor
// tensor_fundamentals := {symbol, 0, 1, constant}
// tensor_basic_operators := {+,-,*,/,negative}
// multiplication --> inner product of most right and most left index
// only devision by scalar
// tensor_product_functions := {inner, outer, simple_outer}
class tensor;
class tensor_expression;
class tensor_add;
class tensor_mul;
class tensor_pow;
class tensor_power_diff;
class tensor_negative;
class inner_product_wrapper;
class basis_change_imp;
class outer_product_wrapper;
class kronecker_delta;
class simple_outer_product;
class tensor_symmetry;
class tensor_deviatoric;
class tensor_volumetric;
class tensor_inv;
class tensor_zero;
class identity_tensor;
class tensor_projector;
class tensor_scalar_mul;
class tensor_scalar_div;
class tensor_to_scalar_with_tensor_mul;
class tensor_to_scalar_with_tensor_div;
// det, adj, skew, vol, dev,
//
// using tensor_node =
//     std::variant<tensor<ValueType>, tensor_negative<ValueType>,
//                  inner_product_wrapper<ValueType>,
//                  basis_change_imp<ValueType>,
//                  outer_product_wrapper<ValueType>,
//                  kronecker_delta<ValueType>, simple_outer_product<ValueType>,
//                  tensor_add<ValueType>, tensor_mul<ValueType>,
//                  tensor_symmetry<ValueType>, tensor_inv<ValueType>,
//                  tensor_deviatoric<ValueType>, tensor_volumetric<ValueType>,
//                  tensor_zero<ValueType>, tensor_pow<ValueType>,
//                  identity_tensor<ValueType>, tensor_projector<ValueType>,
//                  tensor_scalar_mul<ValueType>, tensor_power_diff<ValueType>,
//                  tensor_scalar_div<ValueType>,
//                  tensor_to_scalar_with_tensor_mul<ValueType>,
//                  tensor_to_scalar_with_tensor_div<ValueType>>;

// scalar
class scalar_expression;

// scalar_fundamentals := {symbol, 0, 1, constant}
class scalar;
class scalar_zero;
class scalar_one;
class scalar_constant;

// scalar_basic_operators := {+,-,*,/,negative}
// class scalar_div;
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

class scalar_function;

//
// using scalar_node = std::variant<
//     scalar<ValueType>, scalar_zero<ValueType>, scalar_one<ValueType>,
//     scalar_constant<ValueType>,
//     // scalar_basic_operators := {+,-,*,/,negative}
//     scalar_div<ValueType>, scalar_add<ValueType>, scalar_mul<ValueType>,
//     scalar_negative<ValueType>, scalar_function<ValueType>,
//     // scalar_trigonometric_functions := {cos, sin, tan, asin, acos, atan}
//     scalar_sin<ValueType>, scalar_cos<ValueType>, scalar_tan<ValueType>,
//     scalar_asin<ValueType>, scalar_acos<ValueType>, scalar_atan<ValueType>,
//     // scalar_special_math_functions := {pow, sqrt, log, ln, e^expr, sign,
//     abs} scalar_pow<ValueType>, scalar_sqrt<ValueType>,
//     scalar_log<ValueType>,
//     // scalar_ln<ValueType>,
//     scalar_exp<ValueType>, scalar_sign<ValueType>, scalar_abs<ValueType>>;

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
class tensor_to_scalar_with_scalar_add;
class tensor_to_scalar_with_scalar_mul;
//  class tensor_to_scalar_with_scalar_sub;
class tensor_to_scalar_with_scalar_div;
class scalar_with_tensor_to_scalar_div;
class tensor_to_scalar_pow;
class tensor_to_scalar_pow_with_scalar_exponent;
class tensor_inner_product_to_scalar;
class tensor_to_scalar_zero;
class tensor_to_scalar_one;
class tensor_to_scalar_log;
class tensor_to_scalar_scalar_wrapper;

// var
//
// using tensor_to_scalar_node = std::variant<
//     tensor_trace<ValueType>, tensor_det<ValueType>, tensor_dot<ValueType>,
//     tensor_norm<ValueType>, tensor_to_scalar_negative<ValueType>,
//     tensor_to_scalar_add<ValueType>, tensor_to_scalar_mul<ValueType>,
//     tensor_to_scalar_div<ValueType>,
//     tensor_to_scalar_with_scalar_add<ValueType>,
//     tensor_to_scalar_with_scalar_mul<ValueType>,
//     tensor_to_scalar_with_scalar_div<ValueType>,
//     scalar_with_tensor_to_scalar_div<ValueType>,
//     tensor_to_scalar_pow<ValueType>, tensor_to_scalar_log<ValueType>,
//     tensor_to_scalar_pow_with_scalar_exponent<ValueType>,
//     tensor_inner_product_to_scalar<ValueType>,
//     tensor_to_scalar_zero<ValueType>, tensor_to_scalar_one<ValueType>,
//     tensor_to_scalar_scalar_wrapper<ValueType>>;

////poly
// template <typename Type, typename ValueType>
// using VisitableTensorScalarImpl_t =
//     VisitableImpl<expression_crtp<Type,tensor_scalar_expression<ValueType>>,
//     Type,
//                   trace_wrapper<ValueType>>;

//
// using VisitableTensorScalar_t =
//     Visitable<tensor_scalar_expression<ValueType>, trace_wrapper<ValueType>>;

//
// using VisitorTensorScalar_t = Visitor<trace_wrapper<ValueType>>;

// #if defined(SYMTM_USE_POLY)
template <typename ExprBase, typename T> struct expression_holder_data_type {
  using data_type = ExprBase;
};

// #elif defined(SYMTM_USE_VARIANT)
template <typename ExprBase, typename T> struct expression_holder_data_type;

// template <typename ValueType, typename T>
// struct expression_holder_data_type<tensor_expression<ValueType>, T> {
//   using data_type = tensor_node<ValueType>;
// };

// template <typename ValueType, typename T>
// struct expression_holder_data_type<scalar_expression<ValueType>, T> {
//   using data_type = scalar_node<ValueType>;
// };

//
// struct expression_holder_data_type<scalar_expression<ValueType>,
// scalar_sin<ValueType>> {
//   using data_type = scalar_special_node<ValueType>;
// };

// template <typename ValueType, typename T>
// struct expression_holder_data_type<tensor_to_scalar_expression<ValueType>, T>
// {
//   using data_type = tensor_to_scalar_node<ValueType>;
// };

// #endif

// template<typename T>
// struct get_hash_value{
//   static constexpr inline auto value(T const& x){
//     return x.hash_value();
//   }
// };

template <typename T> struct get_type<std::shared_ptr<T>> {
  using type = T;
};
template <typename T> struct get_type<expression_holder<T>> {
  using type = T;
};
// template <typename T> struct get_type<variant_wrapper<scalar_special_node,
// scalar_expression>> { using type = scalar_expression; }; template
// <typename T> struct get_type<scalar_special_node> { using type =
// scalar_expression; };

template <typename T>
using get_type_t = typename get_type<std::remove_cvref_t<T>>::type;

///
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

// template <std::size_t Index, typename T> struct result : public
// std::true_type {
//   [[maybe_unused]] static constexpr std::size_t index = Index;
//   static constexpr bool is_variant_type = true;
//   using variant_type = T;
// };

// template <std::size_t Index, typename T, typename Variant>
// struct variant_index_inner;

// template <std::size_t Index, typename T, typename First, typename ...Args>
// struct variant_index_inner<Index, T, std::variant<First, Args...>>
//     : public variant_index_inner<Index+1, T, std::variant<Args...>> {};

// template <std::size_t Index, typename T>
// struct variant_index_inner<Index, T, std::variant<>> : public std::false_type
// {};

// template <std::size_t Index, typename T, typename ...Args>
// struct variant_index_inner<Index, T, std::variant<T, Args...>>
//     : public std::true_type {
//   static constexpr std::size_t index = Index;
// };

// template <std::size_t Index, typename T, typename Variant>
// struct variant_index_outer;

// template <std::size_t Index, typename T, typename First, typename ...Args>
// struct variant_index_outer<Index, T, std::variant<First, Args...>>
//     : public std::conditional<variant_index_inner<0, T, typename
//     First::variant_type>::value,
//                               result<Index, First>,
//                               variant_index_outer<Index+1, T,
//                               std::variant<Args...>>>::type
//{};

// template <std::size_t Index, typename T>
// struct variant_index_outer<Index, T, std::variant<>> : public std::false_type
// {};

template <std::size_t Index, typename T, typename Variant> struct variant_index;

// template <typename T, typename Wrapper>
// struct variant_index_inner_loop : public variant_index<0, T, typename
// Wrapper::variant_type>, public std::false_type
//{};

template <std::size_t Index, typename T>
struct variant_index<Index, T, std::variant<>> : public std::false_type {};

template <std::size_t Index, typename T, typename First, typename... Args>
struct variant_index<Index, T, std::variant<First, Args...>>
    : public variant_index<Index + 1, T, std::variant<Args...>> {};

// template <std::size_t Index, typename T, typename First, typename... Args>
// struct variant_index<Index, T, std::variant<First, Args...>>
//     : public std::conditional<
//           variant_index_inner_loop<T, First>::value,
//           result<Index, First>,
//           variant_index<Index + 1, T, std::variant<Args...>>
//           >::type
//{};

template <std::size_t Index, typename T, typename... Args>
struct variant_index<Index, T, std::variant<T, Args...>>
    : public std::true_type {
  [[maybe_unused]] static constexpr std::size_t index = Index;
};

// template <typename T, typename... Args>
//[[nodiscard]] auto make_expression(Args &&...args) {
//   using ExprType = typename get_type_t::expr_type;
//   using variant = typename expression_holder_data_type<ExprType,
//   T>::data_type; return
//   expression_holder<ExprType>(std::make_shared<variant>(
//       std::in_place_index<variant_index<variant, T>()>,
//       std::forward<Args>(args)...));
// }

// template <typename T, typename... Args>
//[[nodiscard]] auto make_trigonometric_expression(Args&&... args) {
//   using value_type = typename T::value_type;
//   using expr_type = scalar_expression<value_type>;
//   using variant = scalar_node<value_type>;
//   return
//   expression_holder<expr_type>(std::make_shared<variant>(std::in_place_index<variant_index<variant,
//   trigonometric_functions<value_type>>()>,
//       trigonometric_functions<value_type>(T(std::forward<Args>(args)...))));
// }

// template <typename LHS, typename RHS> struct base_expression;

// template <typename EXPR> struct base_expression<EXPR, EXPR> {
//   using type = EXPR;
// };

//
// struct base_expression<scalar_expression<ValueType>,
//                        tensor_expression<ValueType>> {
//   using type = tensor_expression<ValueType>;
// };

//
// struct base_expression<tensor_expression<ValueType>,
//                        scalar_expression<ValueType>> {
//   using type = tensor_expression<ValueType>;
// };

//
// struct base_expression<scalar_expression<ValueType>,
//                        tensor_to_scalar_expression<ValueType>> {
//   using type = tensor_to_scalar_expression<ValueType>;
// };

//
// struct base_expression<tensor_to_scalar_expression<ValueType>,
//                        scalar_expression<ValueType>> {
//   using type = tensor_to_scalar_expression<ValueType>;
// };

//
// struct base_expression<tensor_expression<ValueType>,
//                        tensor_to_scalar_expression<ValueType>> {
//   using type = tensor_expression<ValueType>;
// };

//
// struct base_expression<tensor_to_scalar_expression<ValueType>,
//                        tensor_expression<ValueType>> {
//   using type = tensor_expression<ValueType>;
// };

// template <typename LHS, typename RHS>
// using base_expression_t =
//     typename base_expression<std::decay_t<LHS>, std::decay_t<RHS>>::type;

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

// expr_holder<B> ⊗ number => expr_holder<B>
template <class B, class N>
requires detail::is_number_v<N>
struct result_expression<expression_holder<B>, N> {
  using type = expression_holder<B>;
};

// number ⊗ expr_holder<B> => expr_holder<B>
template <class N, class B>
requires detail::is_number_v<N>
struct result_expression<N, expression_holder<B>> {
  using type = expression_holder<B>;
};

// expr_holder<L> ⊗ expr_holder<R> => expr_holder<promoted>
template <class LBase, class RBase>
struct result_expression<expression_holder<LBase>, expression_holder<RBase>> {
  using type = expression_holder<detail::promote_expr_base_t<LBase, RBase>>;
};

template <class LHS, class RHS>
using result_expression_t =
    typename result_expression<std::decay_t<LHS>, std::decay_t<RHS>>::type;

} // namespace numsim::cas

#endif // NUMSIM_CAS_TRAITS_H
