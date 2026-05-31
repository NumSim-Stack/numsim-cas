#ifndef TENSOR_IF_THEN_ELSE_H
#define TENSOR_IF_THEN_ELSE_H

#include <numsim_cas/core/ternary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

/**
 * @class tensor_if_then_else
 * @brief Piecewise tensor selection with a SCALAR condition:
 *        `cond != 0 ? then : else`, where `then` and `else` are tensors.
 *
 * Mixed-base variant of the if_then_else family. The condition lives
 * in the scalar domain (typically a comparison-indicator from #136 or
 * any scalar evaluating to 0/1 at runtime); the branches live in the
 * tensor domain.
 *
 * Used for piecewise tensor-valued constitutive responses:
 * - Damage-activation tensors: stiffness selects between elastic
 *   `C` and damaged `D*C` based on a yield-function scalar.
 * - Contact: contact stress tensor selects between zero and the
 *   penalty stiffness contribution based on the scalar gap sign.
 *
 * Construction-time simplifications (applied in
 * `if_then_else(...)` in `tensor_std.h`):
 * - `if_then_else(scalar_zero, a, b) → b`
 * - `if_then_else(scalar_one, a, b)  → a`
 * - `if_then_else(scalar_constant{c}, a, b) → a if c != 0 else b`
 * - `if_then_else(cond, a, a) → a` (then and else identical)
 *
 * Differentiation w.r.t. a tensor variable mirrors the same-domain
 * variants: `d/dA if_then_else(cond, X(A), Y(A)) = if_then_else(cond,
 * dX/dA, dY/dA)` — assumes cond doesn't depend on A. Conditions that
 * do depend on A would technically produce Dirac contributions at the
 * boundary; ignoring those is acceptable for the constitutive-model
 * use cases (yield-function / contact-gap on a measure-zero set).
 *
 * Closes the tensor half of #210; closes #135.
 */
class tensor_if_then_else final
    : public ternary_op<tensor_node_base_t<tensor_if_then_else>,
                        scalar_expression, tensor_expression,
                        tensor_expression> {
public:
  using base =
      ternary_op<tensor_node_base_t<tensor_if_then_else>, scalar_expression,
                 tensor_expression, tensor_expression>;

  // Tensor-domain nodes need (dim, rank) propagated to the
  // tensor_node_base_t through their constructor. `then` and `else`
  // must agree on shape (the factory in `tensor_std.h` asserts this);
  // the constructor takes dim/rank from `then` and forwards them to
  // the base via the trailing `Args` that `ternary_op` forwards to
  // `ThisBase`.
  template <typename ExprC, typename ExprT, typename ExprE>
  tensor_if_then_else(ExprC &&cond, ExprT &&then_branch, ExprE &&else_branch)
      : base(std::forward<ExprC>(cond), std::forward<ExprT>(then_branch),
             std::forward<ExprE>(else_branch), then_branch.get().dim(),
             then_branch.get().rank()) {
    // Defensive shape check at the constructor boundary in addition to
    // the factory's assert: callers that bypass the `if_then_else(...)`
    // factory (e.g. direct `make_expression<tensor_if_then_else>(...)`)
    // would otherwise build a node whose stored dim/rank comes from
    // `then` while `else` silently disagrees.
    //
    // Read from base's stored holders, not the constructor parameters —
    // by the time the body runs, base() has moved-from the forwarding
    // references and accessing them via `then_branch.get()` would hit
    // a null holder and throw.
    assert(this->expr_then().get().dim() == this->expr_else().get().dim());
    assert(this->expr_then().get().rank() == this->expr_else().get().rank());
  }

  tensor_if_then_else(tensor_if_then_else const &expr)
      : base(static_cast<base const &>(expr)) {}
  tensor_if_then_else(tensor_if_then_else &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  tensor_if_then_else() = delete;
  ~tensor_if_then_else() override = default;
  const tensor_if_then_else &operator=(tensor_if_then_else &&) = delete;
};

} // namespace numsim::cas

#endif // TENSOR_IF_THEN_ELSE_H
