#ifndef TENSOR_TO_SCALAR_POSITIVITY_PROPAGATION_H
#define TENSOR_TO_SCALAR_POSITIVITY_PROPAGATION_H

#include <cassert>

#include <numsim_cas/core/assumptions.h>
#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_domain_traits.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_scalar_wrapper.h>

// #260 — positivity-tag propagation for t2s operators (mul, neg, pow).
//
// Each operator captures a `view` of its operand assumptions BEFORE
// forwarding into the simplifier (which may consume the holder as an
// rvalue), then applies the matching propagation rule to the result
// holder. Joint insertions mirror scalar_assume.h's pattern: a
// `positive` insertion also writes nonnegative + nonzero + real_tag.
// set_inferred() marks them as established facts (forward-compat with
// the planned assumption propagator).
//
// ── Aliasing contract ───────────────────────────────────────────
//
// `result` may alias one of the operand holders. The simplifier is
// allowed to fold (e.g. `x * 1 → x`) and return the operand's own
// holder; in that case `result.data()` is the operand's node and
// inserting into `result.data()->assumptions()` mutates the operand.
//
// All rules here are designed so this mutation is sound — they only
// fire when the inferred tags are already implied by the operand's
// state. e.g. `mark_positive(x)` after `propagate_mul(x_pos, 1_pos)`
// when the fold returned x is safe because x was already positive.
//
// If you add a new rule, preserve this invariant: a rule that fires
// on operand views V_lhs, V_rhs must produce a result tag that is a
// logical consequence of V_lhs ∧ V_rhs — never strictly stronger than
// any single operand's existing state.
//
// ── Robustness to direct-manager callers ────────────────────────
//
// Reading code that mixed `pos·nonneg → nonneg` is implemented as
// `(pos || nonneg) && (pos || nonneg) → nonneg`. This handles the
// case where a caller used the lower-level
// `manager.insert(positive{})` without going through the joint-
// insertion helpers — `positive{}` is then present without
// `nonnegative{}`. Same shape for the pow "exponent is real" guard:
// `real_tag || any sign tag || integer || rational || try_numeric()`.

namespace numsim::cas::tensor_to_scalar_detail::positivity {

// Single struct holds all the assumption bits we read off an operand.
//
// NOTE: these are tag-state predicates, not truth predicates. A "true"
// for `positive` means the manager contains `positive{}`, which IS the
// closest tractable proxy for "the value is positive" given how the
// codebase represents assumptions today — but a caller who mutates the
// manager into an incoherent state (e.g. inserts both `positive{}` and
// `nonpositive{}`) would defeat any inference here.
struct view {
  bool positive;
  bool nonnegative;
  bool nonpositive;
  bool negative;
  bool nonzero;
  bool real; // real_tag OR integer/rational/irrational OR known numeric

