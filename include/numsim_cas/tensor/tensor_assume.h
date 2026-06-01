#ifndef TENSOR_ASSUME_H
#define TENSOR_ASSUME_H

#include <numsim_cas/tensor/projector_algebra.h>
#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

// --- Set assumptions ---

inline void assume_symmetric(expression_holder<tensor_expression> const &expr) {
  expr.data()->set_space({Symmetric{}, AnyTraceTag{}});
}

inline void assume_skew(expression_holder<tensor_expression> const &expr) {
  expr.data()->set_space({Skew{}, AnyTraceTag{}});
}

inline void
assume_volumetric(expression_holder<tensor_expression> const &expr) {
  expr.data()->set_space({Symmetric{}, VolumetricTag{}});
}

inline void
assume_deviatoric(expression_holder<tensor_expression> const &expr) {
  expr.data()->set_space({Symmetric{}, DeviatoricTag{}});
}

inline void assume_minor(expression_holder<tensor_expression> const &expr) {
  expr.data()->set_space({Minor{}, AnyTraceTag{}});
}

inline void assume_major(expression_holder<tensor_expression> const &expr) {
  expr.data()->set_space({Major{}, AnyTraceTag{}});
}

inline void
assume_minor_major(expression_holder<tensor_expression> const &expr) {
  expr.data()->set_space({MinorMajor{}, AnyTraceTag{}});
}

// --- Algebraic-property annotations (#228) ---
// Set membership in the tensor_algebra_assumption_manager — mirrors the
// scalar `assume(expr, positive{})` pattern with auto-implication chains
// encoded as joint insertions (PD => PSD).
//
// PD / PSD also imply symmetric; that's a cross-mechanism implication
// (the projector-space tag lives on a separate field). We propagate it
// by setting {Symmetric, AnyTraceTag} on the space, unless a strictly
// more-specific symmetric subspace (Vol / Dev) is already annotated —
// volumetric-PD (positive multiple of I) is a real case and the Vol tag
// must be preserved.

namespace detail {
// PD/PSD => symmetric: set the space if not already a Sym subspace.
// Incompatible spaces (Skew, Minor, Major, MinorMajor) get overwritten.
inline void set_symmetric_unless_more_specific(tensor_expression *e) {
  if (auto const &sp = e->space()) {
    auto kind = classify_space(*sp);
    if (kind == ProjKind::Sym || kind == ProjKind::Vol || kind == ProjKind::Dev)
      return;
  }
  e->set_space({Symmetric{}, AnyTraceTag{}});
}
} // namespace detail

inline void
assume_orthogonal(expression_holder<tensor_expression> const &expr) {
  expr.data()->tensor_algebra_assumptions().insert(orthogonal{});
}

inline void
assume_positive_definite(expression_holder<tensor_expression> const &expr) {
  auto &a = expr.data()->tensor_algebra_assumptions();
  a.insert(positive_definite{});
  // PD => PSD by definition.
  a.insert(positive_semidefinite{});
  // PD => symmetric (cross-mechanism implication onto the space field).
  detail::set_symmetric_unless_more_specific(expr.data().get());
}

inline void
assume_positive_semidefinite(expression_holder<tensor_expression> const &expr) {
  auto &a = expr.data()->tensor_algebra_assumptions();
  a.insert(positive_semidefinite{});
  detail::set_symmetric_unless_more_specific(expr.data().get());
}

// Remove a single algebra assumption. Mirrors scalar_assume.h's
// remove_assumption. NOTE: this does NOT undo cross-mechanism
// implications — removing positive_definite{} leaves the
// {Symmetric, AnyTraceTag} space tag in place, since the user's
// original belief "A is symmetric" may still stand independently.
// Use clear_space() separately if the space tag should also go.
inline void remove_assumption(expression_holder<tensor_expression> const &expr,
                              tensor_algebra_assumption const &a) {
  expr.data()->tensor_algebra_assumptions().erase(a);
}

// --- Query assumptions ---

inline bool is_symmetric(expression_holder<tensor_expression> const &expr) {
  // PD / PSD imply symmetric independently of the projector-space tag, so
  // a user who annotates PD then calls clear_space() still sees symmetric.
  auto const &a = expr.get().tensor_algebra_assumptions();
  if (a.contains(positive_definite{}) || a.contains(positive_semidefinite{}))
    return true;
  auto const &sp = expr.get().space();
  if (!sp)
    return false;
  auto kind = classify_space(*sp);
  // Sym, Vol, Dev are all subspaces of Sym
  return kind == ProjKind::Sym || kind == ProjKind::Vol ||
         kind == ProjKind::Dev;
}

inline bool is_skew(expression_holder<tensor_expression> const &expr) {
  auto const &sp = expr.get().space();
  if (!sp)
    return false;
  return classify_space(*sp) == ProjKind::Skew;
}

inline bool is_volumetric(expression_holder<tensor_expression> const &expr) {
  auto const &sp = expr.get().space();
  if (!sp)
    return false;
  return classify_space(*sp) == ProjKind::Vol;
}

inline bool is_deviatoric(expression_holder<tensor_expression> const &expr) {
  auto const &sp = expr.get().space();
  if (!sp)
    return false;
  return classify_space(*sp) == ProjKind::Dev;
}

inline bool is_minor(expression_holder<tensor_expression> const &expr) {
  auto const &sp = expr.get().space();
  if (!sp)
    return false;
  return std::holds_alternative<Minor>(sp->perm);
}

inline bool is_major(expression_holder<tensor_expression> const &expr) {
  auto const &sp = expr.get().space();
  if (!sp)
    return false;
  return std::holds_alternative<Major>(sp->perm);
}

inline bool is_minor_major(expression_holder<tensor_expression> const &expr) {
  auto const &sp = expr.get().space();
  if (!sp)
    return false;
  return std::holds_alternative<MinorMajor>(sp->perm);
}

// --- Algebraic-property queries (#228) ---
// Simple set-membership checks against the tensor_algebra_assumption_manager.
// Auto-implication (PD => PSD) is already baked into the set by
// assume_positive_definite() — no special-case logic needed here.

inline bool is_orthogonal(expression_holder<tensor_expression> const &expr) {
  return expr.get().tensor_algebra_assumptions().contains(orthogonal{});
}

inline bool
is_positive_definite(expression_holder<tensor_expression> const &expr) {
  return expr.get().tensor_algebra_assumptions().contains(positive_definite{});
}

inline bool
is_positive_semidefinite(expression_holder<tensor_expression> const &expr) {
  return expr.get().tensor_algebra_assumptions().contains(
      positive_semidefinite{});
}

} // namespace numsim::cas

#endif // TENSOR_ASSUME_H
