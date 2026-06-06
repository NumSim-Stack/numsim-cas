#ifndef TENSOR_ASSUME_H
#define TENSOR_ASSUME_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/require_symbol.h>
#include <numsim_cas/tensor/projector_algebra.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/tensor_zero.h>

namespace numsim::cas {

// --- Set assumptions ---
//
// SymPy step 4: every assume_* helper guards on is_symbol() and throws
// invalid_assumption_error on compounds / constants / wrappers. The
// previous silent-accept behavior allowed user code to assert facts on
// non-Symbols where the result was either redundant (constants are
// already classified by their type) or semantically wrong (compounds
// derive structure from children, not from user assertion).

inline void assume_symmetric(expression_holder<tensor_expression> const &expr) {
  detail::require_symbol(expr.get(), "assume_symmetric");
  expr.data()->set_space({Symmetric{}, AnyTraceTag{}});
}

inline void assume_skew(expression_holder<tensor_expression> const &expr) {
  detail::require_symbol(expr.get(), "assume_skew");
  expr.data()->set_space({Skew{}, AnyTraceTag{}});
}

inline void
assume_volumetric(expression_holder<tensor_expression> const &expr) {
  detail::require_symbol(expr.get(), "assume_volumetric");
  expr.data()->set_space({Symmetric{}, VolumetricTag{}});
}

inline void
assume_deviatoric(expression_holder<tensor_expression> const &expr) {
  detail::require_symbol(expr.get(), "assume_deviatoric");
  expr.data()->set_space({Symmetric{}, DeviatoricTag{}});
}

inline void assume_minor(expression_holder<tensor_expression> const &expr) {
  detail::require_symbol(expr.get(), "assume_minor");
  expr.data()->set_space({Minor{}, AnyTraceTag{}});
}

inline void assume_major(expression_holder<tensor_expression> const &expr) {
  detail::require_symbol(expr.get(), "assume_major");
  expr.data()->set_space({Major{}, AnyTraceTag{}});
}

inline void
assume_minor_major(expression_holder<tensor_expression> const &expr) {
  detail::require_symbol(expr.get(), "assume_minor_major");
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
// PD/PSD => symmetric: set the space if not already a recognised sym
// subspace. Two families count as "sym-or-more-specific":
//   - Rank-2 ProjKind::Sym / Vol / Dev (the existing case).
//   - Rank-4 Minor / MinorMajor perms (added for #246 α-2b — PD on a
//     rank-4 stiffness with minor-major symmetry is the canonical
//     case in continuum-mechanics elasticity tangents; without this
//     branch the helper would overwrite MinorMajor with rank-2 Sym
//     and lose the rank-4 structural info).
// Incompatible spaces (Skew, plain Major-only without Minor, etc.)
// still get overwritten — but ONLY at rank-2, since `Symmetric{}` is
// a rank-2 perm. For rank-4 with no recognised space, the helper is
// a no-op: writing a rank-2 Sym tag to a rank-4 tensor would be a
// structural mismatch. The user can call assume_minor_major /
// assume_major explicitly if they want a sym subspace on a rank-4
// input; the PD/PSD algebra-manager implication still stands.
inline void set_symmetric_unless_more_specific(tensor_expression *e) {
  if (auto const &sp = e->space()) {
    auto kind = classify_space(*sp);
    if (kind == ProjKind::Sym || kind == ProjKind::Vol || kind == ProjKind::Dev)
      return;
    if (std::holds_alternative<Minor>(sp->perm) ||
        std::holds_alternative<MinorMajor>(sp->perm))
      return;
  }
  // Rank gate: only rank-2 gets the Symmetric{} write. Rank-4 with no
  // qualifying space is left untouched — see the helper doc above.
  if (e->rank() != 2)
    return;
  e->set_space({Symmetric{}, AnyTraceTag{}});
}
} // namespace detail

inline void
assume_orthogonal(expression_holder<tensor_expression> const &expr) {
  detail::require_symbol(expr.get(), "assume_orthogonal");
  expr.data()->tensor_algebra_assumptions().insert(orthogonal{});
}

inline void
assume_positive_definite(expression_holder<tensor_expression> const &expr) {
  detail::require_symbol(expr.get(), "assume_positive_definite");
  auto &a = expr.data()->tensor_algebra_assumptions();
  a.insert(positive_definite{});
  // PD => PSD by definition.
  a.insert(positive_semidefinite{});
  // PD => symmetric (cross-mechanism implication onto the space field).
  detail::set_symmetric_unless_more_specific(expr.data().get());
}

inline void
assume_positive_semidefinite(expression_holder<tensor_expression> const &expr) {
  detail::require_symbol(expr.get(), "assume_positive_semidefinite");
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
  // Zero short-circuit: 0 = 0^T trivially, so zero is symmetric at any rank.
  // tensor_zero never appears as a subterm (collapse-rules ensure it's only
  // ever a top-level result), so direct holder queries are the only path
  // that reaches this. See docs/sympy-assumption-redesign.md.
  if (is_same<tensor_zero>(expr))
    return true;
  // PD / PSD imply symmetric independently of the projector-space tag, so
  // a user who annotates PD then calls clear_space() still sees symmetric.
  auto const &a = expr.get().tensor_algebra_assumptions();
  if (a.contains(positive_definite{}) || a.contains(positive_semidefinite{}))
    return true;
  auto const &sp = expr.get().space();
  if (!sp)
    return false;
  // MinorMajor at rank-4 is the "fully symmetric" rank-4 case (both minor
  // C_ijkl = C_jikl = C_ijlk and major C_ijkl = C_klij symmetries). The
  // canonical example is the rank-4 minor identity δ_ik·δ_jl. classify_space
  // maps MinorMajor → Other (it's not a rank-2 perm classification), so we
  // need an explicit branch — same cross-mechanism pattern as the PD check.
  // Plain Minor or plain Major alone don't make a tensor "symmetric" in the
  // strongest sense, so they're not included here. Trace gate: a
  // {MinorMajor, VolumetricTag} combination is constructible via set_space
  // but not via any assume_* helper; the rank-4 MinorMajor classification
  // is only well-defined with AnyTraceTag (no trace constraint applied).
  if (std::holds_alternative<MinorMajor>(sp->perm) &&
      std::holds_alternative<AnyTraceTag>(sp->trace))
    return true;
  auto kind = classify_space(*sp);
  // Sym, Vol, Dev are all subspaces of Sym
  return kind == ProjKind::Sym || kind == ProjKind::Vol ||
         kind == ProjKind::Dev;
}

inline bool is_skew(expression_holder<tensor_expression> const &expr) {
  // Zero short-circuit: 0 = -0^T trivially, so zero is also skew. This is
  // the one place Sym and Skew can both be true for the same expression;
  // every other concrete expression has a single perm classification.
  if (is_same<tensor_zero>(expr))
    return true;
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

// ── apply_assumption: dispatch for expression_holder::assumption() ──
// Found via ADL from the holder's variadic assumption() method. Each
// overload forwards to the corresponding named assume_*() helper.
// Tensor facts span two storage layers:
//   - Structural perm/trace classification → m_tensor_space (via the
//     per-fact-type tag overloads on Symmetric, Skew, etc.)
//   - Algebraic facts (orthogonal, PD, PSD) → m_tensor_algebra_assumptions
//
// The require_symbol check happens at the holder level; the assume_*
// helpers also re-check (cheap virtual call).

inline void apply_assumption(expression_holder<tensor_expression> &h,
                             Symmetric) {
  assume_symmetric(h);
}
inline void apply_assumption(expression_holder<tensor_expression> &h, Skew) {
  assume_skew(h);
}
inline void apply_assumption(expression_holder<tensor_expression> &h, Minor) {
  assume_minor(h);
}
inline void apply_assumption(expression_holder<tensor_expression> &h, Major) {
  assume_major(h);
}
inline void apply_assumption(expression_holder<tensor_expression> &h,
                             MinorMajor) {
  assume_minor_major(h);
}
inline void apply_assumption(expression_holder<tensor_expression> &h,
                             VolumetricTag) {
  assume_volumetric(h);
}
inline void apply_assumption(expression_holder<tensor_expression> &h,
                             DeviatoricTag) {
  assume_deviatoric(h);
}
inline void apply_assumption(expression_holder<tensor_expression> &h,
                             orthogonal) {
  assume_orthogonal(h);
}
inline void apply_assumption(expression_holder<tensor_expression> &h,
                             positive_definite) {
  assume_positive_definite(h);
}
inline void apply_assumption(expression_holder<tensor_expression> &h,
                             positive_semidefinite) {
  assume_positive_semidefinite(h);
}

} // namespace numsim::cas

#endif // TENSOR_ASSUME_H