  // "At least nonneg" via TAG state: the operand carries either
  // `positive{}` or `nonnegative{}`. Robust to direct-manager callers
  // who set `positive{}` without the joint `nonnegative{}`. The "tag"
  // suffix is intentional — distinguishes from the truth claim.
  bool at_least_nonneg_tag() const { return positive || nonnegative; }
  bool at_least_nonpos_tag() const { return negative || nonpositive; }
};

// Read sign tags from the t2s expression's assumption manager. If the
// expression is a `tensor_to_scalar_scalar_wrapper` (the bridge from
// the scalar domain into t2s), ALSO read the wrapped scalar's tags —
// the wrapper's own manager is fresh at construction and doesn't
// inherit from the scalar's. Forwarding is single-level: we don't
// recurse through nested wrappers (the wrapper today wraps a
// scalar_expression, never another t2s, so single-level is exhaustive).
//
// `real_tag` is treated permissively: real_tag itself, any sign tag
// (which joint-inserts real_tag in the normal path), the discrete-
// number tags (integer, rational), and concrete numeric constants
// (try_numeric success) all count as "real". This avoids waiting on
// #261 (constants pre-annotation) for the common `pow(x, integer)`
// case.
inline view read(expression_holder<tensor_to_scalar_expression> const &e) {
  // Defense-in-depth: an invalid holder would null-deref through
  // numeric_assumption_manager::contains. The
  // tensor_to_scalar_differentiation.cpp accumulator fix
  // (`tensor_to_scalar_add` now uses the explicit `is_valid()`
  // check pattern instead of relying on `expression_holder::
  // operator+=`'s invalid-lhs safety net) eliminates the only
  // known source. Keeping the guard belt-and-suspenders for any
  // future visitor that hasn't adopted the pattern.
  if (!e.is_valid())
    return {};
  auto const &a = e.data()->assumptions();
  view v{a.contains(numsim::cas::positive{}),
         a.contains(numsim::cas::nonnegative{}),
         a.contains(numsim::cas::nonpositive{}),
         a.contains(numsim::cas::negative{}),
         a.contains(numsim::cas::nonzero{}),
         a.contains(numsim::cas::real_tag{}) ||
             a.contains(numsim::cas::integer{}) ||
             a.contains(numsim::cas::rational{}) ||
             a.contains(numsim::cas::irrational{})};
  if (is_same<tensor_to_scalar_scalar_wrapper>(e)) {
    auto const &inner =
        e.template get<tensor_to_scalar_scalar_wrapper>().expr();
    auto const &ia = inner.data()->assumptions();
    v.positive = v.positive || ia.contains(numsim::cas::positive{});
    v.nonnegative = v.nonnegative || ia.contains(numsim::cas::nonnegative{});
    v.nonpositive = v.nonpositive || ia.contains(numsim::cas::nonpositive{});
    v.negative = v.negative || ia.contains(numsim::cas::negative{});
    v.nonzero = v.nonzero || ia.contains(numsim::cas::nonzero{});
    v.real = v.real || ia.contains(numsim::cas::real_tag{}) ||
             ia.contains(numsim::cas::integer{}) ||
             ia.contains(numsim::cas::rational{}) ||
             ia.contains(numsim::cas::irrational{});
  }
  // Concrete numeric constants are real by construction — they evaluate
  // to a `scalar_number` (double under the hood today; no complex path
  // in t2s). Treating try_numeric() success as real_tag avoids waiting
  // on #261 (pre-annotating tensor_to_scalar_one/zero/scalar_constant)
  // for the common `pow(x, integer)` exponent case.
  if (!v.real) {
    using traits = numsim::cas::domain_traits<tensor_to_scalar_expression>;
    if (traits::try_numeric(e))
      v.real = true;
  }
  return v;
}

// Defense-in-depth assertion macro. Catches a rule that would insert
// a tag contradicting the operand's existing state — e.g. a
// hypothetical incorrect `(neg, neg) → pos` rule firing on a manager
// that already contains `negative{}`. Aliasing makes this critical:
// if the result holder shares its node with an operand (via a fold
// returning the operand), the insertion mutates the operand, and a
// wrong rule corrupts the operand's tags.
//
// The check is a debug-only `assert` (compiled out under NDEBUG) —
// rules in this header are intended to be statically correct, so the
// assertions are for future contributors who add rules.
#ifndef NDEBUG
#define NUMSIM_CAS_PROP_ASSERT_NO_CONTRADICTION(manager, banned_tag)           \
  do {                                                                         \
    assert(!(manager).contains(banned_tag) &&                                  \
           "positivity_propagation: rule would set a tag contradicting "       \
           "the operand's existing state");                                    \
  } while (0)
#else
#define NUMSIM_CAS_PROP_ASSERT_NO_CONTRADICTION(manager, banned_tag) ((void)0)
#endif

inline void
mark_positive(expression_holder<tensor_to_scalar_expression> const &e) {
  auto &a = e.data()->assumptions();
  NUMSIM_CAS_PROP_ASSERT_NO_CONTRADICTION(a, numsim::cas::negative{});
  NUMSIM_CAS_PROP_ASSERT_NO_CONTRADICTION(a, numsim::cas::nonpositive{});
  a.insert(numsim::cas::positive{});
  a.insert(numsim::cas::nonnegative{});
  a.insert(numsim::cas::nonzero{});
  a.insert(numsim::cas::real_tag{});
  a.set_inferred();
}
inline void
mark_negative(expression_holder<tensor_to_scalar_expression> const &e) {
  auto &a = e.data()->assumptions();
  NUMSIM_CAS_PROP_ASSERT_NO_CONTRADICTION(a, numsim::cas::positive{});
  NUMSIM_CAS_PROP_ASSERT_NO_CONTRADICTION(a, numsim::cas::nonnegative{});
  a.insert(numsim::cas::negative{});
  a.insert(numsim::cas::nonpositive{});
  a.insert(numsim::cas::nonzero{});
  a.insert(numsim::cas::real_tag{});
  a.set_inferred();
}
inline void
mark_nonnegative(expression_holder<tensor_to_scalar_expression> const &e) {
  auto &a = e.data()->assumptions();
  NUMSIM_CAS_PROP_ASSERT_NO_CONTRADICTION(a, numsim::cas::negative{});
  a.insert(numsim::cas::nonnegative{});
  a.insert(numsim::cas::real_tag{});
  a.set_inferred();
}
inline void
mark_nonpositive(expression_holder<tensor_to_scalar_expression> const &e) {
  auto &a = e.data()->assumptions();
  NUMSIM_CAS_PROP_ASSERT_NO_CONTRADICTION(a, numsim::cas::positive{});
  a.insert(numsim::cas::nonpositive{});
  a.insert(numsim::cas::real_tag{});
  a.set_inferred();
}

// Mul: pos·pos → pos; (≥nonneg)·(≥nonneg) → nonneg.
//
// Sign-aware rules (neg·neg → pos, pos·neg → neg) are out of scope
// per #260's "narrow scope here to mul/div/pow/neg first".
//
// ORDER MATTERS: stronger before weaker. The `pos·pos → pos` branch
// must run first; swapping with the `at_least_nonneg_tag` branch
// would collapse `pos·pos` to `nonneg` (still correct, but weaker
// than necessary — and `nonzero` would be lost).
inline void propagate_mul_from_views(
    view lhs, view rhs,
    expression_holder<tensor_to_scalar_expression> const &result) {
  if (lhs.positive && rhs.positive) {
    mark_positive(result);
  } else if (lhs.at_least_nonneg_tag() && rhs.at_least_nonneg_tag()) {
    mark_nonnegative(result);
  }
}

// Neg: flip sign, preserve magnitude class.
//
// REACHABILITY NOTE: only the `positive → negative` and
// `at_least_nonneg_tag → nonpositive` branches are reachable from the
// current call graph. The `-(-x) → x` fold in the t2s_negative
// factory short-circuits before this helper runs, so an operand
// carrying `negative{}` or `nonpositive{}` can't reach here — those
// tags are only ever produced by THIS helper's own `mark_negative` /
// `mark_nonpositive`, and the fold strips them.
//
// The `negative → positive` and `at_least_nonpos_tag → nonnegative`
// branches are kept for future-symmetry. If/when a sign-aware op
// (out of scope per #260) produces a `negative{}`-tagged result via
// a non-tensor_negative node (e.g. `mul(positive, negative_const)`
// after sign-aware mul lands), those branches start firing and the
// neg propagation stays correct without further changes.
inline void propagate_neg_from_view(
    view operand,
    expression_holder<tensor_to_scalar_expression> const &result) {
  if (operand.positive) {
    mark_negative(result);
  } else if (operand.at_least_nonneg_tag()) {
    mark_nonpositive(result);
  } else if (operand.negative) {
    mark_positive(result);
  } else if (operand.at_least_nonpos_tag()) {
    mark_nonnegative(result);
  }
}

// Pow: pow(pos, real) → pos; pow(≥nonneg, ≥nonneg) → nonneg.
//
// The "real exponent" guard prevents a complex exponent from silently
// inheriting positivity. The strict pow(neg, even-int) → pos rule is
// deferred (#260 scope-out).
//
// ORDER MATTERS (same shape as mul): pos+real first, then the
// broader nonneg case.
inline void propagate_pow_from_views(
    view base, view exponent,
    expression_holder<tensor_to_scalar_expression> const &result) {
  if (base.positive && exponent.real) {
    mark_positive(result);
    return;
  }
  if (base.at_least_nonneg_tag() && exponent.at_least_nonneg_tag()) {
    mark_nonnegative(result);
  }
}

} // namespace numsim::cas::tensor_to_scalar_detail::positivity

#endif // TENSOR_TO_SCALAR_POSITIVITY_PROPAGATION_H
