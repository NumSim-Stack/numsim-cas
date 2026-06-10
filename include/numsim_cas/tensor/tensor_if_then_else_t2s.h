#ifndef TENSOR_IF_THEN_ELSE_T2S_H
#define TENSOR_IF_THEN_ELSE_T2S_H

#include <numsim_cas/core/ternary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

/**
 * @class tensor_if_then_else_t2s
 * @brief Piecewise tensor selection with a TENSOR-TO-SCALAR condition:
 *        `cond != 0 ? then : else`, where `then` and `else` are tensors
 *        and `cond` is a tensor-to-scalar expression (#241).
 *
 * Sibling of `tensor_if_then_else_scalar` (scalar cond). Both have the
 * same shape — only the cond's domain differs. The two are kept as
 * separate concrete nodes (rather than collapsed via a "lift t2s to
 * scalar" wrapper) so the type system reflects the cond's actual
 * domain at construction.
 *
 * Used by the diff visitor for `tensor_to_scalar_if_then_else`
 * (#241): differentiating a t2s piecewise expression w.r.t. a tensor
 * yields a tensor piecewise expression whose condition is the
 * original t2s condition. Without this node, the diff visitor would
 * have to either throw (the pre-#241 mitigation) or silently use the
 * `then` branch as an approximation (the pre-mitigation silent-wrong
 * bug).
 *
 * Construction-time simplifications mirror the scalar-cond sibling
 * (applied in `if_then_else(t2s_cond, ...)` in `tensor_std.h`):
 * - `if_then_else(tensor_to_scalar_zero, a, b) → b`
 * - `if_then_else(tensor_to_scalar_one,  a, b) → a`
 * - `if_then_else(cond, a, a) → a`
 *
 * Differentiation conventions match the scalar-cond sibling:
 * `d/dA if_then_else(cond, X(A), Y(A)) = if_then_else(cond, dX/dA,
 * dY/dA)`. The cond's dependence on A produces Dirac contributions
 * at the boundary that are ignored — acceptable for piecewise
 * constitutive responses on a measure-zero switching set.
 *
 * Closes #241 (the AST bridge that made the diff rule expressible).
 */
class tensor_if_then_else_t2s final
    : public ternary_op<tensor_node_base_t<tensor_if_then_else_t2s>,
                        tensor_to_scalar_expression, tensor_expression,
                        tensor_expression> {
public:
  using base = ternary_op<tensor_node_base_t<tensor_if_then_else_t2s>,
                          tensor_to_scalar_expression, tensor_expression,
                          tensor_expression>;

  // Tensor-domain nodes need (dim, rank) propagated to the
  // tensor_node_base_t through their constructor. `then` and `else`
  // must agree on shape (the factory in `tensor_std.h` asserts this);
  // the constructor takes dim/rank from `then` and forwards them to
  // the base via the trailing `Args` that `ternary_op` forwards to
  // `ThisBase`. Mirrors `tensor_if_then_else_scalar`.
  template <typename ExprC, typename ExprT, typename ExprE>
  tensor_if_then_else_t2s(ExprC &&cond, ExprT &&then_branch,
                          ExprE &&else_branch)
      : base(std::forward<ExprC>(cond), std::forward<ExprT>(then_branch),
             std::forward<ExprE>(else_branch), then_branch.get().dim(),
             then_branch.get().rank()) {
    // Defensive shape check at the constructor boundary in addition to
    // the factory's assert: callers that bypass the `if_then_else(...)`
    // factory (e.g. direct
    // `make_expression<tensor_if_then_else_t2s>(...)`) would otherwise
    // build a node whose stored dim/rank comes from `then` while
    // `else` silently disagrees.
    //
    // Read from base's stored holders, not the constructor parameters —
    // by the time the body runs, base() has moved-from the forwarding
    // references and accessing them via `then_branch.get()` would hit
    // a null holder and throw.
    assert(this->expr_then().get().dim() == this->expr_else().get().dim());
    assert(this->expr_then().get().rank() == this->expr_else().get().rank());
  }

  tensor_if_then_else_t2s(tensor_if_then_else_t2s const &expr)
      : base(static_cast<base const &>(expr)) {}
  tensor_if_then_else_t2s(tensor_if_then_else_t2s &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  tensor_if_then_else_t2s() = delete;
  ~tensor_if_then_else_t2s() override = default;
  const tensor_if_then_else_t2s &operator=(tensor_if_then_else_t2s &&) = delete;
};

} // namespace numsim::cas

#endif // TENSOR_IF_THEN_ELSE_T2S_H
