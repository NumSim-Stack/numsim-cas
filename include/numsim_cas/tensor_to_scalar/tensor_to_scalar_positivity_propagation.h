#ifndef TENSOR_TO_SCALAR_POSITIVITY_PROPAGATION_H
#define TENSOR_TO_SCALAR_POSITIVITY_PROPAGATION_H

#include <numsim_cas/core/assumptions.h>
#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_domain_traits.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_scalar_wrapper.h>

// #260 — positivity-tag propagation for t2s operators (mul, neg, pow).
// Each operator captures a `view` of its operand assumptions BEFORE
// forwarding into the simplifier (which may consume the holder as an
// rvalue), then applies the matching propagation rule to the result
// holder. The result holder's assumption manager is shared via
// shared_ptr, so the insertions are visible to all references.
//
// Joint insertions mirror scalar_assume.h's pattern: a `positive`
// insertion also writes nonnegative + nonzero + real_tag, etc.
// set_inferred() at the end marks them as established facts —
// forward-compat for the assumption propagator (#246 broader fix).

namespace numsim::cas::tensor_to_scalar_detail::positivity {

struct view {
  bool positive;
  bool nonnegative;
  bool nonpositive;
  bool negative;
  bool nonzero;
};

struct view_with_real {
  view sign;
  bool real;
};

// Read sign tags from the t2s expression's assumption manager. If the
// expression is a `tensor_to_scalar_scalar_wrapper` (the bridge from
// the scalar domain into t2s), ALSO read the wrapped scalar's tags —
// the wrapper's own manager is fresh at construction and doesn't
// inherit from the scalar's. Without this transparency, a wrapped
// `pow(α, 3)` for positive α would lose its `positive` tag at the
// wrapper boundary, and the t2s mul rule below couldn't see it.
inline view read(expression_holder<tensor_to_scalar_expression> const &e) {
  auto const &a = e.data()->assumptions();
  view v{a.contains(numsim::cas::positive{}),
         a.contains(numsim::cas::nonnegative{}),
         a.contains(numsim::cas::nonpositive{}),
         a.contains(numsim::cas::negative{}),
         a.contains(numsim::cas::nonzero{})};
  if (is_same<tensor_to_scalar_scalar_wrapper>(e)) {
    auto const &inner =
        e.template get<tensor_to_scalar_scalar_wrapper>().expr();
    auto const &ia = inner.data()->assumptions();
    v.positive = v.positive || ia.contains(numsim::cas::positive{});
    v.nonnegative = v.nonnegative || ia.contains(numsim::cas::nonnegative{});
    v.nonpositive = v.nonpositive || ia.contains(numsim::cas::nonpositive{});
    v.negative = v.negative || ia.contains(numsim::cas::negative{});
    v.nonzero = v.nonzero || ia.contains(numsim::cas::nonzero{});
  }
  return v;
}

inline view_with_real
read_with_real(expression_holder<tensor_to_scalar_expression> const &e) {
  bool real = e.data()->assumptions().contains(numsim::cas::real_tag{});
  if (!real && is_same<tensor_to_scalar_scalar_wrapper>(e)) {
    auto const &inner =
        e.template get<tensor_to_scalar_scalar_wrapper>().expr();
    real = inner.data()->assumptions().contains(numsim::cas::real_tag{});
  }
  // Concrete numeric constants are real by construction — they evaluate
  // to a `scalar_number` (double under the hood today, no complex path
  // in t2s). Without this branch a pow-exponent like
  // `-get_scalar_one()` would fail the real-exponent guard since
  // numeric constants don't carry the real_tag annotation today (#261
  // tracks that broader pre-annotation). Treating try_numeric()
  // success as real_tag avoids waiting on #261 for the common case
  // and is correct: a stored numeric is real by definition.
  if (!real) {
    using traits = numsim::cas::domain_traits<tensor_to_scalar_expression>;
    if (traits::try_numeric(e))
      real = true;
  }
  return {read(e), real};
}

inline void
mark_positive(expression_holder<tensor_to_scalar_expression> const &e) {
  auto &a = e.data()->assumptions();
  a.insert(numsim::cas::positive{});
  a.insert(numsim::cas::nonnegative{});
  a.insert(numsim::cas::nonzero{});
  a.insert(numsim::cas::real_tag{});
  a.set_inferred();
}
inline void
mark_negative(expression_holder<tensor_to_scalar_expression> const &e) {
  auto &a = e.data()->assumptions();
  a.insert(numsim::cas::negative{});
  a.insert(numsim::cas::nonpositive{});
  a.insert(numsim::cas::nonzero{});
  a.insert(numsim::cas::real_tag{});
  a.set_inferred();
}
inline void
mark_nonnegative(expression_holder<tensor_to_scalar_expression> const &e) {
  auto &a = e.data()->assumptions();
  a.insert(numsim::cas::nonnegative{});
  a.insert(numsim::cas::real_tag{});
  a.set_inferred();
}
inline void
mark_nonpositive(expression_holder<tensor_to_scalar_expression> const &e) {
  auto &a = e.data()->assumptions();
  a.insert(numsim::cas::nonpositive{});
  a.insert(numsim::cas::real_tag{});
  a.set_inferred();
}

// Mul: pos·pos → pos, (any nonneg combo) → nonneg.
// Sign-aware rules (neg·neg → pos etc.) are out of scope per #260.
inline void propagate_mul_from_views(
    view lhs, view rhs,
    expression_holder<tensor_to_scalar_expression> const &result) {
  if (lhs.positive && rhs.positive) {
    mark_positive(result);
  } else if (lhs.nonnegative && rhs.nonnegative) {
    // (pos·nonneg) and (nonneg·pos) are subsumed because positive
    // implies nonnegative.
    mark_nonnegative(result);
  }
}

// Neg: flip sign, preserve magnitude class. The -(-x) → x fold lives in
// the cpp file and bypasses this path entirely.
inline void propagate_neg_from_view(
    view operand,
    expression_holder<tensor_to_scalar_expression> const &result) {
  if (operand.positive) {
    mark_negative(result);
  } else if (operand.nonnegative) {
    mark_nonpositive(result);
  } else if (operand.negative) {
    mark_positive(result);
  } else if (operand.nonpositive) {
    mark_nonnegative(result);
  }
}

// Pow: pow(pos, real) → pos; pow(nonneg, nonneg) → nonneg. The "real
// exponent" guard prevents a complex exponent from silently inheriting
// positivity. The strict pow(neg, even-int) → pos rule is deferred
// (#260 scope-out).
inline void propagate_pow_from_views(
    view_with_real base, view_with_real exponent,
    expression_holder<tensor_to_scalar_expression> const &result) {
  // An exponent is "real" if real_tag is set OR if any of the sign
  // tags is set (those joint-insert real_tag too).
  const bool exponent_is_real =
      exponent.real || exponent.sign.positive || exponent.sign.negative ||
      exponent.sign.nonnegative || exponent.sign.nonpositive;
  if (base.sign.positive && exponent_is_real) {
    mark_positive(result);
    return;
  }
  if (base.sign.nonnegative && exponent.sign.nonnegative) {
    mark_nonnegative(result);
  }
}

} // namespace numsim::cas::tensor_to_scalar_detail::positivity

#endif // TENSOR_TO_SCALAR_POSITIVITY_PROPAGATION_H
