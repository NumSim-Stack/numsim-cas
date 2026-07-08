#ifndef NUMSIM_CAS_CORE_POSITIVITY_PROPAGATION_H
#define NUMSIM_CAS_CORE_POSITIVITY_PROPAGATION_H

#include <cassert>

#include <numsim_cas/core/assumptions.h>
#include <numsim_cas/core/expression_holder.h>

// Domain-agnostic positivity-tag propagation for mul/neg/pow (#260 t2s,
// #305 scalar). Each domain supplies its own `read(expr)` returning a
// normalized numeric_assumption_manager snapshot; the rules live here.
//
// read() returns a value COPY of the operand's manager, taken BEFORE the
// simplifier moves the operand (it reuses refcount-1 temporaries, so the
// holder may be moved-from by the time `result` exists). It also inserts
// real_tag whenever the operand is real-by-implication (integer/rational/
// irrational or a numeric constant), so rules only check real_tag. Cost:
// one small set copy per op — see #310 for removing the eager path.
//
// Aliasing: `result` may alias an operand (folds like `x*1 → x` return
// the operand's holder). Rules must therefore only assert tags already
// implied by the operand's precondition — a consequence of the operand
// snapshots, never strictly stronger.

namespace numsim::cas::positivity {

// "positive OR nonnegative" / "negative OR nonpositive". Robust to
// callers who set `positive` without the joint `nonnegative`.
inline bool at_least_nonneg(numeric_assumption_manager const &m) {
  return m.contains(numsim::cas::positive{}) ||
         m.contains(numsim::cas::nonnegative{});
}
inline bool at_least_nonpos(numeric_assumption_manager const &m) {
  return m.contains(numsim::cas::negative{}) ||
         m.contains(numsim::cas::nonpositive{});
}

// Debug guard: catch a rule that would set a tag contradicting the
// operand's state (unsound under aliasing). Compiled out under NDEBUG.
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

// Joint-insertion helpers, mirror scalar_assume.h. Templated so they
// work across any domain whose expression base has `assumptions()`.
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

// pos·pos → pos; (≥nonneg)·(≥nonneg) → nonneg. Stronger rule first, else
// pos·pos collapses to nonneg and loses nonzero{}.
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

// Sign flip, magnitude class preserved. Only the first two branches are
// reachable today (the `-(-x) → x` fold strips negative/nonpositive
// operands before this runs); the latter two are kept for the sign-aware
// ops in #306.
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

// pow(pos, real) → pos; pow(≥nonneg, ≥nonneg) → nonneg. The real-exponent
// guard blocks a complex exponent inheriting positivity. pow(neg, even)
// deferred (#306).
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
