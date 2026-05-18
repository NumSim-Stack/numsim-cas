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

namespace detail {
// Extract a scalar_number from `e` if it represents a literal value:
// scalar_zero, scalar_one, scalar_constant, or a negation of any of those.
// Returns nullopt otherwise. Shared by pow() constant folding and by the
// six comparison free functions below.
inline std::optional<scalar_number>
try_extract_scalar_number(expression_holder<scalar_expression> const &e) {
  if (is_same<scalar_zero>(e))
    return scalar_number{0};
  if (is_same<scalar_one>(e))
    return scalar_number{1};
  if (is_same<scalar_constant>(e))
    return e.template get<scalar_constant>().value();
  if (is_same<scalar_negative>(e)) {
    auto const &n = e.template get<scalar_negative>().expr();
    if (is_same<scalar_zero>(n))
      return scalar_number{0};
    if (is_same<scalar_one>(n))
      return -scalar_number{1};
    if (is_same<scalar_constant>(n))
      return -n.template get<scalar_constant>().value();
  }
  return std::nullopt;
}
} // namespace detail

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
    auto lhs_val = detail::try_extract_scalar_number(expr_lhs);
    auto rhs_val = detail::try_extract_scalar_number(expr_rhs);
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
//
// Each returns a Real *indicator* expression: 1.0 when the comparison
// holds, 0.0 otherwise. This lets `(eps > eps0) * sigma` work directly
// as the damage-activation idiom and gives if_then_else a single
// Real-typed condition.
//
// NB: `eq(a, b)` (CAS comparison node, value-based) is NOT the same as
// `a == b` (structural equality on expression_holder, used for hashing
// and sorting). They live in different namespaces of meaning.
//
// Construction-time simplifications applied in order:
//  1. structural identity → resolves immediately;
//  2. both sides extractable as scalar_number → numeric fold;
//  3. one side numeric + the other side's sign is known via
//     is_{positive,negative,nonnegative,nonpositive} → sign-based fold.
namespace detail {

// Strategy tags — values returned by the per-op sign hook.
enum class comp_reduce { fold_one, fold_zero, no_fold };

// Sign cone collected from the four `is_*` query helpers. The four
// flags are not mutually exclusive: e.g. a known-zero expression has
// both `nonneg` and `nonpos` set; a known-strict-positive has both
// `pos` and `nonneg`. The per-op hooks consume these as half-planes:
//   * pos ⇒ value > 0
//   * neg ⇒ value < 0
//   * nonneg ⇒ value ≥ 0
//   * nonpos ⇒ value ≤ 0
struct sign_cone {
  bool pos;
  bool neg;
  bool nonneg;
  bool nonpos;
};

inline sign_cone make_sign_cone(expression_holder<scalar_expression> const &e) {
  return {is_positive(e), is_negative(e), is_nonnegative(e), is_nonpositive(e)};
}

// Generic comparison constructor.
//
// Op carries:
//   * `node_type` — the node to construct on no-fold;
//   * `identity_holds` — what `cmp(x, x)` should fold to (true → one);
//   * `from_numeric(a, b)` → bool — the numeric meaning;
//   * `from_signs(lhs_cone, rhs_cone)` → comp_reduce.
template <typename Op, scalar_expr_holder L, scalar_expr_holder R>
[[nodiscard]] auto make_comparison(L &&lhs, R &&rhs, Op op) {
  assert(lhs.is_valid() && rhs.is_valid());

  if (lhs == rhs)
    return op.identity_holds ? get_scalar_one() : get_scalar_zero();

  auto na = try_extract_scalar_number(lhs);
  auto nb = try_extract_scalar_number(rhs);
  if (na && nb)
    return op.from_numeric(*na, *nb) ? get_scalar_one() : get_scalar_zero();

  switch (op.from_signs(make_sign_cone(lhs), make_sign_cone(rhs))) {
  case comp_reduce::fold_one:
    return get_scalar_one();
  case comp_reduce::fold_zero:
    return get_scalar_zero();
  case comp_reduce::no_fold:
    break;
  }

  return make_expression<typename Op::node_type>(std::forward<L>(lhs),
                                                 std::forward<R>(rhs));
}

// Sign-cone analysis for `lt(a, b) = a < b`.
//   a ≥ 0 AND b ≤ 0  →  a ≥ 0 ≥ b  →  a < b impossible       → fold_0
//   a < 0 AND b ≥ 0  →  a < 0 ≤ b                            → fold_1
//   a ≤ 0 AND b > 0  →  a ≤ 0 < b                            → fold_1
struct lt_op {
  using node_type = scalar_lt;
  static constexpr bool identity_holds = false;
  static bool from_numeric(scalar_number const &a, scalar_number const &b) {
    return numeric_less(a, b);
  }
  static comp_reduce from_signs(sign_cone const &a, sign_cone const &b) {
    if (a.nonneg && b.nonpos)
      return comp_reduce::fold_zero;
    if ((a.neg && b.nonneg) || (a.nonpos && b.pos))
      return comp_reduce::fold_one;
    return comp_reduce::no_fold;
  }
};

struct gt_op {
  using node_type = scalar_gt;
  static constexpr bool identity_holds = false;
  static bool from_numeric(scalar_number const &a, scalar_number const &b) {
    return numeric_less(b, a);
  }
  static comp_reduce from_signs(sign_cone const &a, sign_cone const &b) {
    if (a.nonpos && b.nonneg)
      return comp_reduce::fold_zero;
    if ((a.pos && b.nonpos) || (a.nonneg && b.neg))
      return comp_reduce::fold_one;
    return comp_reduce::no_fold;
  }
};

struct le_op {
  using node_type = scalar_le;
  static constexpr bool identity_holds = true;
  static bool from_numeric(scalar_number const &a, scalar_number const &b) {
    return !numeric_less(b, a);
  }
  static comp_reduce from_signs(sign_cone const &a, sign_cone const &b) {
    if (a.nonpos && b.nonneg)
      return comp_reduce::fold_one;
    if ((a.pos && b.nonpos) || (a.nonneg && b.neg))
      return comp_reduce::fold_zero;
    return comp_reduce::no_fold;
  }
};

struct ge_op {
  using node_type = scalar_ge;
  static constexpr bool identity_holds = true;
  static bool from_numeric(scalar_number const &a, scalar_number const &b) {
    return !numeric_less(a, b);
  }
  static comp_reduce from_signs(sign_cone const &a, sign_cone const &b) {
    if (a.nonneg && b.nonpos)
      return comp_reduce::fold_one;
    if ((a.neg && b.nonneg) || (a.nonpos && b.pos))
      return comp_reduce::fold_zero;
    return comp_reduce::no_fold;
  }
};

// eq(a, b) folds when the sign cones force the values to differ.
// One side strictly outside an inclusive interval containing the
// other is enough.
struct eq_op {
  using node_type = scalar_eq;
  static constexpr bool identity_holds = true;
  static bool from_numeric(scalar_number const &a, scalar_number const &b) {
    return a == b;
  }
  static comp_reduce from_signs(sign_cone const &a, sign_cone const &b) {
    bool forces_unequal = (a.pos && b.nonpos) || (a.nonneg && b.neg) ||
                          (a.nonpos && b.pos) || (a.neg && b.nonneg);
    return forces_unequal ? comp_reduce::fold_zero : comp_reduce::no_fold;
  }
};

struct ne_op {
  using node_type = scalar_ne;
  static constexpr bool identity_holds = false;
  static bool from_numeric(scalar_number const &a, scalar_number const &b) {
    return !(a == b);
  }
  static comp_reduce from_signs(sign_cone const &a, sign_cone const &b) {
    bool forces_unequal = (a.pos && b.nonpos) || (a.nonneg && b.neg) ||
                          (a.nonpos && b.pos) || (a.neg && b.nonneg);
    return forces_unequal ? comp_reduce::fold_one : comp_reduce::no_fold;
  }
};

} // namespace detail

