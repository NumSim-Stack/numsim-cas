#ifndef SCALAR_OPERATORS_H
#define SCALAR_OPERATORS_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/make_constant.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/core/promote_expr.h>

#include <numsim_cas/scalar/scalar_binary_simplify_fwd.h>
#include <numsim_cas/scalar/scalar_constant.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/scalar/scalar_functions_fwd.h>
#include <numsim_cas/scalar/scalar_globals.h>
namespace numsim::cas::detail {

// // scalar -> scalar promotion (identity)
// expression_holder<scalar_expression>
// tag_invoke(promote_expr_fn,
//            std::type_identity<scalar_expression>,
//            expression_holder<scalar_expression> expr);

// // scalar binary ops
// template<class L, class R>
// requires std::same_as<std::remove_cvref_t<L>,
// expression_holder<scalar_expression>> &&
//          std::same_as<std::remove_cvref_t<R>,
//          expression_holder<scalar_expression>>
// expression_holder<scalar_expression>
// tag_invoke(add_fn, L&& lhs, R&& rhs);

// template<class L, class R>
// requires std::same_as<std::remove_cvref_t<L>,
// expression_holder<scalar_expression>> &&
//          std::same_as<std::remove_cvref_t<R>,
//          expression_holder<scalar_expression>>
// expression_holder<scalar_expression>
// tag_invoke(sub_fn, L&& lhs, R&& rhs);

// template<class L, class R>
// requires std::same_as<std::remove_cvref_t<L>,
// expression_holder<scalar_expression>> &&
//          std::same_as<std::remove_cvref_t<R>,
//          expression_holder<scalar_expression>>
// expression_holder<scalar_expression>
// tag_invoke(mul_fn, L&& lhs, R&& rhs);

// template<class L, class R>
// requires std::same_as<std::remove_cvref_t<L>,
// expression_holder<scalar_expression>> &&
//          std::same_as<std::remove_cvref_t<R>,
//          expression_holder<scalar_expression>>
// expression_holder<scalar_expression>
// tag_invoke(div_fn, L&& lhs, R&& rhs);

// scalar -> scalar promotion (identity)
inline expression_holder<scalar_expression>
tag_invoke(promote_expr_fn, std::type_identity<scalar_expression>,
           expression_holder<scalar_expression> expr) {
  return expr;
}

// scalar binary ops
template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<scalar_expression>>
inline expression_holder<scalar_expression> tag_invoke(add_fn, L &&lhs,
                                                       R &&rhs) {
  if (is_same<scalar_negative>(rhs) &&
      lhs == rhs.template get<scalar_negative>().expr()) {
    return get_scalar_zero();
  }

  if (is_same<scalar_negative>(lhs) &&
      rhs == lhs.template get<scalar_negative>().expr()) {
    return get_scalar_zero();
  }
  return binary_scalar_add_simplify(std::forward<L>(lhs), std::forward<R>(rhs));
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<scalar_expression>>
inline expression_holder<scalar_expression> tag_invoke(sub_fn, L &&lhs,
                                                       R &&rhs) {
  if (lhs == rhs) {
    return get_scalar_zero();
  }
  return binary_scalar_sub_simplify(std::forward<L>(lhs), std::forward<R>(rhs));
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<scalar_expression>>
inline expression_holder<scalar_expression> tag_invoke(mul_fn, L &&lhs,
                                                       R &&rhs) {
  return binary_scalar_mul_simplify(std::forward<L>(lhs), std::forward<R>(rhs));
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<scalar_expression>>
inline expression_holder<scalar_expression> tag_invoke(div_fn, L &&lhs,
                                                       R &&rhs) {
  if (is_same<scalar_zero>(lhs)) {
    return lhs;
  }
  return lhs * pow(rhs, -get_scalar_one());
  // return binary_scalar_div_simplify(std::move(lhs), std::move(rhs));
}

// arithmetic -> scalar_constant
template <class T>
requires std::is_arithmetic_v<std::remove_cvref_t<T>>
expression_holder<scalar_expression>
tag_invoke(make_constant_fn, std::type_identity<scalar_expression>, T &&v) {
  // adapt to your actual scalar number type:
  using V = std::remove_cvref_t<T>;
  if constexpr (std::is_integral_v<T>) {
    if (v == 0)
      return get_scalar_zero();
    if (v == 1)
      return get_scalar_one();
    if (v == -1)
      return -get_scalar_one();
    return make_expression<scalar_constant>(static_cast<double>(v));
  } else {
    // Floating: treat *exact* 0.0 and 1.0 as special
    if (v == T{0})
      return get_scalar_zero();
    if (v == T{1})
      return get_scalar_one();
    if (v == T{-1})
      return -get_scalar_one();
    return make_expression<scalar_constant>(static_cast<double>(v));
  }
  return make_expression<scalar_constant>(static_cast<V>(v));
}

} // namespace numsim::cas::detail

#endif // SCALAR_OPERATORS_H
