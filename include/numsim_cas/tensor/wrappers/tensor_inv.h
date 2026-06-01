#ifndef TENSOR_INV_H
#define TENSOR_INV_H

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
    // inv(PSD) propagates PSD: at runtime the eval throws if actually
    // singular, so any reaching result has finite eigenvalues — but
    // we can't tell PD-vs-PSD symbolically without more info, so stay
    // conservative and propagate exactly what the user asserted.
    //
    // PD/PSD also imply symmetric. Reuse the existing detail helper
    // to set {Symmetric, AnyTraceTag} on the space iff there's no
    // strictly more specific Sym subspace (Vol / Dev) — same rule the
    // assume_positive_definite factory uses.
    //
    // Orthogonal is NOT propagated here. At rank-2 the inv() factory
    // folds inv(orthogonal) to trans() before reaching this ctor; at
    // rank-4 "orthogonal" isn't a standard concept.
    auto const &input_alg = this->expr().get().tensor_algebra_assumptions();
    bool propagate_pd = input_alg.contains(positive_definite{});
    bool propagate_psd = input_alg.contains(positive_semidefinite{});
    if (propagate_pd || propagate_psd) {
      auto &out = this->tensor_algebra_assumptions();
      if (propagate_pd) {
        out.insert(positive_definite{});
        out.insert(
            positive_semidefinite{}); // PD => PSD (matches #245 convention)
      } else {
        out.insert(positive_semidefinite{});
      }
      detail::set_symmetric_unless_more_specific(this);
    }
  }
};

} // namespace numsim::cas

#endif // TENSOR_INV_H
