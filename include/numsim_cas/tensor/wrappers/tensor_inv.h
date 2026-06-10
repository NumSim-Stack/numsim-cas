#ifndef TENSOR_INV_H
#define TENSOR_INV_H

#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor/tensor_assume.h>
#include <numsim_cas/tensor/tensor_expression.h>
namespace numsim::cas {

class tensor_inv final : public unary_op<tensor_node_base_t<tensor_inv>> {
public:
  using base = unary_op<tensor_node_base_t<tensor_inv>>;

  template <typename Expr>
  explicit tensor_inv(
      Expr &&_expr) // NOLINT(bugprone-forwarding-reference-overload)
      : base(std::forward<Expr>(_expr), _expr.get().dim(), _expr.get().rank()) {
    // ── Rank gate (#292) ──────────────────────────────────────────
    // Mirror the inv() factory's rank gate (tensor_functions.h:467)
    // here so direct `make_expression<tensor_inv>(...)` constructions
    // that bypass the factory still get rejected at the same point.
    // Without this, the space-propagation block below silently produces
    // a tensor_inv with no annotation, and downstream consumers
    // (evaluator, every visitor) misbehave in ways that vary by site —
    // the diff visitors had to grow belt-and-braces rank checks to
    // catch this. Throw matches the factory's convention.
    if (this->rank() != 2 && this->rank() != 4) {
      throw invalid_expression_error(
          "tensor_inv: only rank-2 and rank-4 tensors are supported "
          "(got rank " +
          std::to_string(this->rank()) + ")");
    }
    // ── Space propagation (projector-space tag) ────────────────────
    if (auto const &sp = this->expr().get().space()) {
      const auto r = this->rank();
      if (r == 2) {
        // Rank-2: tmech::inv preserves Symmetric, Volumetric, and Skew
        // perms. Deviatoric / Harmonic trace tags are NOT preserved
        // (tr(A^{-1}) != 0 in general) — downgrade to AnyTraceTag while
        // keeping the perm.
        if (std::holds_alternative<DeviatoricTag>(sp->trace) ||
            std::holds_alternative<HarmonicTag>(sp->trace))
          this->set_space({Symmetric{}, AnyTraceTag{}});
        else
          this->set_space(*sp);
      } else if (r == 4) {
        // Rank-4 (#248): only Minor / MinorMajor perms survive. Those
        // route to tmech::inv (Voigt-symmetric path), which DOES
        // preserve the Voigt symmetry — so the inverse is also
        // minor- (resp. minor-and-major-)symmetric. Any other perm
        // (Skew, Sym_r, Anti_r, Harm_r, Young, General) routes to
        // tmech::invf (fully anisotropic) — those algebraic properties
        // are NOT preserved by the full 9×9 matrix inverse, so the
        // result is general anisotropic. Leave the annotation cleared
        // in that case rather than incorrectly preserving it.
        if (std::holds_alternative<Minor>(sp->perm) ||
            std::holds_alternative<MinorMajor>(sp->perm))
          this->set_space(*sp);
        // else: space left unset on the result (general anisotropic).
      }
      // Other ranks: inv() factory rejects them; this branch
      // shouldn't be reached.
    }

    // ── Algebra-assumption propagation (#246 α-2b) ─────────────────
    // inv(PD) is PD (positive-definite matrices stay PD under
    // inversion — eigenvalues 1/λᵢ are all positive when λᵢ are).
    // inv(PSD) propagates PSD conservatively: a non-singular PSD is
    // actually PD, but we can't tell singularity symbolically, so we
    // propagate exactly what the user asserted; the runtime eval
    // throws if actually singular.
    //
    // PD/PSD also imply symmetric. Reuse the detail helper to set the
    // Symmetric space iff no strictly more specific sym subspace
    // (Vol/Dev at rank-2; Minor/MinorMajor at rank-4) is already set.
    //
    // NOTE on ordering: this runs AFTER the space-propagation block
    // above. For contradictory inputs (e.g. assume_skew(W) +
    // assume_positive_definite(W)) the space block transfers Skew to
    // the result first; then this block OVERWRITES with Sym via the
    // detail helper. The user-asserted contradiction resolves by
    // honouring PD's symmetric implication on the output.
    //
    // Orthogonal is intentionally NOT propagated here. At rank-2 the
    // inv() factory folds inv(orthogonal) → trans() before reaching
    // this ctor (α-2a). At rank-4 "orthogonal" isn't a standard
    // concept so the annotation is vacuous on input and silently
    // dropped — calling assume_orthogonal on a rank-4 tensor is a
    // user error, not propagated.
    auto const &input_alg = this->expr().get().tensor_algebra_assumptions();
    const bool input_has_pd = input_alg.contains(positive_definite{});
    const bool input_has_psd =
        input_has_pd || input_alg.contains(positive_semidefinite{});
    if (input_has_psd) {
      auto &out = this->tensor_algebra_assumptions();
      if (input_has_pd)
        out.insert(positive_definite{});
      out.insert(positive_semidefinite{}); // PD ⇒ PSD (#245 convention)
      detail::set_symmetric_unless_more_specific(this);
    }
  }
};

} // namespace numsim::cas

#endif // TENSOR_INV_H
