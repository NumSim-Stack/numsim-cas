#ifndef SCALAR_OPERATORS_H
#define SCALAR_OPERATORS_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/make_constant.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/core/promote_expr.h>

#include <numsim_cas/scalar/scalar_binary_simplify_fwd.h>
#include <numsim_cas/scalar/scalar_constant.h>
#include <numsim_cas/scalar/scalar_domain_traits.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/scalar/scalar_functions_fwd.h>
#include <numsim_cas/scalar/scalar_globals.h>
#include <numsim_cas/scalar/scalar_negative.h>

#include <numsim_cas/scalar/simplifier/scalar_simplifier_add.h>
#include <numsim_cas/scalar/simplifier/scalar_simplifier_mul.h>
#include <numsim_cas/scalar/simplifier/scalar_simplifier_pow.h>
#include <numsim_cas/scalar/simplifier/scalar_simplifier_sub.h>

namespace numsim::cas::detail {

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

  auto &_lhs{lhs.template get<scalar_visitable_t>()};
  simplifier::add_base visitor(std::forward<L>(lhs), std::forward<R>(rhs));
  return _lhs.accept(visitor);
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<scalar_expression>>
inline expression_holder<scalar_expression> tag_invoke(sub_fn, L &&lhs,
                                                       R &&rhs) {
  auto &_lhs{lhs.template get<scalar_visitable_t>()};
  simplifier::sub_base visitor(std::forward<L>(lhs), std::forward<R>(rhs));
  return _lhs.accept(visitor);
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<scalar_expression>>
inline expression_holder<scalar_expression> tag_invoke(mul_fn, L &&lhs,
                                                       R &&rhs) {
  auto &_lhs{lhs.template get<scalar_visitable_t>()};
  simplifier::mul_base visitor(std::forward<L>(lhs), std::forward<R>(rhs));
  return _lhs.accept(visitor);
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
  // constant / constant → exact rational constant
  using traits = domain_traits<scalar_expression>;
  auto lhs_val = traits::try_numeric(lhs);
  auto rhs_val = traits::try_numeric(rhs);
  if (lhs_val && rhs_val && !(*rhs_val == scalar_number{0})) {
    auto result = *lhs_val / *rhs_val;
    if (result == scalar_number{0})
      return get_scalar_zero();
    if (result == scalar_number{1})
      return get_scalar_one();
    return traits::make_constant(result);
  }
  return lhs * pow(rhs, -get_scalar_one());
}

} // namespace numsim::cas::detail

#endif // SCALAR_OPERATORS_H
