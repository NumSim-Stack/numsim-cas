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
using expr_set = std::set<ExprType, detail::expression_comparator>;

template <typename ExprType>
using expr_map = std::map<ExprType, ExprType, detail::expression_comparator>;

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

template <typename ExprType> using umap = std::map<std::size_t, ExprType>;

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

namespace detail {
/// class for counting the different expression types
/// like scalar, tensorial, matrix, ...
template <typename ExpressionBase> struct expression_id_imp {
  static inline type_id m_id{0};
  static inline type_id m_max_id{0};

  static type_id getID() {
    m_max_id++;
    return m_id++;
  }
};

template <typename Type, typename ExpressionBase = void> struct expression_id {
  static inline type_id value{expression_id_imp<ExpressionBase>::getID()};
};
} // namespace detail

template <typename Expr> class expression_holder;
template <typename T> class tensor_expression;

template <typename Derived, typename ValueType, std::size_t, std::size_t,
          std::size_t>
class tensor_data_eval;

template <typename T> struct get_type {
  using type = T; // std::remove_pointer_t<std::remove_reference_t<T>>;
};

template <typename Derived, typename ValueType>
using tensor_data_eval_up_unary = tensor_data_eval<Derived, ValueType, 3, 8, 1>;

template <typename Derived, typename ValueType>
using tensor_data_eval_up_binary =
    tensor_data_eval<Derived, ValueType, 3, 8, 2>;

// template <typename T, typename Expr> [[nodiscard]] auto make_expression(Expr
// &&expr) {
//   using ExprType = typename get_type_t<Expr>::expr_type;
//   return expression_holder<ExprType>(std::make_unique<T>(std::move(expr)));
// }

struct call_tensor {
  template <typename Tensor>
  static constexpr inline auto dim(Tensor const &tensor) {
    return tensor.dim();
  }

  template <typename Tensor>
  static constexpr inline auto rank(Tensor const &tensor) {
    return tensor.rank();
  }

  template <typename ValueType>
  static constexpr inline auto
  dim(expression_holder<tensor_expression<ValueType>> const &tensor) {
    return tensor.get().dim();
  }

  template <typename ValueType>
  static constexpr inline auto
  rank(expression_holder<tensor_expression<ValueType>> const &tensor) {
    return tensor.get().rank();
  }
};

// expression base
class expression;
template <typename Derived, typename Base> class expression_crtp;
template <typename ExprType, typename Derived> class n_ary_tree;

// tensor
// tensor_fundamentals := {symbol, 0, 1, constant}
// tensor_basic_operators := {+,-,*,/,negative}
// multiplication --> inner product of most right and most left index
// only devision by scalar
// tensor_product_functions := {inner, outer, simple_outer}
template <typename T> class tensor;
template <typename ValueType> class tensor_expression;
template <typename ValueType> class tensor_add;
template <typename ValueType> class tensor_mul;
template <typename ValueType> class tensor_pow;
template <typename ValueType> class tensor_negative;
template <typename ValueType> class inner_product_wrapper;
template <typename ValueType> class basis_change_imp;
template <typename ValueType> class outer_product_wrapper;
template <typename ValueType> class kronecker_delta;
// template <typename ValueType> class simple_outer_product;
template <typename ValueType> class tensor_scalar_mul;
template <typename ValueType> class tensor_scalar_div;
template <typename ValueType> class tensor_symmetry;
template <typename ValueType> class tensor_zero;
// det, tr, adj, skew, vol, dev,

template <typename ValueType>
using tensor_node =
    std::variant<tensor<ValueType>, tensor_negative<ValueType>,
                 inner_product_wrapper<ValueType>, basis_change_imp<ValueType>,
                 outer_product_wrapper<ValueType>, kronecker_delta<ValueType>,
                 // simple_outer_product<ValueType>,
                 tensor_add<ValueType>, tensor_mul<ValueType>,
                 tensor_scalar_mul<ValueType>, tensor_scalar_div<ValueType>,
                 tensor_symmetry<ValueType>, tensor_zero<ValueType>,
                 tensor_pow<ValueType>>;

// scalar
template <typename ValueType> class scalar_expression;

