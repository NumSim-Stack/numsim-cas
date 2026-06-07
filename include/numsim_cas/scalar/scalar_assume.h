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

[[deprecated("use expression_holder<scalar_expression>::assumption() instead")]]
inline void assume(expression_holder<scalar_expression> const &expr, positive) {
  detail::require_symbol(expr.get(), "assume(positive)");
  auto &a = expr.data()->assumptions();
  a.insert(positive{});
  a.insert(nonnegative{});
  a.insert(nonzero{});
  a.insert(real_tag{});
  expr.data()->assumptions().set_inferred();
}

[[deprecated("use expression_holder<scalar_expression>::assumption() instead")]]
inline void assume(expression_holder<scalar_expression> const &expr, negative) {
  detail::require_symbol(expr.get(), "assume(negative)");
  auto &a = expr.data()->assumptions();
  a.insert(negative{});
  a.insert(nonpositive{});
  a.insert(nonzero{});
  a.insert(real_tag{});
  expr.data()->assumptions().set_inferred();
}

[[deprecated("use expression_holder<scalar_expression>::assumption() instead")]]
inline void assume(expression_holder<scalar_expression> const &expr,
                   nonnegative) {
  detail::require_symbol(expr.get(), "assume(nonnegative)");
  auto &a = expr.data()->assumptions();
  a.insert(nonnegative{});
  a.insert(real_tag{});
  expr.data()->assumptions().set_inferred();
}

[[deprecated("use expression_holder<scalar_expression>::assumption() instead")]]
inline void assume(expression_holder<scalar_expression> const &expr,
                   nonpositive) {
  detail::require_symbol(expr.get(), "assume(nonpositive)");
  auto &a = expr.data()->assumptions();
  a.insert(nonpositive{});
  a.insert(real_tag{});
  expr.data()->assumptions().set_inferred();
}

[[deprecated("use expression_holder<scalar_expression>::assumption() instead")]]
inline void assume(expression_holder<scalar_expression> const &expr, nonzero) {
  detail::require_symbol(expr.get(), "assume(nonzero)");
  auto &a = expr.data()->assumptions();
  a.insert(nonzero{});
  expr.data()->assumptions().set_inferred();
}

[[deprecated("use expression_holder<scalar_expression>::assumption() instead")]]
inline void assume(expression_holder<scalar_expression> const &expr, integer) {
  detail::require_symbol(expr.get(), "assume(integer)");
  auto &a = expr.data()->assumptions();
  a.insert(integer{});
  a.insert(rational{});
  a.insert(real_tag{});
  expr.data()->assumptions().set_inferred();
}

[[deprecated("use expression_holder<scalar_expression>::assumption() instead")]]
inline void assume(expression_holder<scalar_expression> const &expr, even) {
  detail::require_symbol(expr.get(), "assume(even)");
  auto &a = expr.data()->assumptions();
  a.insert(even{});
  a.insert(integer{});
  a.insert(rational{});
  a.insert(real_tag{});
  expr.data()->assumptions().set_inferred();
}

[[deprecated("use expression_holder<scalar_expression>::assumption() instead")]]
inline void assume(expression_holder<scalar_expression> const &expr, odd) {
  detail::require_symbol(expr.get(), "assume(odd)");
  auto &a = expr.data()->assumptions();
  a.insert(odd{});
  a.insert(integer{});
  a.insert(rational{});
  a.insert(real_tag{});
  expr.data()->assumptions().set_inferred();
}

[[deprecated("use expression_holder<scalar_expression>::assumption() instead")]]
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

[[deprecated("use expression_holder<scalar_expression>::assumption() instead")]]
inline void assume(expression_holder<scalar_expression> const &expr, rational) {
  detail::require_symbol(expr.get(), "assume(rational)");
  auto &a = expr.data()->assumptions();
  a.insert(rational{});
  a.insert(real_tag{});
  expr.data()->assumptions().set_inferred();
}

[[deprecated("use expression_holder<scalar_expression>::assumption() instead")]]
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

// ── apply_assumption: dispatch for expression_holder::assumption() ──
// Found via ADL from the holder's variadic assumption() method. Each
// overload forwards to the corresponding scalar assume() overload —
// which is responsible for the implication-chain insertions (positive
// → nonzero → real, etc.). The require_symbol check was already done
// at the holder level; the assume() calls here also re-check (cheap
// virtual call).
//
// Diagnostic suppression: assume() is marked [[deprecated]] to nudge
// users toward assumption(). Internal calls from apply_assumption are
// intentional — the variadic API delegates through the legacy helper.
// Suppress the deprecation warning here so the variadic path doesn't
// emit spurious warnings at every template instantiation.
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

namespace detail {
// Membership in numeric_assumption's variant alternatives that have a
// corresponding scalar `assume(holder, tag)` overload. Constrains the
// scalar apply_assumption template so the assumption_fact_for concept on
// the holder correctly rejects bogus types (e.g. int).
//
// NOTE: `irrational` and `complex_tag` are part of the numeric_assumption
// variant but have NO assume() overloads today — they're intentionally
// omitted here so the concept rejects them up-front instead of admitting
// them and failing inside the template body. cpp-pro seventh-pass F1
// flagged this concept/dispatch mismatch. If those tags are ever needed,
// add the assume() overloads first, then re-include them in this trait.
template <typename T> struct is_numeric_assumption_tag : std::false_type {};
template <> struct is_numeric_assumption_tag<positive> : std::true_type {};
template <> struct is_numeric_assumption_tag<negative> : std::true_type {};
template <> struct is_numeric_assumption_tag<nonzero> : std::true_type {};
template <> struct is_numeric_assumption_tag<nonnegative> : std::true_type {};
template <> struct is_numeric_assumption_tag<nonpositive> : std::true_type {};
template <> struct is_numeric_assumption_tag<integer> : std::true_type {};
template <> struct is_numeric_assumption_tag<even> : std::true_type {};
template <> struct is_numeric_assumption_tag<odd> : std::true_type {};
template <> struct is_numeric_assumption_tag<rational> : std::true_type {};
template <> struct is_numeric_assumption_tag<real_tag> : std::true_type {};
template <> struct is_numeric_assumption_tag<prime> : std::true_type {};
} // namespace detail

template <typename Tag>
requires detail::is_numeric_assumption_tag<std::remove_cvref_t<Tag>>::value
inline void apply_assumption(expression_holder<scalar_expression> &h,
                             Tag &&tag) {
  assume(h, std::forward<Tag>(tag));
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

} // namespace numsim::cas

#endif // SCALAR_ASSUME_H
