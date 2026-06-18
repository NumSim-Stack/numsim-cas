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
// Everything else (mark_*, propagate_*, view) is shared.

namespace numsim::cas::positivity {

// Read sign tags from a t2s expression's assumption manager. If the
// expression is a `tensor_to_scalar_scalar_wrapper`, ALSO read the
// wrapped scalar's tags — the wrapper's own manager is fresh at
// construction and doesn't inherit. Wrapper-forwarding is
// single-level (the wrapper today wraps a scalar_expression, never
// another t2s).
inline view read(expression_holder<tensor_to_scalar_expression> const &e) {
  // Invalid-holder guard: an invalid holder would null-deref through
  // numeric_assumption_manager::contains. Reached from any
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
  // Concrete numeric constants are real by construction. #261 will
  // pre-annotate these, but until then we promote try_numeric()
  // success to real_tag here.
  if (!v.real) {
    using traits = numsim::cas::domain_traits<tensor_to_scalar_expression>;
    if (traits::try_numeric(e))
      v.real = true;
  }
  return v;
}

} // namespace numsim::cas::positivity

#endif // TENSOR_TO_SCALAR_POSITIVITY_PROPAGATION_H