// scalar_fundamentals := {symbol, 0, 1, constant}
template <typename ValueType> class scalar;
template <typename ValueType> class scalar_zero;
template <typename ValueType> class scalar_one;
template <typename ValueType> class scalar_constant;

// scalar_basic_operators := {+,-,*,/,negative}
template <typename ValueType> class scalar_div;
template <typename ValueType> class scalar_add;
template <typename ValueType> class scalar_sub;
template <typename ValueType> class scalar_mul;
template <typename ValueType> class scalar_negative;

// scalar_trigonometric_functions := {cos, sin, tan, asin, acos, atan}
template <typename ValueType> class scalar_sin;
template <typename ValueType> class scalar_cos;
template <typename ValueType> class scalar_tan;
template <typename ValueType> class scalar_asin;
template <typename ValueType> class scalar_acos;
template <typename ValueType> class scalar_atan;

// scalar_special_math_functions := {pow, sqrt, log, ln, e^expr, sign, abs}
template <typename ValueType> class scalar_pow;
template <typename ValueType> class scalar_sqrt;
template <typename ValueType> class scalar_log;
template <typename ValueType> class scalar_ln;
template <typename ValueType> class scalar_exp;
template <typename ValueType> class scalar_sign;
template <typename ValueType> class scalar_abs;

template <typename ValueType>
using scalar_node = std::variant<
    scalar<ValueType>, scalar_zero<ValueType>, scalar_one<ValueType>,
    scalar_constant<ValueType>,
    // scalar_basic_operators := {+,-,*,/,negative}
    scalar_div<ValueType>, scalar_add<ValueType>,
    // scalar_sub<ValueType>,
    scalar_mul<ValueType>, scalar_negative<ValueType>,
    // scalar_trigonometric_functions := {cos, sin, tan, asin, acos, atan}
    scalar_sin<ValueType>, scalar_cos<ValueType>, scalar_tan<ValueType>,
    scalar_asin<ValueType>, scalar_acos<ValueType>, scalar_atan<ValueType>,
    // scalar_special_math_functions := {pow, sqrt, log, ln, e^expr, sign, abs}
    scalar_pow<ValueType>, scalar_sqrt<ValueType>, scalar_log<ValueType>,
    // scalar_ln<ValueType>,
    scalar_exp<ValueType>, scalar_sign<ValueType>, scalar_abs<ValueType>>;

// tensor valued scalar functions
template <typename ValueType> class tensor_to_scalar_expression;
template <typename ValueType> class tensor_trace;
template <typename ValueType> class tensor_dot;
template <typename ValueType> class tensor_det;
template <typename ValueType> class tensor_norm;
template <typename ValueType> class tensor_to_scalar_negative;
template <typename ValueType> class tensor_to_scalar_add;
template <typename ValueType> class tensor_to_scalar_mul;
template <typename ValueType> class tensor_to_scalar_sub;
template <typename ValueType> class tensor_to_scalar_div;
template <typename ValueType> class tensor_to_scalar_with_scalar_add;
template <typename ValueType> class tensor_to_scalar_with_scalar_mul;
template <typename ValueType> class tensor_to_scalar_with_scalar_sub;
template <typename ValueType> class tensor_to_scalar_with_scalar_div;
template <typename ValueType> class scalar_with_tensor_to_scalar_div;
template <typename ValueType> class tensor_to_scalar_pow;
template <typename ValueType> class tensor_to_scalar_pow_with_scalar_exponent;
template <typename ValueType> class tensor_inner_product_to_scalar;

// var
template <typename ValueType>
using tensor_to_scalar_node = std::variant<
    tensor_trace<ValueType>, tensor_det<ValueType>, tensor_dot<ValueType>,
    tensor_norm<ValueType>, tensor_to_scalar_negative<ValueType>,
    tensor_to_scalar_add<ValueType>, tensor_to_scalar_mul<ValueType>,
    tensor_to_scalar_sub<ValueType>, tensor_to_scalar_div<ValueType>,
    tensor_to_scalar_with_scalar_add<ValueType>,
    tensor_to_scalar_with_scalar_mul<ValueType>,
    tensor_to_scalar_with_scalar_sub<ValueType>,
    tensor_to_scalar_with_scalar_div<ValueType>,
    scalar_with_tensor_to_scalar_div<ValueType>,
    tensor_to_scalar_pow<ValueType>,
    tensor_to_scalar_pow_with_scalar_exponent<ValueType> /*,
     tensor_inner_product_to_scalar<ValueType>*/
    >;

