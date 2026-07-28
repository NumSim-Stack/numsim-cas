#ifndef NUMSIM_CAS_CORE_SIMPLIFIER_COMMON_H
#define NUMSIM_CAS_CORE_SIMPLIFIER_COMMON_H

#include <numsim_cas/core/domain_traits.h>
#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/core/scalar_number.h>
#include <numsim_cas/functions.h>

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
  // callers reach here after erase/mutation on a copied node; the copied
  // cached hash is stale either way (round-2 review on #339)
  add.invalidate_hash();
  if (add.symbol_map().empty()) {
    return add.coeff().is_valid() ? add.coeff() : Traits::zero();
  }
  if (add.symbol_map().size() == 1 && !add.coeff().is_valid()) {
    return add.symbol_map().begin()->second;
  }
  return expr;
}

// merge_or_insert with exact-negation combining and zero filtering:
// dispatcher-side child insertion must never leave a {t, -t} pair or a
// literal zero child (round-8/9 reviews).
template <typename Traits>
inline void insert_signed(typename Traits::add_type &add,
                          typename Traits::expr_holder_t entry) {
  add_insert_signed(add, std::move(entry),
                    [](typename Traits::expr_holder_t const &e) {
                      return is_same<typename Traits::zero_type>(e);
                    });
}

// Fold a numeric delta into the coefficient of `expr` (an add node).
// A valid-but-non-numeric coefficient (symbolic, t2s) is combined as an
// expression — get_coefficient reports 0 for it, and the old free()/
// set_coeff pattern silently deleted it (round-6 review).
template <typename Traits>
[[nodiscard]] expression_holder<typename Traits::expression_type>
fold_constant_into_add_coeff(
    expression_holder<typename Traits::expression_type> expr,
    scalar_number const &delta) {
  auto &add = expr.template get<typename Traits::add_type>();
  if (add.coeff().is_valid() && !Traits::try_numeric(add.coeff())) {
    auto sym = add.coeff();
    add.coeff().free();
    add.set_coeff(sym + Traits::make_constant(delta));
    return finalize_add<Traits>(std::move(expr));
  }
  const auto value{get_coefficient<Traits>(add, 0) + delta};
  add.coeff().free();
  if (value != 0) {
    add.set_coeff(Traits::make_constant(value));
  }
  return finalize_add<Traits>(std::move(expr));
}

} // namespace numsim::cas::detail

#endif // NUMSIM_CAS_CORE_SIMPLIFIER_COMMON_H
