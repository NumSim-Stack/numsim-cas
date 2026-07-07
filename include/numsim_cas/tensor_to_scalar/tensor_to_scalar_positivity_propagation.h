#ifndef TENSOR_TO_SCALAR_POSITIVITY_PROPAGATION_H
#define TENSOR_TO_SCALAR_POSITIVITY_PROPAGATION_H

#include <numsim_cas/core/positivity_propagation.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_domain_traits.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_scalar_wrapper.h>

// T2S adapter for core/positivity_propagation.h. Only read() is
// domain-specific; mark_*/propagate_* are shared.

namespace numsim::cas::positivity {

// Snapshot a t2s expression's sign tags (real_tag materialized). For a
// scalar_wrapper, also merge the wrapped scalar's tags (the wrapper's
// own manager is fresh). Wrapper-forwarding is single-level.
inline numeric_assumption_manager
read(expression_holder<tensor_to_scalar_expression> const &e) {
  // Guard invalid holders (user code, or a diff accumulator before its
  // first assignment) — assumptions() would null-deref otherwise.
  if (!e.is_valid())
    return {};
  numeric_assumption_manager m = e.data()->assumptions(); // value snapshot
  if (is_same<tensor_to_scalar_scalar_wrapper>(e)) {
    auto const &inner =
        e.template get<tensor_to_scalar_scalar_wrapper>().expr();
    for (auto const &t : inner.data()->assumptions().data())
      m.insert(t);
  }
  if (m.contains(numsim::cas::integer{}) ||
      m.contains(numsim::cas::rational{}) ||
      m.contains(numsim::cas::irrational{}))
    m.insert(numsim::cas::real_tag{});
  // Numeric constants are real (complex is rejected at construction).
  if (!m.contains(numsim::cas::real_tag{})) {
    using traits = numsim::cas::domain_traits<tensor_to_scalar_expression>;
    if (traits::try_numeric(e))
      m.insert(numsim::cas::real_tag{});
  }
  return m;
}

} // namespace numsim::cas::positivity

#endif // TENSOR_TO_SCALAR_POSITIVITY_PROPAGATION_H
