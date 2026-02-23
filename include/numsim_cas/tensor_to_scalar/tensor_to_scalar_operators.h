#ifndef TENSOR_TO_SCALAR_OPERATORS_H
#define TENSOR_TO_SCALAR_OPERATORS_H

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
#include <numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_pow.h>
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
  auto &_lhs{lhs.template get<tensor_to_scalar_visitable_t>()};
  tensor_to_scalar_detail::simplifier::sub_base visitor(std::forward<L>(lhs),
                                                        std::forward<R>(rhs));
  return _lhs.accept(visitor);
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<tensor_to_scalar_expression>>
inline expression_holder<tensor_to_scalar_expression>
tag_invoke(sub_fn, L &&lhs, R &&rhs) {
  return make_expression<tensor_to_scalar_scalar_wrapper>(
             std::forward<L>(lhs)) -
         std::forward<R>(rhs);
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<tensor_to_scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<scalar_expression>>
inline expression_holder<tensor_to_scalar_expression>
tag_invoke(sub_fn, L &&lhs, R &&rhs) {
  return std::forward<L>(lhs) -
         make_expression<tensor_to_scalar_scalar_wrapper>(std::forward<R>(rhs));
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
  return std::forward<L>(lhs) *
         pow(make_expression<tensor_to_scalar_scalar_wrapper>(
                 std::forward<R>(rhs)),
             -get_scalar_one());
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<tensor_to_scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<tensor_to_scalar_expression>>
inline expression_holder<tensor_to_scalar_expression>
tag_invoke(div_fn, L &&lhs, R &&rhs) {
  return std::forward<L>(lhs) * pow(std::forward<R>(rhs), -get_scalar_one());
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<tensor_to_scalar_expression>>
inline expression_holder<tensor_to_scalar_expression>
tag_invoke(div_fn, L &&lhs, R &&rhs) {
  return std::forward<L>(lhs) * pow(std::forward<R>(rhs), -get_scalar_one());
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

#endif // TENSOR_TO_SCALAR_OPERATORS_H