////poly
// template <typename Type, typename ValueType>
// using VisitableTensorScalarImpl_t =
//     VisitableImpl<expression_crtp<Type,tensor_scalar_expression<ValueType>>,
//     Type,
//                   trace_wrapper<ValueType>>;

// template <typename ValueType>
// using VisitableTensorScalar_t =
//     Visitable<tensor_scalar_expression<ValueType>, trace_wrapper<ValueType>>;

// template <typename ValueType>
// using VisitorTensorScalar_t = Visitor<trace_wrapper<ValueType>>;

// #if defined(SYMTM_USE_POLY)
template <typename ExprBase, typename T> struct expression_holder_data_type {
  using data_type = ExprBase;
};

// #elif defined(SYMTM_USE_VARIANT)
template <typename ExprBase, typename T> struct expression_holder_data_type;

template <typename ValueType, typename T>
struct expression_holder_data_type<tensor_expression<ValueType>, T> {
  using data_type = tensor_node<ValueType>;
};

template <typename ValueType, typename T>
struct expression_holder_data_type<scalar_expression<ValueType>, T> {
  using data_type = scalar_node<ValueType>;
};
// template <typename ValueType>
// struct expression_holder_data_type<scalar_expression<ValueType>,
// scalar_sin<ValueType>> {
//   using data_type = scalar_special_node<ValueType>;
// };

template <typename ValueType, typename T>
struct expression_holder_data_type<tensor_to_scalar_expression<ValueType>, T> {
  using data_type = tensor_to_scalar_node<ValueType>;
};
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
// template <typename T> struct get_type<variant_wrapper<scalar_special_node<T>,
// scalar_expression<T>>> { using type = scalar_expression<T>; }; template
// <typename T> struct get_type<scalar_special_node<T>> { using type =
// scalar_expression<T>; };

template <typename T>
using get_type_t = typename get_type<remove_cvref_t<T>>::type;

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
//   using ExprType = typename get_type_t<T>::expr_type;
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

template <typename LHS, typename RHS> struct base_expression;

template <typename EXPR> struct base_expression<EXPR, EXPR> {
  using type = EXPR;
};

template <typename ValueType>
struct base_expression<scalar_expression<ValueType>,
                       tensor_expression<ValueType>> {
  using type = tensor_expression<ValueType>;
};

template <typename ValueType>
struct base_expression<tensor_expression<ValueType>,
                       scalar_expression<ValueType>> {
  using type = tensor_expression<ValueType>;
};

template <typename ValueType>
struct base_expression<scalar_expression<ValueType>,
                       tensor_to_scalar_expression<ValueType>> {
  using type = tensor_to_scalar_expression<ValueType>;
};

template <typename ValueType>
struct base_expression<tensor_to_scalar_expression<ValueType>,
                       scalar_expression<ValueType>> {
  using type = tensor_to_scalar_expression<ValueType>;
};

template <typename ValueType>
struct base_expression<tensor_expression<ValueType>,
                       tensor_to_scalar_expression<ValueType>> {
  using type = tensor_expression<ValueType>;
};

template <typename ValueType>
struct base_expression<tensor_to_scalar_expression<ValueType>,
                       tensor_expression<ValueType>> {
  using type = tensor_expression<ValueType>;
};

template <typename LHS, typename RHS>
using base_expression_t = typename base_expression<LHS, RHS>::type;

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
concept isScalarOne =
    std::is_same_v<std::remove_cvref_t<T>, scalar_one<typename T::value_type>>;
template <class T>
concept isScalarZero =
    std::is_same_v<std::remove_cvref_t<T>, scalar_zero<typename T::value_type>>;
template <class T>
concept isScalarConstant =
    std::is_same_v<std::remove_cvref_t<T>,
                   scalar_constant<typename T::value_type>>;

} // NAMESPACE numsim::cas

#endif // NUMSIM_CAS_TRAITS_H
