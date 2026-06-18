#ifndef NUMSIM_CAS_CORE_POSITIVITY_PROPAGATION_H
#define NUMSIM_CAS_CORE_POSITIVITY_PROPAGATION_H

#include <cassert>

#include <numsim_cas/core/assumptions.h>
#include <numsim_cas/core/expression_holder.h>

// Domain-agnostic positivity-tag propagation for arithmetic operators
// (mul, neg, pow). #260 (t2s) and #305 (scalar) both built parallel
// helpers; this header lifts everything except the domain-specific
// `read(expr)` function (which inspects domain-specific node types
// like scalar_constant or tensor_to_scalar_scalar_wrapper).
//
// Each domain header in the parent dirs supplies its own `read()`
// returning a `view`, then delegates the rule machinery here.
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
// state. e.g. `mark_positive(x)` after a fold returning `x` is safe
// because x was already positive in the rule's precondition.
//
// If you add a new rule, preserve this invariant: a rule that fires
// on operand views V_lhs, V_rhs must produce a result tag that is a
// logical consequence of V_lhs ∧ V_rhs — never strictly stronger
// than what either operand's state implies.
//
// ── Robustness to direct-manager callers ────────────────────────
//
// `at_least_nonneg_tag()` is `positive || nonnegative`. A direct-
// manager caller who inserts `positive{}` alone (without the joint
// `nonnegative{}` insertion done by the assume_* helpers) still gets
// the right answer — the rule sees `positive` and concludes
// nonneg-ness implicitly.

namespace numsim::cas::positivity {

// Tag-state view of an expression's assumption manager. NOTE: these
// are tag-state predicates, not truth predicates. A caller who
// mutates the manager into an incoherent state (e.g. inserts both
// `positive{}` and `nonpositive{}`) defeats any inference here.
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
  // suffix distinguishes from a truth claim.
  bool at_least_nonneg_tag() const { return positive || nonnegative; }
  bool at_least_nonpos_tag() const { return negative || nonpositive; }
};

// Defense-in-depth: a rule that inserts a tag contradicting the
// operand's existing state would be incorrect under aliasing
// (mutating an operand we shouldn't). Debug-only — compiled out
// under NDEBUG.
#ifndef NDEBUG
#define NUMSIM_CAS_POSITIVITY_ASSERT_NO_CONTRADICTION(manager, banned_tag)     \
  do {                                                                         \
    assert(!(manager).contains(banned_tag) &&                                  \
           "positivity_propagation: rule would set a tag contradicting "       \
           "the operand's existing state");                                    \
  } while (0)
#else
#define NUMSIM_CAS_POSITIVITY_ASSERT_NO_CONTRADICTION(manager, banned_tag)     \
  ((void)0)
#endif

// Joint-insertion helpers, mirror scalar_assume.h's pattern.
// Templated on the expression type so the same helpers work across
// scalar, t2s, and any future domain whose expression base supplies
// `assumptions()`.
template <typename Expr>
inline void mark_positive(expression_holder<Expr> const &e) {
  auto &a = e.data()->assumptions();
  NUMSIM_CAS_POSITIVITY_ASSERT_NO_CONTRADICTION(a, numsim::cas::negative{});
  NUMSIM_CAS_POSITIVITY_ASSERT_NO_CONTRADICTION(a, numsim::cas::nonpositive{});
  a.insert(numsim::cas::positive{});
  a.insert(numsim::cas::nonnegative{});
  a.insert(numsim::cas::nonzero{});
  a.insert(numsim::cas::real_tag{});
  a.set_inferred();
}

template <typename Expr>
inline void mark_negative(expression_holder<Expr> const &e) {
  auto &a = e.data()->assumptions();
  NUMSIM_CAS_POSITIVITY_ASSERT_NO_CONTRADICTION(a, numsim::cas::positive{});
  NUMSIM_CAS_POSITIVITY_ASSERT_NO_CONTRADICTION(a, numsim::cas::nonnegative{});
  a.insert(numsim::cas::negative{});
  a.insert(numsim::cas::nonpositive{});
  a.insert(numsim::cas::nonzero{});
  a.insert(numsim::cas::real_tag{});
  a.set_inferred();
}

template <typename Expr>
inline void mark_nonnegative(expression_holder<Expr> const &e) {
  auto &a = e.data()->assumptions();
  NUMSIM_CAS_POSITIVITY_ASSERT_NO_CONTRADICTION(a, numsim::cas::negative{});
  a.insert(numsim::cas::nonnegative{});
  a.insert(numsim::cas::real_tag{});
  a.set_inferred();
}

template <typename Expr>
inline void mark_nonpositive(expression_holder<Expr> const &e) {
  auto &a = e.data()->assumptions();
  NUMSIM_CAS_POSITIVITY_ASSERT_NO_CONTRADICTION(a, numsim::cas::positive{});
  a.insert(numsim::cas::nonpositive{});
  a.insert(numsim::cas::real_tag{});
  a.set_inferred();
}

// Mul: pos·pos → pos; (≥nonneg)·(≥nonneg) → nonneg.
// ORDER MATTERS: stronger before weaker — swapping would collapse
// pos·pos to nonneg and lose nonzero{}.
template <typename Expr>
inline void propagate_mul_from_views(view lhs, view rhs,
                                     expression_holder<Expr> const &result) {
  if (lhs.positive && rhs.positive) {
    mark_positive(result);
  } else if (lhs.at_least_nonneg_tag() && rhs.at_least_nonneg_tag()) {
    mark_nonnegative(result);
  }
}

// Neg: flip sign, preserve magnitude class.
//
// REACHABILITY NOTE: only the `positive → negative` and
// `at_least_nonneg_tag → nonpositive` branches are reachable from
// the current call graph. The `-(-x) → x` fold in each domain's
// negative factory short-circuits before this helper runs, so an
// operand carrying `negative{}` or `nonpositive{}` can't reach here
// — those tags are only ever produced by THIS helper's own
// `mark_negative` / `mark_nonpositive`, and the fold strips them.
//
// The `negative → positive` and `at_least_nonpos_tag → nonnegative`
// branches are kept for future-symmetry. If/when a sign-aware op
// (out of scope per #260) produces a `negative{}`-tagged result via
// a non-negative-node (e.g. `mul(positive, negative_const)` after
// sign-aware mul lands per #306), those branches start firing and
// the neg propagation stays correct without further changes.
template <typename Expr>
inline void propagate_neg_from_view(view operand,
                                    expression_holder<Expr> const &result) {
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
// The "real exponent" guard prevents a complex exponent from
// silently inheriting positivity. The strict pow(neg, even-int) →
// pos rule is deferred (#260 scope-out, tracked in #306).
template <typename Expr>
inline void propagate_pow_from_views(view base, view exponent,
                                     expression_holder<Expr> const &result) {
  if (base.positive && exponent.real) {
    mark_positive(result);
    return;
  }
  if (base.at_least_nonneg_tag() && exponent.at_least_nonneg_tag()) {
    mark_nonnegative(result);
  }
}

} // namespace numsim::cas::positivity

#endif // NUMSIM_CAS_CORE_POSITIVITY_PROPAGATION_H
