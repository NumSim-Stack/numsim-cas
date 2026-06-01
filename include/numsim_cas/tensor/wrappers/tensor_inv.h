#ifndef TENSOR_INV_H
#define TENSOR_INV_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>
namespace numsim::cas {

class tensor_inv final : public unary_op<tensor_node_base_t<tensor_inv>> {
public:
  using base = unary_op<tensor_node_base_t<tensor_inv>>;

  template <typename Expr>
  explicit tensor_inv(
      Expr &&_expr) // NOLINT(bugprone-forwarding-reference-overload)
      : base(std::forward<Expr>(_expr), _expr.get().dim(), _expr.get().rank()) {
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
  }
};

} // namespace numsim::cas

#endif // TENSOR_INV_H
