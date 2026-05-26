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
#include <numsim_cas/scalar/scalar_max.h>
#include <numsim_cas/scalar/scalar_min.h>
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
// scalar_zero, scalar_one, scalar_constant, or a (possibly nested)
// negation of any of those. Returns nullopt otherwise. Shared by
// pow() constant folding and by the six comparison free functions
// below. Recursion through negation defends against the rare path
// where `make_expression<scalar_negative>` is called twice without
// going through `operator-` (which would have collapsed `-(-x)`).
inline std::optional<scalar_number>
try_extract_scalar_number(expression_holder<scalar_expression> const &e) {
  if (is_same<scalar_zero>(e))
    return scalar_number{0};
  if (is_same<scalar_one>(e))
    return scalar_number{1};
  if (is_same<scalar_constant>(e))
    return e.template get<scalar_constant>().value();
  if (is_same<scalar_negative>(e)) {
    auto inner =
        try_extract_scalar_number(e.template get<scalar_negative>().expr());
    if (inner)
      return -*inner;
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

// Sign cone collected from the assumption tags on an expression.
// The four flags are not mutually exclusive: a known-zero expression
// has both `nonneg` and `nonpos`; a known-strict-positive has both
// `pos` and `nonneg`. Half-plane meanings:
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

// Build a `sign_cone` with a single `infer_assumptions` call.
// `is_positive(e)` etc. each infer-then-query, so calling all four
// would re-enter the inferred-flag check four times. Here we do it
// once and read the resulting assumption set directly.
inline sign_cone make_sign_cone(expression_holder<scalar_expression> const &e) {
  infer_assumptions(e);
  auto const &a = e.data()->assumptions();
  return {a.contains(positive{}), a.contains(negative{}),
          a.contains(nonnegative{}), a.contains(nonpositive{})};
}

// `lt(a, b) = a < b` — the canonical sign-cone analysis. All other
// op sign hooks are expressed by mirroring this one (swap-arguments
// for gt/ge/le, equality for eq/ne) further down.
//
//   a ≥ 0 AND b ≤ 0  →  a ≥ 0 ≥ b  →  a < b impossible       → fold_0
//   a < 0 AND b ≥ 0  →  a < 0 ≤ b                            → fold_1
//   a ≤ 0 AND b > 0  →  a ≤ 0 < b                            → fold_1
inline comp_reduce lt_signs(sign_cone const &a, sign_cone const &b) {
  if (a.nonneg && b.nonpos)
    return comp_reduce::fold_zero;
  if ((a.neg && b.nonneg) || (a.nonpos && b.pos))
    return comp_reduce::fold_one;
  return comp_reduce::no_fold;
}

// eq(a, b) folds when the sign cones force the values to differ.
// One side strictly outside an inclusive interval containing the
// other is enough.
inline bool eq_signs_force_unequal(sign_cone const &a, sign_cone const &b) {
  return (a.pos && b.nonpos) || (a.nonneg && b.neg) || (a.nonpos && b.pos) ||
         (a.neg && b.nonneg);
}

inline comp_reduce flip_fold(comp_reduce r) {
  switch (r) {
  case comp_reduce::fold_one:
    return comp_reduce::fold_zero;
  case comp_reduce::fold_zero:
    return comp_reduce::fold_one;
  case comp_reduce::no_fold:
    return comp_reduce::no_fold;
  }
  return comp_reduce::no_fold;
}

// Generic comparison constructor. Op carries:
//   * `node_type` — the node to construct on no-fold;
//   * `identity_holds` — what `cmp(x, x)` should fold to;
//   * `from_numeric(a, b)` → bool — numeric meaning;
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

// ─── Six op strategies, derived from `lt` and `eq` ──────────────────
// `gt(a,b) = lt(b,a)`. `le(a,b) = !lt(b,a)` (wait — actually
// `le(a,b) = a ≤ b = !(a > b) = !lt(b,a)`. So le inverts gt's truth).
// `ge(a,b) = !lt(a,b)`. `ne` inverts `eq`.

struct lt_op {
  using node_type = scalar_lt;
  static constexpr bool identity_holds = false;
  static bool from_numeric(scalar_number const &a, scalar_number const &b) {
    return numeric_less(a, b);
  }
  static comp_reduce from_signs(sign_cone const &a, sign_cone const &b) {
    return lt_signs(a, b);
  }
};

struct gt_op {
  using node_type = scalar_gt;
  static constexpr bool identity_holds = false;
  static bool from_numeric(scalar_number const &a, scalar_number const &b) {
    return lt_op::from_numeric(b, a);
  }
  static comp_reduce from_signs(sign_cone const &a, sign_cone const &b) {
    return lt_signs(b, a);
  }
};

struct le_op {
  using node_type = scalar_le;
  static constexpr bool identity_holds = true;
  static bool from_numeric(scalar_number const &a, scalar_number const &b) {
    return !gt_op::from_numeric(a, b);
  }
  static comp_reduce from_signs(sign_cone const &a, sign_cone const &b) {
    return flip_fold(lt_signs(b, a)); // !gt
  }
};

struct ge_op {
  using node_type = scalar_ge;
  static constexpr bool identity_holds = true;
  static bool from_numeric(scalar_number const &a, scalar_number const &b) {
    return !lt_op::from_numeric(a, b);
  }
  static comp_reduce from_signs(sign_cone const &a, sign_cone const &b) {
    return flip_fold(lt_signs(a, b)); // !lt
  }
};

struct eq_op {
  using node_type = scalar_eq;
  static constexpr bool identity_holds = true;
  static bool from_numeric(scalar_number const &a, scalar_number const &b) {
    return a == b;
  }
  static comp_reduce from_signs(sign_cone const &a, sign_cone const &b) {
    return eq_signs_force_unequal(a, b) ? comp_reduce::fold_zero
                                        : comp_reduce::no_fold;
  }
};

struct ne_op {
  using node_type = scalar_ne;
  static constexpr bool identity_holds = false;
  static bool from_numeric(scalar_number const &a, scalar_number const &b) {
    return !eq_op::from_numeric(a, b);
  }
  static comp_reduce from_signs(sign_cone const &a, sign_cone const &b) {
    return eq_signs_force_unequal(a, b) ? comp_reduce::fold_one
                                        : comp_reduce::no_fold;
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

// ─── Min / max (#137) ────────────────────────────────────────────────
// max(a, b) and min(a, b) with construction-time folds:
//   max(x, x) → x                       (idempotent on equal operands)
//   max(numeric, numeric) → numeric     (constant folding)
//   min(x, x) → x
//   min(numeric, numeric) → numeric
// The constant-fold path reuses `try_extract_scalar_number` from the pow
// helper above — same pattern as pow's numeric folding. No deeper folds
// like max(0, max(x, 0)) → max(x, 0) yet; those go in a future
// simplifier pass.

template <scalar_expr_holder L, scalar_expr_holder R>
[[nodiscard]] auto max(L &&lhs, R &&rhs) {
  assert(lhs.is_valid());
  assert(rhs.is_valid());
  // max(x, x) → x
  if (lhs.get().hash_value() == rhs.get().hash_value())
    return std::forward<L>(lhs);
  // Constant folding
  auto lval = detail::try_extract_scalar_number(lhs);
  auto rval = detail::try_extract_scalar_number(rhs);
  if (lval && rval) {
    if (numeric_less(*lval, *rval))
      return std::forward<R>(rhs);
    return std::forward<L>(lhs);
  }
  // Canonical order: sort by hash so max(x, y) and max(y, x) build the
  // same node. Mathematically max is commutative; without this
  // canonicalisation the two would have different structural hashes
  // (verified during #137 review) and downstream rules like
  // "max(a, b) + max(b, a) → 2*max(a, b)" would never fire.
  if (lhs.get().hash_value() > rhs.get().hash_value())
    return make_expression<scalar_max>(std::forward<R>(rhs),
                                       std::forward<L>(lhs));
  return make_expression<scalar_max>(std::forward<L>(lhs),
                                     std::forward<R>(rhs));
}

template <scalar_expr_holder L, scalar_expr_holder R>
[[nodiscard]] auto min(L &&lhs, R &&rhs) {
  assert(lhs.is_valid());
  assert(rhs.is_valid());
  // min(x, x) → x
  if (lhs.get().hash_value() == rhs.get().hash_value())
    return std::forward<L>(lhs);
  // Constant folding
  auto lval = detail::try_extract_scalar_number(lhs);
  auto rval = detail::try_extract_scalar_number(rhs);
  if (lval && rval) {
    if (numeric_less(*lval, *rval))
      return std::forward<L>(lhs);
    return std::forward<R>(rhs);
  }
  // Canonical order — see max() above for rationale.
  if (lhs.get().hash_value() > rhs.get().hash_value())
    return make_expression<scalar_min>(std::forward<R>(rhs),
                                       std::forward<L>(lhs));
  return make_expression<scalar_min>(std::forward<L>(lhs),
                                     std::forward<R>(rhs));
}

template <scalar_expr_holder E> [[nodiscard]] auto log10(E &&e) {
  expression_holder<scalar_expression> ten =
      make_expression<scalar_constant>(scalar_number{10});
  expression_holder<scalar_expression> log_ten =
      make_expression<scalar_log>(std::move(ten));
  return log(std::forward<E>(e)) / std::move(log_ten);
}

template <scalar_expr_holder E> [[nodiscard]] auto sinh(E const &e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  expression_holder<scalar_expression> two =
      make_expression<scalar_constant>(scalar_number{2});
  return (exp(e) - exp(-e)) / std::move(two);
}

template <scalar_expr_holder E> [[nodiscard]] auto cosh(E const &e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_one();
  // cosh(-x) = cosh(x) — fold the negation away so chain rules see the
  // simpler form.
  if (is_same<scalar_negative>(e))
    return cosh(e.template get<scalar_negative>().expr());
  expression_holder<scalar_expression> two =
      make_expression<scalar_constant>(scalar_number{2});
  return (exp(e) + exp(-e)) / std::move(two);
}

template <scalar_expr_holder E> [[nodiscard]] auto tanh(E const &e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  // tanh(-x) = -tanh(x)
  if (is_same<scalar_negative>(e))
    return -tanh(e.template get<scalar_negative>().expr());
  // tanh(x) = (exp(2x) - 1) / (exp(2x) + 1). The naive sinh(e)/cosh(e)
  // form expands to a sum/difference of exp(x) and exp(-x), and the
  // quotient-rule differentiation of that quotient tries to insert the
  // shared exp(x) child twice into the same n_ary_add — tripping
  // n_ary_tree::insert_hash's duplicate guard (#180). The alt form has
  // only one exp() term so diff() doesn't fan out into colliding adds.
  // The two forms are algebraically identical:
  //   (e^x - e^{-x}) / (e^x + e^{-x}) = (e^{2x} - 1) / (e^{2x} + 1).
  // Closes #180.
  expression_holder<scalar_expression> one = get_scalar_one();
  expression_holder<scalar_expression> two =
      make_expression<scalar_constant>(scalar_number{2});
  auto e2x = exp(std::move(two) * e);
  // Bind numerator and denominator first; C++'s unspecified evaluation
  // order for `/` arguments combined with `std::move(e2x)` in one branch
  // could move `e2x` before the other branch uses it (mirror of the
  // atanh evaluation-order fix in this same file).
  auto num = e2x - one;
  auto den = std::move(e2x) + std::move(one);
  return std::move(num) / std::move(den);
}

template <scalar_expr_holder E> [[nodiscard]] auto asinh(E const &e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  expression_holder<scalar_expression> one = get_scalar_one();
  return log(e + sqrt(pow(e, 2) + std::move(one)));
}

template <scalar_expr_holder E> [[nodiscard]] auto acosh(E const &e) {
  // acosh(1) = 0 — covers scalar_one and scalar_constant{1}.
  if (is_same<scalar_one>(e) ||
      (is_same<scalar_constant>(e) &&
       e.template get<scalar_constant>().value() == scalar_number{1}))
    return get_scalar_zero();
  expression_holder<scalar_expression> one = get_scalar_one();
  return log(e + sqrt(pow(e, 2) - std::move(one)));
}

template <scalar_expr_holder E> [[nodiscard]] auto atanh(E const &e) {
  if (is_same<scalar_zero>(e))
    return get_scalar_zero();
  // The C++ standard does not specify evaluation order of operator/'s
  // operands. Build numerator and denominator as separate named statements
  // so the moves are unambiguously ordered before the division consumes
  // them. (gcc/MSVC evaluate RHS first; clang evaluates LHS first.)
  auto num = get_scalar_one() + e;
  auto den = get_scalar_one() - e;
  expression_holder<scalar_expression> two =
      make_expression<scalar_constant>(scalar_number{2});
  return log(std::move(num) / std::move(den)) / std::move(two);
}

// ─── Macauley bracket / ramp / Heaviside (#138) ──────────────────────
// Constitutive-modelling primitives, all composed from the existing
// (max/min/comparison/sqrt/pow) primitives — no dedicated AST nodes,
// mirroring the hyperbolic-functions approach from PR #123.

/**
 * @brief Macauley positive part: `<e>+ = max(e, 0)`.
 *
 * Standard in damage activation, ramp yield surfaces, contact pressure.
 * Returns 0 when e ≤ 0 and e otherwise. Differentiation goes through
 * the underlying `max()` and throws until `if_then_else` (#135) lands;
 * see `scalar_max`'s diff overload.
 *
 * Construction-time fold: `macauley_plus(-e) → macauley_minus(e)`
 * (factored through `scalar_negative` detection).
 */
template <scalar_expr_holder E> [[nodiscard]] auto macauley_plus(E &&e) {
  // <-x>+ = max(-x, 0) = -min(x, 0) = <x>-. Pull negations out so
  // chain rules see the simpler ramp pair.
  if (is_same<scalar_negative>(e))
    return -min(e.template get<scalar_negative>().expr(), get_scalar_zero());
  // <<x>+>+ = <x>+ — the positive part is non-negative by construction,
  // so applying the bracket again is a no-op. Detect the inner
  // macauley shape (a max with one operand being 0) directly because
  // the bracket has no dedicated AST node.
  if (is_same<scalar_max>(e)) {
    auto const &m = e.template get<scalar_max>();
    if (is_same<scalar_zero>(m.expr_lhs()) ||
        is_same<scalar_zero>(m.expr_rhs()))
      return std::forward<E>(e);
  }
  return max(std::forward<E>(e), get_scalar_zero());
}

/**
 * @brief Macauley negative part: `<e>- = -min(e, 0)`.
 *
 * Symmetric to `macauley_plus`. Used for plastic dissipation rate,
 * negative-stress contributions, asymmetric damage.
 */
template <scalar_expr_holder E> [[nodiscard]] auto macauley_minus(E &&e) {
  // <-x>- = -min(-x, 0) = max(x, 0) = <x>+.
  if (is_same<scalar_negative>(e))
    return max(e.template get<scalar_negative>().expr(), get_scalar_zero());
  return -min(std::forward<E>(e), get_scalar_zero());
}

/**
 * @brief Heaviside step function: `0 if e < 0, 1 otherwise`.
 *
 * Implemented via `ge(e, 0)` — the comparison node from #136 returns
 * 1.0 if its first operand is ≥ its second, 0.0 otherwise. This gives
 * the standard right-continuous step (H(0) = 1).
 *
 * The issue body originally suggested implementing this via
 * `if_then_else` (#135); using `ge` instead removes the #135
 * dependency and produces the same evaluation result. Once #135
 * lands, both routes simplify to indicator + comparison combinations
 * that the simplifier can collapse uniformly.
 */
template <scalar_expr_holder E> [[nodiscard]] auto heaviside(E &&e) {
  return ge(std::forward<E>(e), get_scalar_zero());
}

/**
 * @brief Smoothed Macauley positive part: `(e + sqrt(e² + ε²)) / 2`.
 *
 * `C^∞` regularisation of `<e>+ = max(e, 0)`. As `ε → 0` it recovers
 * the non-smooth Macauley bracket; for nonzero `ε` the second-order
 * derivative exists everywhere (useful for Newton-style solvers that
 * choke on the kink at `e = 0`).
 */
template <scalar_expr_holder E, scalar_expr_holder Eps>
[[nodiscard]] auto smoothed_macauley(E const &e, Eps const &eps) {
  expression_holder<scalar_expression> two =
      make_expression<scalar_constant>(scalar_number{2});
  return (e + sqrt(pow(e, 2) + pow(eps, 2))) / std::move(two);
}

} // namespace numsim::cas

#endif // SCALAR_STD_H
