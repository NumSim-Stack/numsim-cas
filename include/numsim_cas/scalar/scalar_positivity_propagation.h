#ifndef SCALAR_POSITIVITY_PROPAGATION_H
#define SCALAR_POSITIVITY_PROPAGATION_H

#include <numsim_cas/core/positivity_propagation.h>
#include <numsim_cas/scalar/scalar_constant.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/scalar/scalar_negative.h>
#include <numsim_cas/scalar/scalar_one.h>
#include <numsim_cas/scalar/scalar_zero.h>

// Scalar adapter for the domain-agnostic positivity propagation
// (see `core/positivity_propagation.h`). Only the `read()` function
// is domain-specific: it inspects scalar-domain leaf node types
// (scalar_zero/one/constant/negative) for the "is real-by-
// construction numeric constant" check. Everything else
// (mark_*, propagate_*) is shared.

namespace numsim::cas::positivity {

// Snapshot a scalar expression's sign tags, normalized so real_tag is
// materialized (integer/rational/irrational and numeric constants all
// imply real) — so `pow(x, integer)` exponents fire the real-exponent
// guard without waiting on #261 (constants pre-annotation).
inline numeric_assumption_manager
read(expression_holder<scalar_expression> const &e) {
  // Invalid-holder guard: an invalid holder would null-deref through
  // e.data()->assumptions(). This is reached from any
  // `tag_invoke(mul_fn / pow_fn / neg_fn, scalar, ...)` call, which
  // includes:
  //   * user code: `expression_holder<scalar_expression>{} * x`
  //     constructs an invalid lhs that the *= safety net would
  //     normally handle silently, but our read() bypasses it.
  //   * diff visitor accumulator state — was the source of the
  //     FuzzyScalarDiff seed 35 crash. scalar_differentiation now
  //     identity-inits its accumulators so the diff-internal source
  //     is closed. The guard remains for the user-code path and any
  //     future visitor that doesn't follow the identity-init
  //     pattern.
  if (!e.is_valid())
    return {};
  numeric_assumption_manager m = e.data()->assumptions(); // value snapshot
  // real-by-implication.
  if (m.contains(numsim::cas::integer{}) ||
      m.contains(numsim::cas::rational{}) ||
      m.contains(numsim::cas::irrational{}))
    m.insert(numsim::cas::real_tag{});
  // Concrete numeric constants are real by construction. Open-coded
  // structural check (instead of domain_traits::try_numeric) to
  // sidestep the scalar_domain_traits → scalar_all → scalar_std
  // include cycle. Promoting any scalar_constant to real is sound
  // because complex constants are rejected at scalar_constant
  // construction (scalar_constant.h) — every constant here is real.
  if (!m.contains(numsim::cas::real_tag{})) {
    if (is_same<scalar_zero>(e) || is_same<scalar_one>(e) ||
        is_same<scalar_constant>(e))
      m.insert(numsim::cas::real_tag{});
    else if (is_same<scalar_negative>(e)) {
      auto const &inner = e.template get<scalar_negative>().expr();
      if (is_same<scalar_zero>(inner) || is_same<scalar_one>(inner) ||
          is_same<scalar_constant>(inner))
        m.insert(numsim::cas::real_tag{});
    }
  }
  return m;
}

} // namespace numsim::cas::positivity

#endif // SCALAR_POSITIVITY_PROPAGATION_H
