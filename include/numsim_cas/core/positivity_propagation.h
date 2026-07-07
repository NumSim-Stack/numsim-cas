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
// returning a `numeric_assumption_manager` SNAPSHOT of the operand's
// sign tags (normalized so real_tag is materialized — see below), then
// delegates the rule machinery here.
//
// ── Why a snapshot (not a live manager reference) ────────────────
//
// The eager call sites read operand sign-tags BEFORE forwarding the
// operands into the simplifier, which consumes them as rvalues (it
// reuses refcount-1 temporaries). By the time `result` exists the
// operand holders may be moved-from, so we cannot read them then — the
// rule inputs must be captured up front. `read()` returns a value copy
// of the operand's `numeric_assumption_manager`, reusing the existing
// typed assumption tags instead of a bespoke bool mirror. The rules
// below query it with `.contains(positive{})` etc.
//
// `read()` also NORMALIZES the snapshot: it inserts real_tag whenever
// the operand is real-by-implication (integer/rational/irrational, or a
// numeric constant), so the rules only ever check real_tag. Complex
// constants are rejected at construction (scalar_constant.h), so every
// numeric constant reaching here is real.
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
// If you add a new rule, preserve this invariant: a rule that fires on
// operand snapshots S_lhs, S_rhs must produce a result tag that is a
// logical consequence of S_lhs ∧ S_rhs — never strictly stronger than
// what either operand's state implies.

namespace numsim::cas::positivity {

// "At least nonneg": the operand snapshot carries `positive` or
// `nonnegative`. Robust to direct-manager callers who set `positive`
// without the joint `nonnegative` insertion done by the assume_*
// helpers — the rule sees `positive` and concludes nonneg-ness.
inline bool at_least_nonneg(numeric_assumption_manager const &m) {
  return m.contains(numsim::cas::positive{}) ||
         m.contains(numsim::cas::nonnegative{});
}
inline bool at_least_nonpos(numeric_assumption_manager const &m) {
  return m.contains(numsim::cas::negative{}) ||
         m.contains(numsim::cas::nonpositive{});
}

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
inline void propagate_mul(numeric_assumption_manager const &lhs,
                          numeric_assumption_manager const &rhs,
                          expression_holder<Expr> const &result) {
  if (lhs.contains(numsim::cas::positive{}) &&
      rhs.contains(numsim::cas::positive{})) {
    mark_positive(result);
  } else if (at_least_nonneg(lhs) && at_least_nonneg(rhs)) {
    mark_nonnegative(result);
  }
}

// Neg: flip sign, preserve magnitude class.
//
// REACHABILITY NOTE: only the `positive → negative` and
// `at_least_nonneg → nonpositive` branches are reachable from the
// current call graph. The `-(-x) → x` fold in each domain's negative
// factory short-circuits before this helper runs, so an operand
// carrying `negative` or `nonpositive` can't reach here — those tags
// are only ever produced by THIS helper's own `mark_negative` /
// `mark_nonpositive`, and the fold strips them.
//
// The `negative → positive` and `at_least_nonpos → nonnegative`
// branches are kept for future-symmetry. If/when a sign-aware op (out
// of scope per #260) produces a `negative`-tagged result via a
// non-negative-node (e.g. `mul(positive, negative_const)` after
// sign-aware mul lands per #306), those branches start firing and the
// neg propagation stays correct without further changes.
template <typename Expr>
inline void propagate_neg(numeric_assumption_manager const &operand,
                          expression_holder<Expr> const &result) {
  if (operand.contains(numsim::cas::positive{})) {
    mark_negative(result);
  } else if (at_least_nonneg(operand)) {
    mark_nonpositive(result);
  } else if (operand.contains(numsim::cas::negative{})) {
    mark_positive(result);
  } else if (at_least_nonpos(operand)) {
    mark_nonnegative(result);
  }
}

// Pow: pow(pos, real) → pos; pow(≥nonneg, ≥nonneg) → nonneg.
// The "real exponent" guard prevents a complex exponent from silently
// inheriting positivity. (Complex constants are rejected at
// construction, but a real_tag check is still the right precondition —
// it also covers the general non-constant exponent case.) The strict
// pow(neg, even-int) → pos rule is deferred (#260 scope-out, #306).
template <typename Expr>
inline void propagate_pow(numeric_assumption_manager const &base,
                          numeric_assumption_manager const &exponent,
                          expression_holder<Expr> const &result) {
  if (base.contains(numsim::cas::positive{}) &&
      exponent.contains(numsim::cas::real_tag{})) {
    mark_positive(result);
    return;
  }
  if (at_least_nonneg(base) && at_least_nonneg(exponent))
    mark_nonnegative(result);
}

} // namespace numsim::cas::positivity

#undef NUMSIM_CAS_POSITIVITY_ASSERT_NO_CONTRADICTION

#endif // NUMSIM_CAS_CORE_POSITIVITY_PROPAGATION_H
