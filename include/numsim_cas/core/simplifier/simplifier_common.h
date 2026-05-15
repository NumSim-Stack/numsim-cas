#ifndef NUMSIM_CAS_CORE_SIMPLIFIER_COMMON_H
#define NUMSIM_CAS_CORE_SIMPLIFIER_COMMON_H

#include <numsim_cas/core/expression_holder.h>

namespace numsim::cas::detail {

// finalize_add: collapses an `n_ary_tree`-backed add expression to a simpler
// form when its content is trivial. Used at the end of dispatchers that
// build an add from scratch (e.g. n_ary_sub_dispatch::dispatch(add_type),
// where children matching by key can fully cancel).
//
// Returns one of:
//   - the coefficient expression — if the symbol_map is empty and the
//     coefficient is valid;
//   - Traits::zero() — if the symbol_map is empty AND the coefficient is
//     invalid (i.e. nothing remained after cancellation);
//   - the single child — if the symbol_map has exactly one child and no
//     coefficient is set;
//   - the original `expr` unchanged otherwise.
//
// Callers via the visitor pattern propagate the holder transparently, so
// the return-type broadening is invisible to them.
//
// Why a free helper rather than a member on n_ary_tree: the zero-fallback
// is domain-specific (Traits::zero()), which n_ary_tree doesn't have access
// to. This helper sits in the simplifier layer where Traits is already in
// scope at the call site.
template <typename Traits>
[[nodiscard]] expression_holder<typename Traits::expression_type>
finalize_add(expression_holder<typename Traits::expression_type> expr) {
  auto &add = expr.template get<typename Traits::add_type>();
  if (add.symbol_map().empty()) {
    return add.coeff().is_valid() ? add.coeff() : Traits::zero();
  }
  if (add.symbol_map().size() == 1 && !add.coeff().is_valid()) {
    return add.symbol_map().begin()->second;
  }
  return expr;
}

} // namespace numsim::cas::detail

#endif // NUMSIM_CAS_CORE_SIMPLIFIER_COMMON_H
