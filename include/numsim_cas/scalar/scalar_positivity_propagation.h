#ifndef SCALAR_POSITIVITY_PROPAGATION_H
#define SCALAR_POSITIVITY_PROPAGATION_H

#include <cassert>

#include <numsim_cas/core/assumptions.h>
#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/scalar/scalar_constant.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/scalar/scalar_negative.h>
#include <numsim_cas/scalar/scalar_one.h>
#include <numsim_cas/scalar/scalar_zero.h>
// Intentionally NOT including scalar_domain_traits.h — it transitively
// pulls in scalar_std.h via scalar_all.h, which creates a cycle (since
// scalar_std.h includes this header). Open-code the "is real-by-
// construction numeric constant" check using the structural is_same
// predicates instead.

// #305 — positivity-tag propagation for scalar operators (mul, neg, pow).
//
// Direct parallel of `tensor_to_scalar_positivity_propagation.h` (#260).
// Same rule table, same aliasing-mutation contract, same defense-in-depth
// asserts. The only structural difference: scalar has no wrapper-bridge
// node, so `read()` doesn't need the scalar_wrapper-forwarding branch.
//
// When #305 and #260 are both green, the obvious refactor is to lift
// both into a templated `<Domain>` header. Deferred to #262 (cross-
// domain unification) to keep this PR focused.

namespace numsim::cas::scalar_detail::positivity {

// Tag-state view. NOTE: these are tag-manager predicates, not truth
// predicates — a caller writing incoherent tags defeats inference.
struct view {
  bool positive;
  bool nonnegative;
  bool nonpositive;
  bool negative;
  bool nonzero;
  bool real;

  bool at_least_nonneg_tag() const { return positive || nonnegative; }
  bool at_least_nonpos_tag() const { return negative || nonpositive; }
};

inline view read(expression_holder<scalar_expression> const &e) {
  // Scalar differentiation produces transient invalid holders during
  // chain-rule decomposition (a known #287-tracked corner). Guard so
  // a `read(invalid)` returns the all-false view rather than crashing
  // in numeric_assumption_manager::contains via a null data pointer.
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
  // Concrete numeric constants are real by construction. See the t2s
  // header's same-shape note (#261 tracks the broader fix of pre-
  // annotating constant nodes; this branch avoids waiting on it).
  // Open-coded structural check mirroring domain_traits::try_numeric
  // (we can't include scalar_domain_traits.h here due to the
  // scalar_std.h cycle — domain_traits includes scalar_all → scalar_std,
  // and scalar_std includes this header).
  if (!v.real) {
    if (is_same<scalar_zero>(e) || is_same<scalar_one>(e) ||
        is_same<scalar_constant>(e))
      v.real = true;
    else if (is_same<scalar_negative>(e)) {
      auto const &inner = e.template get<scalar_negative>().expr();
      if (is_same<scalar_zero>(inner) || is_same<scalar_one>(inner) ||
          is_same<scalar_constant>(inner))
        v.real = true;
    }
  }
  return v;
}

// Defense-in-depth: catches a future rule that would insert a tag
// contradicting the operand's existing state. Aliasing makes this
// critical — if `result` shares its node with an operand (via a fold
// returning the operand directly), the insertion mutates the operand.
#ifndef NDEBUG
#define NUMSIM_CAS_SCALAR_PROP_ASSERT_NO_CONTRADICTION(manager, banned_tag)    \
  do {                                                                         \
    assert(!(manager).contains(banned_tag) &&                                  \
           "scalar positivity_propagation: rule would set a tag "              \
           "contradicting the operand's existing state");                      \
  } while (0)
#else
#define NUMSIM_CAS_SCALAR_PROP_ASSERT_NO_CONTRADICTION(manager, banned_tag)    \
  ((void)0)
#endif

inline void mark_positive(expression_holder<scalar_expression> const &e) {
  auto &a = e.data()->assumptions();
  NUMSIM_CAS_SCALAR_PROP_ASSERT_NO_CONTRADICTION(a, numsim::cas::negative{});
  NUMSIM_CAS_SCALAR_PROP_ASSERT_NO_CONTRADICTION(a, numsim::cas::nonpositive{});
  a.insert(numsim::cas::positive{});
  a.insert(numsim::cas::nonnegative{});
  a.insert(numsim::cas::nonzero{});
  a.insert(numsim::cas::real_tag{});
  a.set_inferred();
}
inline void mark_negative(expression_holder<scalar_expression> const &e) {
  auto &a = e.data()->assumptions();
  NUMSIM_CAS_SCALAR_PROP_ASSERT_NO_CONTRADICTION(a, numsim::cas::positive{});
  NUMSIM_CAS_SCALAR_PROP_ASSERT_NO_CONTRADICTION(a, numsim::cas::nonnegative{});
  a.insert(numsim::cas::negative{});
  a.insert(numsim::cas::nonpositive{});
  a.insert(numsim::cas::nonzero{});
  a.insert(numsim::cas::real_tag{});
  a.set_inferred();
}
inline void mark_nonnegative(expression_holder<scalar_expression> const &e) {
  auto &a = e.data()->assumptions();
  NUMSIM_CAS_SCALAR_PROP_ASSERT_NO_CONTRADICTION(a, numsim::cas::negative{});
  a.insert(numsim::cas::nonnegative{});
  a.insert(numsim::cas::real_tag{});
  a.set_inferred();
}
inline void mark_nonpositive(expression_holder<scalar_expression> const &e) {
  auto &a = e.data()->assumptions();
  NUMSIM_CAS_SCALAR_PROP_ASSERT_NO_CONTRADICTION(a, numsim::cas::positive{});
  a.insert(numsim::cas::nonpositive{});
  a.insert(numsim::cas::real_tag{});
  a.set_inferred();
}

// Mul: pos·pos → pos; (≥nonneg)·(≥nonneg) → nonneg. ORDER MATTERS.
inline void
propagate_mul_from_views(view lhs, view rhs,
                         expression_holder<scalar_expression> const &result) {
  if (lhs.positive && rhs.positive) {
    mark_positive(result);
  } else if (lhs.at_least_nonneg_tag() && rhs.at_least_nonneg_tag()) {
    mark_nonnegative(result);
  }
}

// Neg: flip sign, preserve magnitude class. The -(-x) → x fold lives
// in the cpp file and bypasses this path. The -neg / -nonpos branches
// are kept for future-symmetry with sign-aware mul (out of scope).
inline void
propagate_neg_from_view(view operand,
                        expression_holder<scalar_expression> const &result) {
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
// ORDER MATTERS.
inline void
propagate_pow_from_views(view base, view exponent,
                         expression_holder<scalar_expression> const &result) {
  if (base.positive && exponent.real) {
    mark_positive(result);
    return;
  }
  if (base.at_least_nonneg_tag() && exponent.at_least_nonneg_tag()) {
    mark_nonnegative(result);
  }
}

} // namespace numsim::cas::scalar_detail::positivity

#endif // SCALAR_POSITIVITY_PROPAGATION_H
