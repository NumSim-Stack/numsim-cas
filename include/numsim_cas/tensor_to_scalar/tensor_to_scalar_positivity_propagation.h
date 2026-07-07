#ifndef TENSOR_TO_SCALAR_POSITIVITY_PROPAGATION_H
#define TENSOR_TO_SCALAR_POSITIVITY_PROPAGATION_H

#include <numsim_cas/core/positivity_propagation.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_domain_traits.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_scalar_wrapper.h>

// T2S adapter for the domain-agnostic positivity propagation (see
// `core/positivity_propagation.h`). Only the `read()` function is
// domain-specific: it forwards through `tensor_to_scalar_scalar_wrapper`
// (the bridge from the scalar domain into t2s) to the inner scalar's
// tags, and checks numeric constants via domain_traits::try_numeric.
// Everything else (mark_*, propagate_*) is shared.

namespace numsim::cas::positivity {

// Snapshot a t2s expression's sign tags (normalized so real_tag is
// materialized). If the expression is a `tensor_to_scalar_scalar_wrapper`,
// ALSO merge the wrapped scalar's tags — the wrapper's own manager is
// fresh at construction and doesn't inherit. Wrapper-forwarding is
// single-level (the wrapper today wraps a scalar_expression, never
// another t2s).
inline numeric_assumption_manager
read(expression_holder<tensor_to_scalar_expression> const &e) {
  // Invalid-holder guard: an invalid holder would null-deref through
  // e.data()->assumptions(). Reached from any
  // `tag_invoke(mul_fn / pow_fn / neg_fn, t2s, ...)` call, which
  // includes:
  //   * user code: `expression_holder<tensor_to_scalar_expression>{}
  //     * x` constructs an invalid lhs that the *= safety net would
  //     normally handle silently, but our read() bypasses it.
  //   * diff visitor accumulator state — tensor_to_scalar_add and
  //     all other tensor-domain diff accumulators use the explicit
  //     `if (acc.is_valid()) ...` pattern, so the diff-internal
  //     source is closed. The guard remains for the user-code path
  //     and any future visitor that doesn't follow the pattern.
  if (!e.is_valid())
    return {};
  numeric_assumption_manager m = e.data()->assumptions(); // value snapshot
  if (is_same<tensor_to_scalar_scalar_wrapper>(e)) {
    auto const &inner =
        e.template get<tensor_to_scalar_scalar_wrapper>().expr();
    for (auto const &t : inner.data()->assumptions().data())
      m.insert(t);
  }
  // real-by-implication.
  if (m.contains(numsim::cas::integer{}) ||
      m.contains(numsim::cas::rational{}) ||
      m.contains(numsim::cas::irrational{}))
    m.insert(numsim::cas::real_tag{});
  // Concrete numeric constants are real by construction. #261 will
  // pre-annotate these, but until then we promote try_numeric() success
  // to real_tag here. Sound because complex constants are rejected at
  // scalar_constant construction (scalar_constant.h), so try_numeric()
  // can only return a real value.
  if (!m.contains(numsim::cas::real_tag{})) {
    using traits = numsim::cas::domain_traits<tensor_to_scalar_expression>;
    if (traits::try_numeric(e))
      m.insert(numsim::cas::real_tag{});
  }
  return m;
}

} // namespace numsim::cas::positivity

#endif // TENSOR_TO_SCALAR_POSITIVITY_PROPAGATION_H
