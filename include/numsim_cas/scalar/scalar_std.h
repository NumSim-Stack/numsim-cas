#ifndef SCALAR_STD_H
#define SCALAR_STD_H

#include <cassert>
#include <sstream>
#include <string>
#include <type_traits>

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/scalar/scalar_abs.h>
#include <numsim_cas/scalar/scalar_acos.h>
#include <numsim_cas/scalar/scalar_asin.h>
#include <numsim_cas/scalar/scalar_assume.h>
#include <numsim_cas/scalar/scalar_atan.h>
#include <numsim_cas/scalar/scalar_binary_simplify_fwd.h>
#include <numsim_cas/scalar/scalar_constant.h>
#include <numsim_cas/scalar/scalar_cos.h>
#include <numsim_cas/scalar/scalar_eq.h>
#include <numsim_cas/scalar/scalar_exp.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/scalar/scalar_ge.h>
#include <numsim_cas/scalar/scalar_globals.h>
#include <numsim_cas/scalar/scalar_gt.h>
#include <numsim_cas/scalar/scalar_io.h>
#include <numsim_cas/scalar/scalar_le.h>
#include <numsim_cas/scalar/scalar_log.h>
#include <numsim_cas/scalar/scalar_lt.h>
#include <numsim_cas/scalar/scalar_make_constant.h>
#include <numsim_cas/scalar/scalar_ne.h>
#include <numsim_cas/scalar/scalar_negative.h>
#include <numsim_cas/scalar/scalar_one.h>
#include <numsim_cas/scalar/scalar_power.h>
#include <numsim_cas/scalar/scalar_sign.h>
#include <numsim_cas/scalar/scalar_sin.h>
#include <numsim_cas/scalar/scalar_sqrt.h>
#include <numsim_cas/scalar/scalar_tan.h>
#include <numsim_cas/scalar/scalar_zero.h>