template <scalar_expr_holder L, scalar_expr_holder R>
[[nodiscard]] auto lt(L &&lhs, R &&rhs) {
  return detail::make_comparison(std::forward<L>(lhs), std::forward<R>(rhs),
                                 detail::lt_op{});
}

template <scalar_expr_holder L, scalar_expr_holder R>
[[nodiscard]] auto gt(L &&lhs, R &&rhs) {
  return detail::make_comparison(std::forward<L>(lhs), std::forward<R>(rhs),
                                 detail::gt_op{});
}

template <scalar_expr_holder L, scalar_expr_holder R>
[[nodiscard]] auto le(L &&lhs, R &&rhs) {
  return detail::make_comparison(std::forward<L>(lhs), std::forward<R>(rhs),
                                 detail::le_op{});
}

template <scalar_expr_holder L, scalar_expr_holder R>
[[nodiscard]] auto ge(L &&lhs, R &&rhs) {
  return detail::make_comparison(std::forward<L>(lhs), std::forward<R>(rhs),
                                 detail::ge_op{});
}

template <scalar_expr_holder L, scalar_expr_holder R>
[[nodiscard]] auto eq(L &&lhs, R &&rhs) {
  return detail::make_comparison(std::forward<L>(lhs), std::forward<R>(rhs),
                                 detail::eq_op{});
}

template <scalar_expr_holder L, scalar_expr_holder R>
[[nodiscard]] auto ne(L &&lhs, R &&rhs) {
  return detail::make_comparison(std::forward<L>(lhs), std::forward<R>(rhs),
                                 detail::ne_op{});
}

} // namespace numsim::cas

#endif // SCALAR_STD_H
