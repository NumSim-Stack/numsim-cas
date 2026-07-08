#ifndef SCALAR_POSITIVITY_PROPAGATION_H
#define SCALAR_POSITIVITY_PROPAGATION_H

#include <numsim_cas/core/positivity_propagation.h>
#include <numsim_cas/scalar/scalar_constant.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/scalar/scalar_negative.h>
#include <numsim_cas/scalar/scalar_one.h>
#include <numsim_cas/scalar/scalar_zero.h>

// Scalar adapter for core/positivity_propagation.h. Only read() is
// domain-specific; mark_*/propagate_* are shared.

namespace numsim::cas::positivity {

// Snapshot a scalar expression's sign tags, normalized so real_tag is
// materialized (integer/rational/irrational and numeric constants imply
// real — lets pow(x, integer) fire the real-exponent guard, #261).
inline numeric_assumption_manager
read(expression_holder<scalar_expression> const &e) {
  // Guard invalid holders (user code, or a diff accumulator before its
  // first assignment) — assumptions() would null-deref otherwise.
  if (!e.is_valid())
    return {};
  numeric_assumption_manager m = e.data()->assumptions(); // value snapshot
  if (m.contains(numsim::cas::integer{}) ||
      m.contains(numsim::cas::rational{}) ||
      m.contains(numsim::cas::irrational{}))
    m.insert(numsim::cas::real_tag{});
  // Numeric constants are real (complex is rejected at construction).
  // Structural check avoids the domain_traits include cycle.
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