namespace numsim::cas {

[[nodiscard]] std::string
to_string(expression_holder<scalar_expression> const &expr);

template <scalar_expr_holder L, scalar_expr_holder R>
[[nodiscard]] auto pow(L &&expr_lhs, R &&expr_rhs) {
  assert(expr_rhs.is_valid());
  assert(expr_lhs.is_valid());

  if (is_same<scalar_one>(expr_lhs) ||
      (is_same<scalar_constant>(expr_lhs) &&
       expr_lhs.template get<scalar_constant>().value() == 1)) {
    return get_scalar_one();
  }

  if (is_same<scalar_zero>(expr_rhs))
    return get_scalar_one();

  if (is_same<scalar_one>(expr_rhs))
    return std::forward<L>(expr_lhs);

  if (is_same<scalar_constant>(expr_rhs) &&
      expr_rhs.template get<scalar_constant>().value() == 1) {
    return std::forward<L>(expr_lhs);
  }

  // Constant folding: pow(numeric, numeric) → numeric
  {
    auto try_num = [](auto const &e) -> std::optional<scalar_number> {
      if (is_same<scalar_zero>(e))
        return scalar_number{0};
      if (is_same<scalar_one>(e))
        return scalar_number{1};
      if (is_same<scalar_constant>(e))
        return e.template get<scalar_constant>().value();
      if (is_same<scalar_negative>(e)) {
        auto inner = [&]() -> std::optional<scalar_number> {
          auto const &neg = e.template get<scalar_negative>().expr();
          if (is_same<scalar_zero>(neg))
            return scalar_number{0};
          if (is_same<scalar_one>(neg))
            return scalar_number{1};
          if (is_same<scalar_constant>(neg))
            return neg.template get<scalar_constant>().value();
          return std::nullopt;
        }();
        if (inner)
          return -(*inner);
      }
      return std::nullopt;
    };
    auto lhs_val = try_num(expr_lhs);
    auto rhs_val = try_num(expr_rhs);
    if (lhs_val && rhs_val) {
      auto result = pow(*lhs_val, *rhs_val);
      if (result) {
        if (*result == scalar_number{0})
          return get_scalar_zero();
        if (*result == scalar_number{1})
          return get_scalar_one();
        return make_expression<scalar_constant>(*result);
      }
    }
  }

  return binary_scalar_pow_simplify(std::forward<L>(expr_lhs),
                                    std::forward<R>(expr_rhs));
}

template <scalar_expr_holder L, typename R>
requires std::is_arithmetic_v<std::remove_cvref_t<R>>
[[nodiscard]] auto pow(L &&expr_lhs, R expr_rhs) {
  auto constant{detail::tag_invoke(detail::make_constant_fn{},
                                   std::type_identity<scalar_expression>{},
                                   expr_rhs)};
  return binary_scalar_pow_simplify(std::forward<L>(expr_lhs),
                                    std::move(constant));
}

template <scalar_expr_holder E> [[nodiscard]] auto sin(E &&e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  if (is_same<scalar_asin>(e))
    return e.template get<scalar_asin>().expr();
  if (is_same<scalar_negative>(e))
    return -sin(e.template get<scalar_negative>().expr());
  return make_expression<scalar_sin>(std::forward<E>(e));
}

template <scalar_expr_holder E> [[nodiscard]] auto cos(E &&e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_one();
  if (is_same<scalar_acos>(e))
    return e.template get<scalar_acos>().expr();
  if (is_same<scalar_negative>(e))
    return cos(e.template get<scalar_negative>().expr());
  return make_expression<scalar_cos>(std::forward<E>(e));
}

template <scalar_expr_holder E> [[nodiscard]] auto tan(E &&e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  if (is_same<scalar_atan>(e))
    return e.template get<scalar_atan>().expr();
  return make_expression<scalar_tan>(std::forward<E>(e));
}

template <scalar_expr_holder E> [[nodiscard]] auto asin(E &&e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  return make_expression<scalar_asin>(std::forward<E>(e));
}

template <scalar_expr_holder E> [[nodiscard]] auto acos(E &&e) {
  if (is_same<scalar_one>(e) ||
      (is_same<scalar_constant>(e) &&
       e.template get<scalar_constant>().value() == scalar_number{1}))
    return get_scalar_zero();
  return make_expression<scalar_acos>(std::forward<E>(e));
}

template <scalar_expr_holder E> [[nodiscard]] auto atan(E &&e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  return make_expression<scalar_atan>(std::forward<E>(e));
}

template <scalar_expr_holder E> [[nodiscard]] auto exp(E &&e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_one();
  if (is_same<scalar_log>(e))
    return e.template get<scalar_log>().expr();
  return make_expression<scalar_exp>(std::forward<E>(e));
}

template <scalar_expr_holder E> [[nodiscard]] auto abs(E &&e) {
  if (is_positive(e) || is_nonnegative(e))
    return std::forward<E>(e);
  if (is_negative(e) || is_nonpositive(e))
    return -std::forward<E>(e);
  return make_expression<scalar_abs>(std::forward<E>(e));
}

template <scalar_expr_holder E> [[nodiscard]] auto sqrt(E &&e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  if (is_same<scalar_one>(e) ||
      (is_same<scalar_constant>(e) &&
       e.template get<scalar_constant>().value() == scalar_number{1}))
    return get_scalar_one();
  if (is_same<scalar_pow>(e)) {
    auto const &p = e.template get<scalar_pow>();
    if (is_same<scalar_constant>(p.expr_rhs()) &&
        p.expr_rhs().template get<scalar_constant>().value() ==
            scalar_number{2}) {
      if (is_nonnegative(p.expr_lhs()) || is_positive(p.expr_lhs()))
        return p.expr_lhs();
    }
  }
  // sqrt(exp(x)) → exp(x/2)
  // Implemented via pow(exp(x), 1/2) which triggers pow_base → exp(x * 1/2)
  if (is_same<scalar_exp>(e)) {
    auto half = make_expression<scalar_constant>(scalar_number{1, 2});
    return binary_scalar_pow_simplify(std::forward<E>(e), std::move(half));
  }
  return make_expression<scalar_sqrt>(std::forward<E>(e));
}

template <scalar_expr_holder E> [[nodiscard]] auto sign(E &&e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  if (is_positive(e))
    return get_scalar_one();
  if (is_negative(e))
    return -get_scalar_one();
  return make_expression<scalar_sign>(std::forward<E>(e));
}

template <scalar_expr_holder E> [[nodiscard]] auto log(E &&e) {
  if (is_same<scalar_one>(e) ||
      (is_same<scalar_constant>(e) &&
       e.template get<scalar_constant>().value() == scalar_number{1}))
    return get_scalar_zero();
  if (is_same<scalar_exp>(e))
    return e.template get<scalar_exp>().expr();
  // log(sqrt(x)) → log(x)/2
  if (is_same<scalar_sqrt>(e)) {
    auto half = make_expression<scalar_constant>(scalar_number{1, 2});
    return log(e.template get<scalar_sqrt>().expr()) * half;
  }
  // log(pow(x, n)) → n * log(x), when x is positive
  if (is_same<scalar_pow>(e)) {
    auto const &p = e.template get<scalar_pow>();
    if (is_positive(p.expr_lhs()))
      return p.expr_rhs() * log(p.expr_lhs());
  }
  return make_expression<scalar_log>(std::forward<E>(e));
}

// ─── Comparison free functions (#136) ────────────────────────────────
// Each returns an expression evaluating to 1.0 if the comparison
// holds, 0.0 otherwise. Construction-time simplifications:
//  * both sides numeric ⇒ fold to scalar_zero/scalar_one
//  * structurally identical sides ⇒ eq/le/ge → 1, lt/gt/ne → 0
namespace detail {
inline std::optional<scalar_number>
scalar_comparison_try_num(expression_holder<scalar_expression> const &e) {
  if (is_same<scalar_zero>(e))
    return scalar_number{0};
  if (is_same<scalar_one>(e))
    return scalar_number{1};
  if (is_same<scalar_constant>(e))
    return e.get<scalar_constant>().value();
  if (is_same<scalar_negative>(e)) {
    auto const &n = e.get<scalar_negative>().expr();
    if (is_same<scalar_zero>(n))
      return scalar_number{0};
    if (is_same<scalar_one>(n))
      return -scalar_number{1};
    if (is_same<scalar_constant>(n))
      return -n.get<scalar_constant>().value();
  }
  return std::nullopt;
}
} // namespace detail

template <scalar_expr_holder L, scalar_expr_holder R>
[[nodiscard]] auto lt(L &&lhs, R &&rhs) {
  assert(lhs.is_valid() && rhs.is_valid());
  if (lhs == rhs)
    return get_scalar_zero();
  auto a = detail::scalar_comparison_try_num(lhs);
  auto b = detail::scalar_comparison_try_num(rhs);
  if (a && b)
    return (*a < *b) ? get_scalar_one() : get_scalar_zero();
  return make_expression<scalar_lt>(std::forward<L>(lhs), std::forward<R>(rhs));
}

template <scalar_expr_holder L, scalar_expr_holder R>
[[nodiscard]] auto gt(L &&lhs, R &&rhs) {
  assert(lhs.is_valid() && rhs.is_valid());
  if (lhs == rhs)
    return get_scalar_zero();
  auto a = detail::scalar_comparison_try_num(lhs);
  auto b = detail::scalar_comparison_try_num(rhs);
  if (a && b)
    return (*b < *a) ? get_scalar_one() : get_scalar_zero();
  return make_expression<scalar_gt>(std::forward<L>(lhs), std::forward<R>(rhs));
}

template <scalar_expr_holder L, scalar_expr_holder R>
[[nodiscard]] auto le(L &&lhs, R &&rhs) {
  assert(lhs.is_valid() && rhs.is_valid());
  if (lhs == rhs)
    return get_scalar_one();
  auto a = detail::scalar_comparison_try_num(lhs);
  auto b = detail::scalar_comparison_try_num(rhs);
  if (a && b)
    return (*b < *a) ? get_scalar_zero() : get_scalar_one();
  return make_expression<scalar_le>(std::forward<L>(lhs), std::forward<R>(rhs));
}

template <scalar_expr_holder L, scalar_expr_holder R>
[[nodiscard]] auto ge(L &&lhs, R &&rhs) {
  assert(lhs.is_valid() && rhs.is_valid());
  if (lhs == rhs)
    return get_scalar_one();
  auto a = detail::scalar_comparison_try_num(lhs);
  auto b = detail::scalar_comparison_try_num(rhs);
  if (a && b)
    return (*a < *b) ? get_scalar_zero() : get_scalar_one();
  return make_expression<scalar_ge>(std::forward<L>(lhs), std::forward<R>(rhs));
}

template <scalar_expr_holder L, scalar_expr_holder R>
[[nodiscard]] auto eq(L &&lhs, R &&rhs) {
  assert(lhs.is_valid() && rhs.is_valid());
  if (lhs == rhs)
    return get_scalar_one();
  auto a = detail::scalar_comparison_try_num(lhs);
  auto b = detail::scalar_comparison_try_num(rhs);
  if (a && b)
    return (*a == *b) ? get_scalar_one() : get_scalar_zero();
  return make_expression<scalar_eq>(std::forward<L>(lhs), std::forward<R>(rhs));
}

template <scalar_expr_holder L, scalar_expr_holder R>
[[nodiscard]] auto ne(L &&lhs, R &&rhs) {
  assert(lhs.is_valid() && rhs.is_valid());
  if (lhs == rhs)
    return get_scalar_zero();
  auto a = detail::scalar_comparison_try_num(lhs);
  auto b = detail::scalar_comparison_try_num(rhs);
  if (a && b)
    return (*a == *b) ? get_scalar_zero() : get_scalar_one();
  return make_expression<scalar_ne>(std::forward<L>(lhs), std::forward<R>(rhs));
}

} // namespace numsim::cas

#endif // SCALAR_STD_H
