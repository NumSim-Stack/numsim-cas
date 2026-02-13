#ifndef TENSOR_SCALAR_OPERATORS_H
#define TENSOR_SCALAR_OPERATORS_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/make_constant.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/core/promote_expr.h>

#include <numsim_cas/scalar/scalar_globals.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/tensor/operators/tensor/tensor_add.h>
#include <numsim_cas/tensor/tensor_zero.h>

#include <numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_add.h>
#include <numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_mul.h>
#include <numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_sub.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_std.h>

namespace numsim::cas::detail {
// scalar binary ops
template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<tensor_to_scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<tensor_to_scalar_expression>>
inline expression_holder<tensor_to_scalar_expression>
tag_invoke(add_fn, [[maybe_unused]] L &&lhs, [[maybe_unused]] R &&rhs) {
  auto &_lhs{lhs.template get<tensor_to_scalar_visitable_t>()};
  tensor_to_scalar_detail::simplifier::add_base visitor(std::forward<L>(lhs),
                                                        std::forward<R>(rhs));
  return _lhs.accept(visitor);
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<tensor_to_scalar_expression>>
inline expression_holder<tensor_to_scalar_expression>
tag_invoke(add_fn, [[maybe_unused]] L &&lhs, [[maybe_unused]] R &&rhs) {
  return rhs +
         make_expression<tensor_to_scalar_scalar_wrapper>(std::forward<L>(lhs));
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<tensor_to_scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<scalar_expression>>
inline expression_holder<tensor_to_scalar_expression>
tag_invoke(add_fn, [[maybe_unused]] L &&lhs, [[maybe_unused]] R &&rhs) {

  return lhs +
         make_expression<tensor_to_scalar_scalar_wrapper>(std::forward<R>(rhs));
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<tensor_to_scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<tensor_to_scalar_expression>>
inline expression_holder<tensor_to_scalar_expression>
tag_invoke(sub_fn, [[maybe_unused]] L &&lhs, [[maybe_unused]] R &&rhs) {
  if (lhs == rhs) {
    return make_expression<tensor_to_scalar_zero>();
  }
  auto &_lhs{lhs.template get<tensor_to_scalar_visitable_t>()};
  tensor_to_scalar_detail::simplifier::sub_base visitor(std::forward<L>(lhs),
                                                        std::forward<R>(rhs));
  return _lhs.accept(visitor);
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<tensor_to_scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<tensor_to_scalar_expression>>
inline expression_holder<tensor_to_scalar_expression>
tag_invoke(mul_fn, [[maybe_unused]] L &&lhs, [[maybe_unused]] R &&rhs) {
  auto &_lhs{lhs.template get<tensor_to_scalar_visitable_t>()};
  tensor_to_scalar_detail::simplifier::mul_base visitor(std::forward<L>(lhs),
                                                        std::forward<R>(rhs));
  return _lhs.accept(visitor);
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<tensor_to_scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<scalar_expression>>
inline expression_holder<tensor_to_scalar_expression>
tag_invoke(mul_fn, L &&lhs, R &&rhs) {
  return lhs *
         make_expression<tensor_to_scalar_scalar_wrapper>(std::forward<R>(rhs));
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<tensor_to_scalar_expression>>
inline expression_holder<tensor_to_scalar_expression>
tag_invoke(mul_fn, L &&lhs, R &&rhs) {
  return rhs *
         make_expression<tensor_to_scalar_scalar_wrapper>(std::forward<L>(lhs));
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<tensor_to_scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<scalar_expression>>
inline expression_holder<tensor_to_scalar_expression>
tag_invoke(div_fn, L &&lhs, R &&rhs) {
  return std::forward<L>(lhs) * pow(std::forward<R>(rhs), -1);
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<tensor_to_scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<tensor_to_scalar_expression>>
inline expression_holder<tensor_to_scalar_expression>
tag_invoke(div_fn, L &&lhs, R &&rhs) {
  if (lhs == rhs) {
    return make_expression<tensor_to_scalar_one>();
  }
  return std::forward<L>(lhs) * pow(std::forward<R>(rhs), -1);
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<tensor_to_scalar_expression>>
inline expression_holder<tensor_to_scalar_expression>
tag_invoke(div_fn, L &&lhs, R &&rhs) {
  return std::forward<L>(lhs) * pow(std::forward<R>(rhs), -1);
}

template <class T>
requires std::is_arithmetic_v<std::remove_cvref_t<T>>
expression_holder<scalar_expression>
tag_invoke(make_constant_fn, std::type_identity<tensor_to_scalar_expression>,
           T &&v) {
  return tag_invoke(make_constant_fn{}, std::type_identity<scalar_expression>{},
                    std::forward<T>(v));
}

} // namespace numsim::cas::detail

// namespace numsim::cas {

// template <typename ExprTypeLHS, typename ExprTypeRHS>
// [[nodiscard]] constexpr inline auto
// binary_add_tensor_to_scalar_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
//   return visit(
//       tensor_to_scalar_detail::simplifier::add_base<ExprTypeLHS,
//       ExprTypeRHS>(
//           std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
//       *lhs);
// }

// template <typename ExprTypeLHS, typename ExprTypeRHS>
// [[nodiscard]] constexpr inline auto
// binary_sub_tensor_to_scalar_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
//   return visit(
//       tensor_to_scalar_detail::simplifier::sub_base<ExprTypeLHS,
//       ExprTypeRHS>(
//           std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
//       *lhs);
// }

// template <typename ExprTypeLHS, typename ExprTypeRHS>
// [[nodiscard]] constexpr inline auto
// binary_mul_tensor_to_scalar_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
//   return visit(
//       tensor_to_scalar_detail::simplifier::mul_base<ExprTypeLHS,
//       ExprTypeRHS>(
//           std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
//       *lhs);
// }

// template <typename ExprTypeLHS, typename ExprTypeRHS>
// [[nodiscard]] constexpr inline auto
// binary_div_tensor_to_scalar_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
//   return visit(
//       tensor_to_scalar_detail::simplifier::div_base<ExprTypeLHS,
//       ExprTypeRHS>(
//           std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
//       *lhs);
// }

// template <typename ExprTypeLHS, typename ExprTypeRHS>
// [[nodiscard]] constexpr inline auto
// binary_add_tensor_to_scalar_with_scalar_simplify(ExprTypeLHS &&lhs,
//                                                  ExprTypeRHS &&rhs) {
//   return visit(
//       tensor_to_scalar_with_scalar_detail::simplifier::add_base<ExprTypeLHS,
//                                                                 ExprTypeRHS>(
//           std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
//       *lhs);
// }

// // template <typename ExprTypeLHS, typename ExprTypeRHS>
// //[[nodiscard]] constexpr inline auto
// // binary_sub_tensor_to_scalar_with_scalar_simplify(ExprTypeLHS &&lhs,
// // ExprTypeRHS &&rhs){
// //   return
// //
// visit(tensor_to_scalar_with_scalar_detail::simplifier::sub_base<ExprTypeLHS,
// //
// ExprTypeRHS>(std::forward<ExprTypeLHS>(lhs),std::forward<ExprTypeRHS>(rhs)),
// //   *lhs);
// // }

// template <typename ExprTypeLHS, typename ExprTypeRHS>
// [[nodiscard]] constexpr inline auto
// binary_mul_tensor_to_scalar_with_scalar_simplify(ExprTypeLHS &&lhs,
//                                                  ExprTypeRHS &&rhs) {
//   return nullptr; /*visit(
//        tensor_to_scalar_with_scalar_detail::simplifier::mul_base<ExprTypeLHS,
//                                                                  ExprTypeRHS>(
//            std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
//        *lhs);*/
// }

// template <typename ExprTypeLHS, typename ExprTypeRHS>
// [[nodiscard]] constexpr inline auto
// binary_div_tensor_to_scalar_with_scalar_simplify(ExprTypeLHS &&lhs,
//                                                  ExprTypeRHS &&rhs) {
//   return nullptr; /*visit(
//        tensor_to_scalar_with_scalar_detail::simplifier::div_base<ExprTypeLHS,
//                                                                  ExprTypeRHS>(
//            std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
//        *lhs);*/
// }

// struct operator_overload<expression_holder<tensor_to_scalar_expression>,
//                          expression_holder<tensor_to_scalar_expression>> {

//   template <typename ExprTypeLHS, typename ExprTypeRHS>
//   [[nodiscard]] static constexpr inline auto add(ExprTypeLHS &&lhs,
//                                                  ExprTypeRHS &&rhs) {
//     return
//     binary_add_tensor_to_scalar_simplify(std::forward<ExprTypeLHS>(lhs),
//                                                 std::forward<ExprTypeRHS>(rhs));
//   }

//   template <typename ExprTypeLHS, typename ExprTypeRHS>
//   [[nodiscard]] static constexpr inline auto mul(ExprTypeLHS &&lhs,
//                                                  ExprTypeRHS &&rhs) {
//     return
//     binary_mul_tensor_to_scalar_simplify(std::forward<ExprTypeLHS>(lhs),
//                                                 std::forward<ExprTypeRHS>(rhs));
//   }

//   template <typename ExprTypeLHS, typename ExprTypeRHS>
//   [[nodiscard]] static constexpr inline auto sub(ExprTypeLHS &&lhs,
//                                                  ExprTypeRHS &&rhs) {
//     return
//     binary_sub_tensor_to_scalar_simplify(std::forward<ExprTypeLHS>(lhs),
//                                                 std::forward<ExprTypeRHS>(rhs));
//   }

//   template <typename ExprTypeLHS, typename ExprTypeRHS>
//   [[nodiscard]] static constexpr inline auto div(ExprTypeLHS &&lhs,
//                                                  ExprTypeRHS &&rhs) {
//     return
//     binary_div_tensor_to_scalar_simplify(std::forward<ExprTypeLHS>(lhs),
//                                                 std::forward<ExprTypeRHS>(rhs));
//   }
// };

// template <typename ValueType>
// struct operator_overload<expression_holder<scalar_expression>,
//                          expression_holder<tensor_to_scalar_expression>> {

//   template <typename ExprTypeLHS, typename ExprTypeRHS>
//   [[nodiscard]] static constexpr inline auto
//   add([[maybe_unused]] ExprTypeLHS &&lhs, [[maybe_unused]] ExprTypeRHS &&rhs)
//   {
//     // return
//     //
//     binary_add_tensor_to_scalar_with_scalar_simplify<tensor_to_scalar_expression>();
//     return binary_add_tensor_to_scalar_with_scalar_simplify(
//         std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
//   }

//   template <typename ExprTypeLHS, typename ExprTypeRHS>
//   [[nodiscard]] static constexpr inline auto
//   mul([[maybe_unused]] ExprTypeLHS &&lhs, [[maybe_unused]] ExprTypeRHS &&rhs)
//   {
//     return binary_mul_tensor_to_scalar_with_scalar_simplify(
//         std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
//   }

//   template <typename ExprTypeLHS, typename ExprTypeRHS>
//   [[nodiscard]] static constexpr inline auto
//   sub([[maybe_unused]] ExprTypeLHS &&lhs, [[maybe_unused]] ExprTypeRHS &&rhs)
//   {
//     return expression_holder<tensor_to_scalar_expression>();
//     // return binary_sub_tensor_simplify(std::forward<ExprTypeLHS>(lhs),
//     // std::forward<ExprTypeRHS>(rhs));
//   }

//   template <typename ExprTypeLHS, typename ExprTypeRHS>
//   [[nodiscard]] static constexpr inline auto
//   div([[maybe_unused]] ExprTypeLHS &&lhs, [[maybe_unused]] ExprTypeRHS &&rhs)
//   {
//     return binary_div_tensor_to_scalar_with_scalar_simplify(
//         std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
//   }
// };

// struct operator_overload<expression_holder<tensor_to_scalar_expression>,
//                          expression_holder<scalar_expression>> {

//   template <typename ExprTypeLHS, typename ExprTypeRHS>
//   [[nodiscard]] static constexpr inline auto
//   add([[maybe_unused]] ExprTypeLHS &&lhs, [[maybe_unused]] ExprTypeRHS &&rhs)
//   {
//     return binary_add_tensor_to_scalar_with_scalar_simplify(
//         std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
//   }

//   template <typename ExprTypeLHS, typename ExprTypeRHS>
//   [[nodiscard]] static constexpr inline auto
//   mul([[maybe_unused]] ExprTypeLHS &&lhs, [[maybe_unused]] ExprTypeRHS &&rhs)
//   {
//     return binary_mul_tensor_to_scalar_with_scalar_simplify(
//         std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
//   }

//   template <typename ExprTypeLHS, typename ExprTypeRHS>
//   [[nodiscard]] static constexpr inline auto
//   sub([[maybe_unused]] ExprTypeLHS &&lhs, [[maybe_unused]] ExprTypeRHS &&rhs)
//   {
//     return expression_holder<tensor_to_scalar_expression>();
//     // return binary_sub_tensor_simplify(std::forward<ExprTypeLHS>(lhs),
//     // std::forward<ExprTypeRHS>(rhs));
//   }

//   template <typename ExprTypeLHS, typename ExprTypeRHS>
//   [[nodiscard]] static constexpr inline auto
//   div([[maybe_unused]] ExprTypeLHS &&lhs, [[maybe_unused]] ExprTypeRHS &&rhs)
//   {
//     return binary_div_tensor_to_scalar_with_scalar_simplify(
//         std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
//   }
// };

// // template<typename ValueType>
// // struct operator_overload<expression_holder<tensor_expression>,
// //                          expression_holder<tensor_to_scalar_expression>>{

// //  template<typename ExprTypeLHS, typename ExprTypeRHS>
// //  [[nodiscard]] static constexpr inline auto add(ExprTypeLHS && lhs,
// //  ExprTypeRHS && rhs){
// //    return binary_add_tensor_simplify(std::forward<ExprTypeLHS>(lhs),
// //    std::forward<ExprTypeRHS>(rhs));
// //  }

// //  template<typename ExprTypeLHS, typename ExprTypeRHS>
// //  [[nodiscard]] static constexpr inline auto mul(ExprTypeLHS && lhs,
// //  ExprTypeRHS && rhs){
// //  }

// //  template<typename ExprTypeLHS, typename ExprTypeRHS>
// //  [[nodiscard]] static constexpr inline auto sub(ExprTypeLHS && lhs,
// //  ExprTypeRHS && rhs){
// //    return binary_sub_tensor_simplify(std::forward<ExprTypeLHS>(lhs),
// //    std::forward<ExprTypeRHS>(rhs));
// //  }
// //};

// // template<typename ValueType>
// // struct
// // operator_overload<expression_holder<tensor_to_scalar_expression>,
// //                          expression_holder<tensor_expression>>{

// //  template<typename ExprTypeLHS, typename ExprTypeRHS>
// //  [[nodiscard]] static constexpr inline auto add(ExprTypeLHS && lhs,
// //  ExprTypeRHS && rhs){
// //    return binary_add_tensor_simplify(std::forward<ExprTypeLHS>(lhs),
// //    std::forward<ExprTypeRHS>(rhs));
// //  }

// //  template<typename ExprTypeLHS, typename ExprTypeRHS>
// //  [[nodiscard]] static constexpr inline auto mul(ExprTypeLHS && lhs,
// //  ExprTypeRHS && rhs){
// //  }

// //  template<typename ExprTypeLHS, typename ExprTypeRHS>
// //  [[nodiscard]] static constexpr inline auto sub(ExprTypeLHS && lhs,
// //  ExprTypeRHS && rhs){
// //    return binary_sub_tensor_simplify(std::forward<ExprTypeLHS>(lhs),
// //    std::forward<ExprTypeRHS>(rhs));
// //  }
// //};
// } // namespace numsim::cas

#endif // TENSOR_SCALAR_OPERATORS_H
