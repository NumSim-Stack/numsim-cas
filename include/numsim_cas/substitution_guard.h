#ifndef SUBSTITUTION_GUARD_H
#define SUBSTITUTION_GUARD_H

#include <string>

#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/core/substitute.h>
#include <numsim_cas/scalar/scalar_assume.h>
#include <numsim_cas/tensor/tensor_assume.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_scalar_wrapper.h>

namespace numsim::cas {

// A fold may have consumed the assumptions asserted on a symbol and erased
// the operation that used them (skew(S) -> 0 for symmetric S), so a
// replacement that does not provably carry the same assumptions would leave
// an unjustified result behind. Unprovable counts as not carried.

namespace detail {

[[noreturn]] inline void reject_substitution(std::string const &fact) {
  throw invalid_expression_error(
      "substitute: replacement does not carry the assumption '" + fact +
      "' asserted on the symbol it replaces");
}

inline char const *numeric_assumption_name(numeric_assumption const &a) {
  static_assert(std::variant_size_v<numeric_assumption> == 13);
  static constexpr char const *names[] = {
      "positive", "negative", "nonzero", "nonnegative", "nonpositive",
      "integer",  "even",     "odd",     "rational",    "irrational",
      "real",     "complex",  "prime"};
  return names[a.index()];
}

inline char const *
tensor_algebra_assumption_name(tensor_algebra_assumption const &a) {
  static_assert(std::variant_size_v<tensor_algebra_assumption> == 5);
  static constexpr char const *names[] = {
      "orthogonal", "positive_definite", "positive_semidefinite",
      "proper_rotation", "improper_rotation"};
  return names[a.index()];
}

} // namespace detail

inline void
validate_substitution(expression_holder<scalar_expression> const &old_val,
                      expression_holder<scalar_expression> const &new_val) {
  if (!old_val.is_valid() || !new_val.is_valid() || !old_val.get().is_symbol())
    return;
  auto const &facts = old_val.get().assumptions().data();
  if (facts.empty())
    return;
  infer_assumptions(new_val);
  for (auto const &a : facts) {
    if (!new_val.get().assumptions().contains(a))
      detail::reject_substitution(detail::numeric_assumption_name(a));
  }
}

inline void
validate_substitution(expression_holder<tensor_expression> const &old_val,
                      expression_holder<tensor_expression> const &new_val) {
  if (!old_val.is_valid() || !new_val.is_valid() || !old_val.get().is_symbol())
    return;

  if (auto const &sp = old_val.get().space(); sp) {
    bool perm_ok = true;
    if (std::holds_alternative<Symmetric>(sp->perm))
      perm_ok = is_symmetric(new_val);
    else if (std::holds_alternative<Skew>(sp->perm))
      perm_ok = is_skew(new_val);
    else if (std::holds_alternative<Minor>(sp->perm))
      perm_ok = is_minor(new_val);
    else if (std::holds_alternative<Major>(sp->perm))
      perm_ok = is_major(new_val);
    else if (std::holds_alternative<MinorMajor>(sp->perm))
      perm_ok = is_minor_major(new_val);
    else if (!std::holds_alternative<General>(sp->perm)) {
      // payload-carrying tags (Young) have no predicate: require the same tag
      auto const &np = new_val.get().space();
      perm_ok = np && np->perm == sp->perm;
    }
    if (!perm_ok)
      detail::reject_substitution("permutation symmetry");

    if (!std::holds_alternative<AnyTraceTag>(sp->trace)) {
      bool trace_ok = false;
      if (std::holds_alternative<VolumetricTag>(sp->trace))
        trace_ok = is_volumetric(new_val);
      else if (std::holds_alternative<DeviatoricTag>(sp->trace))
        trace_ok = is_deviatoric(new_val);
      else {
        auto const &np = new_val.get().space();
        trace_ok = np && np->trace == sp->trace;
      }
      if (!trace_ok)
        detail::reject_substitution("trace constraint");
    }
  }

  for (auto const &a : old_val.get().tensor_algebra_assumptions().data()) {
    bool ok = false;
    if (std::holds_alternative<orthogonal>(a))
      ok = is_orthogonal(new_val);
    else if (std::holds_alternative<proper_rotation>(a))
      ok = is_proper_rotation(new_val);
    else if (std::holds_alternative<improper_rotation>(a))
      ok = is_improper_rotation(new_val);
    else if (std::holds_alternative<positive_definite>(a))
      ok = is_positive_definite(new_val);
    else if (std::holds_alternative<positive_semidefinite>(a))
      ok = is_positive_semidefinite(new_val);
    if (!ok)
      detail::reject_substitution(detail::tensor_algebra_assumption_name(a));
  }
}

inline void validate_substitution(
    expression_holder<tensor_to_scalar_expression> const &old_val,
    expression_holder<tensor_to_scalar_expression> const &new_val) {
  if (!old_val.is_valid() || !new_val.is_valid() || !old_val.get().is_symbol())
    return;
  // A t2s symbol is a wrapped scalar, and the facts sit on the wrapped
  // expression; a non-wrapper replacement must carry them itself.
  auto unwrap = [](expression_holder<tensor_to_scalar_expression> const &h) {
    expression_holder<scalar_expression> inner;
    if (is_same<tensor_to_scalar_scalar_wrapper>(h))
      inner = h.get<tensor_to_scalar_scalar_wrapper>().expr();
    return inner;
  };
  auto old_inner = unwrap(old_val);
  if (!old_inner.is_valid()) {
    for (auto const &a : old_val.get().assumptions().data()) {
      if (!new_val.get().assumptions().contains(a))
        detail::reject_substitution(detail::numeric_assumption_name(a));
    }
    return;
  }
  if (auto new_inner = unwrap(new_val); new_inner.is_valid()) {
    validate_substitution(old_inner, new_inner);
    return;
  }
  for (auto const &a : old_inner.get().assumptions().data()) {
    if (!new_val.get().assumptions().contains(a))
      detail::reject_substitution(detail::numeric_assumption_name(a));
  }
}

} // namespace numsim::cas

#endif // SUBSTITUTION_GUARD_H
