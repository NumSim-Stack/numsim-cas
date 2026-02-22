#ifndef TENSOR_INV_H
#define TENSOR_INV_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>
namespace numsim::cas {

class tensor_inv final : public unary_op<tensor_node_base_t<tensor_inv>> {
public:
  using base = unary_op<tensor_node_base_t<tensor_inv>>;

  template <typename Expr>
<<<<<<< HEAD
  explicit tensor_inv(
      Expr &&_expr) // NOLINT(bugprone-forwarding-reference-overload)
=======
  explicit tensor_inv(Expr &&_expr) // NOLINT(bugprone-forwarding-reference-overload)
>>>>>>> origin/move_to_virtual
      : base(std::forward<Expr>(_expr), _expr.get().dim(), _expr.get().rank()) {
    // inv preserves Symmetric, Volumetric, and Skew spaces.
    // Deviatoric/Harmonic are NOT preserved (tr(A^{-1}) ≠ 0 in general),
    // but the Symmetric perm is still valid — downgrade to Symmetric.
    if (auto const &sp = this->expr().get().space()) {
      if (std::holds_alternative<DeviatoricTag>(sp->trace) ||
          std::holds_alternative<HarmonicTag>(sp->trace))
        this->set_space({Symmetric{}, AnyTraceTag{}});
      else
        this->set_space(*sp);
    }
  }
};

} // namespace numsim::cas

#endif // TENSOR_INV_H
