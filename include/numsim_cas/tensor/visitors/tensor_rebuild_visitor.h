#ifndef TENSOR_REBUILD_VISITOR_H
#define TENSOR_REBUILD_VISITOR_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/eigen_decomposition.h>
#include <numsim_cas/tensor/projector_algebra.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <ranges>

namespace numsim::cas {

class tensor_rebuild_visitor : public tensor_visitor_const_t {
public:
  using tensor_holder_t = expression_holder<tensor_expression>;
  using scalar_holder_t = expression_holder<scalar_expression>;
  using t2s_holder_t = expression_holder<tensor_to_scalar_expression>;

  ~tensor_rebuild_visitor() override = default;

  virtual tensor_holder_t apply(tensor_holder_t const &expr) {
    if (expr.is_valid()) {
      m_current = expr;
      expr.get<tensor_visitable_t>().accept(*this);
      // #93 — reconstructions below build fresh nodes (variadic ctors)
      // that drop post-construction space(). Restore from source, but not
      // over a self-computed space (e.g. tensor_add's child-join), and only
      // for structurally unchanged rebuilds: a subclass that swapped
      // children (substitution) must not inherit the source's space (#352).
      if (m_result.is_valid() && !m_result.get().space()) {
        if (auto const &sp = expr.get().space()) {
          if (m_result == expr || same_projector_contraction(expr, m_result))
            m_result.data()->set_space(*sp);
        }
      }
      return std::move(m_result);
    }
    return expr;
  }

  virtual scalar_holder_t apply_scalar(scalar_holder_t const &expr) {
    return expr;
  }

  virtual t2s_holder_t apply_t2s(t2s_holder_t const &expr) { return expr; }

  // skew(X)/sym(X)/...: the space comes from the projector, not the
  // argument, so it survives child substitution (#93/#352).
  static bool same_projector_contraction(tensor_holder_t const &a,
                                         tensor_holder_t const &b) {
    if (a.get().rank() != b.get().rank() || a.get().dim() != b.get().dim()) {
      return false; // review on #352: never restore across a shape change
    }
    auto ia = as_projector_contraction(a);
    auto ib = as_projector_contraction(b);
    return ia && ib && *ia->proj == *ib->proj;
  }

  // Leaf nodes: return as-is
  void operator()(tensor const &) override { m_result = m_current; }
  void operator()(tensor_zero const &) override { m_result = m_current; }
  void operator()(identity_tensor const &) override { m_result = m_current; }
  void operator()(levi_civita_tensor const &) override { m_result = m_current; }
  void operator()(tensor_projector const &) override { m_result = m_current; }

  // ─── if_then_else (#135 / #210) ─────────────────────────────────
  // Cond is a scalar; routes through apply_scalar for substitution
  // visitor compatibility. Branches are tensors via apply.
  void operator()(tensor_if_then_else_scalar const &v) override {
    m_result = if_then_else(apply_scalar(v.expr_cond()), apply(v.expr_then()),
                            apply(v.expr_else()));
  }

  // ─── if_then_else_t2s (#241) ────────────────────────────────────
  // Sibling of the scalar-cond version. Cond is t2s; routes through
  // apply_t2s.
  void operator()(tensor_if_then_else_t2s const &v) override {
    m_result = if_then_else(apply_t2s(v.expr_cond()), apply(v.expr_then()),
                            apply(v.expr_else()));
  }

  // Unary tensor -> tensor
  void operator()(tensor_negative const &v) override {
    m_result = -apply(v.expr());
  }

  void operator()(tensor_inv const &v) override {
    m_result = inv(apply(v.expr()));
  }

  void operator()(tensor_eigenprojection const &v) override {
    m_result = eigen_decomposition(apply(v.expr())).basis(v.index());
  }

  void operator()(tensor_eigenvector const &v) override {
    m_result = eigen_decomposition(apply(v.expr())).normal(v.index());
  }

  void operator()(tensor_isotropic_function const &v) override {
    m_result =
        make_expression<tensor_isotropic_function>(apply(v.expr()), v.kind());
  }

  void operator()(permute_indices_wrapper const &v) override {
    m_result =
        make_expression<permute_indices_wrapper>(apply(v.expr()), v.indices());
  }

  // Binary tensor x tensor -> tensor
  void operator()(inner_product_wrapper const &v) override {
    m_result = make_expression<inner_product_wrapper>(
        apply(v.expr_lhs()), v.indices_lhs(), apply(v.expr_rhs()),
        v.indices_rhs());
  }

  void operator()(outer_product_wrapper const &v) override {
    m_result = make_expression<outer_product_wrapper>(
        apply(v.expr_lhs()), sequence(v.indices_lhs()), apply(v.expr_rhs()),
        sequence(v.indices_rhs()));
  }

  // Binary tensor x scalar -> tensor (cross-domain!)
  void operator()(tensor_pow const &v) override {
    m_result = make_expression<tensor_pow>(apply(v.expr_lhs()),
                                           apply_scalar(v.expr_rhs()));
  }

  // Binary scalar x tensor -> tensor (cross-domain!)
  void operator()(tensor_scalar_mul const &v) override {
    m_result = apply_scalar(v.expr_lhs()) * apply(v.expr_rhs());
  }

  // Binary tensor x t2s -> tensor (cross-domain!)
  void operator()(tensor_to_scalar_with_tensor_mul const &v) override {
    m_result = make_expression<tensor_to_scalar_with_tensor_mul>(
        apply(v.expr_lhs()), apply_t2s(v.expr_rhs()));
  }

  // N-ary tensor ops
  void operator()(tensor_add const &v) override {
    tensor_holder_t result;
    if (v.coeff().is_valid())
      result += apply(v.coeff());
    for (auto &child : v.symbol_map() | std::views::values)
      result += apply(child);
    m_result = std::move(result);
  }

  void operator()(tensor_mul const &v) override {
    tensor_holder_t result;
    if (v.coeff().is_valid())
      result *= apply(v.coeff());
    for (auto &child : v.data())
      result *= apply(child);
    m_result = std::move(result);
  }

  void operator()(simple_outer_product const &v) override {
    auto rebuilt = make_expression<simple_outer_product>(v.dim(), v.rank());
    for (auto &child : v.data())
      rebuilt.template get<simple_outer_product>().push_back(apply(child));
    m_result = std::move(rebuilt);
  }

protected:
  tensor_holder_t m_current;
  tensor_holder_t m_result;
};

} // namespace numsim::cas

#endif // TENSOR_REBUILD_VISITOR_H
