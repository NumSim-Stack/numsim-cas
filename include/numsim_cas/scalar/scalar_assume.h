#ifndef SCALAR_ASSUME_H
#define SCALAR_ASSUME_H

#include <numsim_cas/core/assumptions.h>
#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/core/require_symbol.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

// Lazy assumption inference (defined in scalar_assumption_propagator.cpp)
void infer_assumptions(expression_holder<scalar_expression> const &expr);

// ── assume(): set assumption + implied assumptions on the node ──────────
//
// SymPy step 4: every assume(...) overload guards on is_symbol() and
// throws invalid_assumption_error on compounds / constants. The previous
// silent-accept behavior would let assume(x + y, positive{}) succeed but
// the assumption never propagated meaningfully — the user wanted "x and
// y are positive" (a fact about leaves) but wrote it on the compound.

inline void assume(expression_holder<scalar_expression> const &expr, positive) {
  detail::require_symbol(expr.get(), "assume(positive)");
  auto &a = expr.data()->assumptions();
  a.insert(positive{});
  a.insert(nonnegative{});
  a.insert(nonzero{});
  a.insert(real_tag{});
  expr.data()->assumptions().set_inferred();
}

inline void assume(expression_holder<scalar_expression> const &expr, negative) {
  detail::require_symbol(expr.get(), "assume(negative)");
  auto &a = expr.data()->assumptions();
  a.insert(negative{});
  a.insert(nonpositive{});
  a.insert(nonzero{});
  a.insert(real_tag{});
  expr.data()->assumptions().set_inferred();
}

inline void assume(expression_holder<scalar_expression> const &expr,
                   nonnegative) {
  detail::require_symbol(expr.get(), "assume(nonnegative)");
  auto &a = expr.data()->assumptions();
  a.insert(nonnegative{});
  a.insert(real_tag{});
  expr.data()->assumptions().set_inferred();
}

inline void assume(expression_holder<scalar_expression> const &expr,
                   nonpositive) {
  detail::require_symbol(expr.get(), "assume(nonpositive)");
  auto &a = expr.data()->assumptions();
  a.insert(nonpositive{});
  a.insert(real_tag{});
  expr.data()->assumptions().set_inferred();
}

inline void assume(expression_holder<scalar_expression> const &expr, nonzero) {
  detail::require_symbol(expr.get(), "assume(nonzero)");
  auto &a = expr.data()->assumptions();
  a.insert(nonzero{});
  expr.data()->assumptions().set_inferred();
}

inline void assume(expression_holder<scalar_expression> const &expr, integer) {
  detail::require_symbol(expr.get(), "assume(integer)");
  auto &a = expr.data()->assumptions();
  a.insert(integer{});
  a.insert(rational{});
  a.insert(real_tag{});
  expr.data()->assumptions().set_inferred();
}

inline void assume(expression_holder<scalar_expression> const &expr, even) {
  detail::require_symbol(expr.get(), "assume(even)");
  auto &a = expr.data()->assumptions();
  a.insert(even{});
  a.insert(integer{});
  a.insert(rational{});
  a.insert(real_tag{});
  expr.data()->assumptions().set_inferred();
}

inline void assume(expression_holder<scalar_expression> const &expr, odd) {
  detail::require_symbol(expr.get(), "assume(odd)");
  auto &a = expr.data()->assumptions();
  a.insert(odd{});
  a.insert(integer{});
  a.insert(rational{});
  a.insert(real_tag{});
  expr.data()->assumptions().set_inferred();
}

inline void assume(expression_holder<scalar_expression> const &expr, prime) {
  detail::require_symbol(expr.get(), "assume(prime)");
  auto &a = expr.data()->assumptions();
  a.insert(prime{});
  a.insert(integer{});
  a.insert(positive{});
  a.insert(nonnegative{});
  a.insert(nonzero{});
  a.insert(rational{});
  a.insert(real_tag{});
  expr.data()->assumptions().set_inferred();
}

inline void assume(expression_holder<scalar_expression> const &expr, rational) {
  detail::require_symbol(expr.get(), "assume(rational)");
  auto &a = expr.data()->assumptions();
  a.insert(rational{});
  a.insert(real_tag{});
  expr.data()->assumptions().set_inferred();
}

inline void assume(expression_holder<scalar_expression> const &expr, real_tag) {
  detail::require_symbol(expr.get(), "assume(real_tag)");
  auto &a = expr.data()->assumptions();
  a.insert(real_tag{});
  expr.data()->assumptions().set_inferred();
}

// ── remove_assumption(): remove a single assumption ─────────────────────

inline void remove_assumption(expression_holder<scalar_expression> const &expr,
                              numeric_assumption const &a) {
  expr.data()->assumptions().erase(a);
}

// ── Query helpers ───────────────────────────────────────────────────────

inline bool is_positive(expression_holder<scalar_expression> const &expr) {
  infer_assumptions(expr);
  return expr.data()->assumptions().contains(positive{});
}

inline bool is_negative(expression_holder<scalar_expression> const &expr) {
  infer_assumptions(expr);
  return expr.data()->assumptions().contains(negative{});
}

inline bool is_nonnegative(expression_holder<scalar_expression> const &expr) {
  infer_assumptions(expr);
  return expr.data()->assumptions().contains(nonnegative{});
}

inline bool is_nonpositive(expression_holder<scalar_expression> const &expr) {
  infer_assumptions(expr);
  return expr.data()->assumptions().contains(nonpositive{});
}

inline bool is_nonzero(expression_holder<scalar_expression> const &expr) {
  infer_assumptions(expr);
  return expr.data()->assumptions().contains(nonzero{});
}

inline bool is_integer(expression_holder<scalar_expression> const &expr) {
  infer_assumptions(expr);
  return expr.data()->assumptions().contains(integer{});
}

inline bool is_even(expression_holder<scalar_expression> const &expr) {
  infer_assumptions(expr);
  return expr.data()->assumptions().contains(even{});
}

inline bool is_real(expression_holder<scalar_expression> const &expr) {
  infer_assumptions(expr);
  return expr.data()->assumptions().contains(real_tag{});
}

inline bool is_rational(expression_holder<scalar_expression> const &expr) {
  infer_assumptions(expr);
  return expr.data()->assumptions().contains(rational{});
}

} // namespace numsim::cas

#endif // SCALAR_ASSUME_H
