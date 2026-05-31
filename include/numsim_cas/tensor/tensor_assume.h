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
// These live on the separate AlgKind field (orthogonal to the projector-
// space tags above). PD / PSD additionally imply Symmetric, so they
// auto-set the projector-space tag — a subsequent is_symmetric() query
// returns true without an explicit assume_symmetric() call.
//
// Orthogonality does NOT imply symmetry (rotations are generally not
// symmetric), so assume_orthogonal leaves the space tag alone.

inline void
assume_orthogonal(expression_holder<tensor_expression> const &expr) {
  expr.data()->set_algebra_kind(AlgKind::Orthogonal);
}

inline void
assume_positive_definite(expression_holder<tensor_expression> const &expr) {
  expr.data()->set_algebra_kind(AlgKind::PositiveDefinite);
  expr.data()->set_space({Symmetric{}, AnyTraceTag{}});
}

inline void
assume_positive_semidefinite(expression_holder<tensor_expression> const &expr) {
  expr.data()->set_algebra_kind(AlgKind::PositiveSemidefinite);
  expr.data()->set_space({Symmetric{}, AnyTraceTag{}});
}

// --- Query assumptions ---

inline bool is_symmetric(expression_holder<tensor_expression> const &expr) {
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

inline bool is_orthogonal(expression_holder<tensor_expression> const &expr) {
  return expr.get().algebra_kind() == AlgKind::Orthogonal;
}

inline bool
is_positive_definite(expression_holder<tensor_expression> const &expr) {
  return expr.get().algebra_kind() == AlgKind::PositiveDefinite;
}

inline bool
is_positive_semidefinite(expression_holder<tensor_expression> const &expr) {
  // PD => PSD: a positive-definite tensor is also positive-semidefinite by
  // definition (the strict inequality implies the weak one). Queries return
  // true in both cases.
  auto k = expr.get().algebra_kind();
  return k == AlgKind::PositiveSemidefinite || k == AlgKind::PositiveDefinite;
}

} // namespace numsim::cas

#endif // TENSOR_ASSUME_H
