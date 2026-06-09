#ifndef TENSOR_DIFFERENTIATION_WRT_SCALAR_H
#define TENSOR_DIFFERENTIATION_WRT_SCALAR_H

// Differentiates a tensor expression with respect to a SCALAR variable.
// Companion to tensor_differentiation (which differentiates w.r.t. a
// tensor variable). Both inherit from tensor_visitor_const_t.
//
// Result rank equals input rank — no rank increase, no identity-tensor
// insertion, no m_rank_arg bookkeeping. The reason: a tensor depends
// on a scalar only through embedded scalar coefficients; the
// derivative carries the same indices as the original tensor.
//
// Leaf rule: a bare `tensor` symbol never equals a scalar arg, so its
// derivative w.r.t. any scalar is `tensor_zero`. Cross-domain nodes
// (tensor_scalar_mul, tensor_mul, tensor_pow, etc.) reinstate the
// scalar product-rule term that the tensor-arg visitor drops as
// "scalar is constant w.r.t. tensor arg". See issue #275.

#include <numeric>
#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/core/diff.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/scalar/scalar_diff.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/tensor/identity_tensor.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <ranges>

namespace numsim::cas {

// Forward declare the CPO we ARE implementing (the issue's main goal).
expression_holder<tensor_expression>
tag_invoke(detail::diff_fn, std::type_identity<tensor_expression>,
           std::type_identity<scalar_expression>,
           expression_holder<tensor_expression> const &,
           expression_holder<scalar_expression> const &);

class tensor_differentiation_wrt_scalar final : public tensor_visitor_const_t {
public:
  using tensor_holder_t = expression_holder<tensor_expression>;
  using scalar_holder_t = expression_holder<scalar_expression>;
  using t2s_holder_t = expression_holder<tensor_to_scalar_expression>;

  explicit tensor_differentiation_wrt_scalar(scalar_holder_t const &arg)
      : m_arg(arg) {}

  tensor_differentiation_wrt_scalar(tensor_differentiation_wrt_scalar const &) =
      delete;
  tensor_differentiation_wrt_scalar(tensor_differentiation_wrt_scalar &&) =
      delete;
  const tensor_differentiation_wrt_scalar &
  operator=(tensor_differentiation_wrt_scalar const &) = delete;

  [[nodiscard]] tensor_holder_t apply(tensor_holder_t const &expr) {
    m_result = tensor_holder_t{};
    if (expr.is_valid()) {
      m_dim = expr.get().dim();
      m_rank_result = expr.get().rank();
      expr.get<tensor_visitable_t>().accept(*this);
    }
    if (!m_result.is_valid()) {
      return make_expression<tensor_zero>(m_dim, m_rank_result);
    }
    return m_result;
  }

  // --- Leaf nodes: all return zero (handled by apply's fallback) ---

  // A bare tensor variable does not depend on a scalar arg.
  void operator()(tensor const &) override {}
  void operator()(tensor_zero const &) override {}
  void operator()(identity_tensor const &) override {}
  void operator()(levi_civita_tensor const &) override {}
  void operator()(tensor_projector const &) override {}

  // --- Linear operators: derivative passes through ---

  void operator()(tensor_add const &visitable) override {
    tensor_holder_t sum;
    for (auto &child : visitable.symbol_map() | std::views::values) {
      auto d = diff(child, m_arg);
      // Pass-1 review: suppress canonical tensor_zero so trivial
      // children don't inflate the printed sum. Mirrors the pattern
      // in tensor_scalar_mul / tensor_mul.
      if (d.is_valid() && !is_same<tensor_zero>(d)) {
        sum += d;
      }
    }
    m_result = std::move(sum);
  }

  void operator()(tensor_negative const &visitable) override {
    auto d = diff(visitable.expr(), m_arg);
    if (d.is_valid() && !is_same<tensor_zero>(d)) {
      m_result = -d;
    }
  }

  // d/ds if_then_else(cond, X(s), Y(s)) = if_then_else(cond, dX/ds, dY/ds)
  // when cond doesn't depend on s. Same lazy-eval-vs-eager-diff
  // asymmetry as the tensor-arg and scalar-arg variants.
  void operator()(tensor_if_then_else const &visitable) override {
    auto dt = diff(visitable.expr_then(), m_arg);
    auto de = diff(visitable.expr_else(), m_arg);
    if (dt.is_valid() && de.is_valid()) {
      m_result =
          if_then_else(visitable.expr_cond(), std::move(dt), std::move(de));
    }
  }

  // (s * T)' = s' * T + s * T' — THE product rule the tensor-arg
  // visitor drops. Closes the central piece of #275.
  void operator()(tensor_scalar_mul const &visitable) override {
    auto const &s = visitable.expr_lhs();
    auto const &T = visitable.expr_rhs();
    auto ds = diff(s, m_arg); // scalar derivative — already supported
    auto dT = diff(T, m_arg); // tensor derivative — this visitor (recursive)
    tensor_holder_t sum;
    if (ds.is_valid() && !is_same<scalar_zero>(ds)) {
      sum = ds * T;
    }
    if (dT.is_valid() && !is_same<tensor_zero>(dT)) {
      if (sum.is_valid()) {
        sum += s * dT;
      } else {
        sum = s * dT;
      }
    }
    m_result = std::move(sum);
  }

  void operator()(permute_indices_wrapper const &visitable) override {
    // d/ds permute_indices(A, perm) = permute_indices(dA/ds, perm).
    // Same permutation: scalar arg adds no rank, so the existing
    // index sequence applies unchanged to the derivative.
    auto dA = diff(visitable.expr(), m_arg);
    if (dA.is_valid() && !is_same<tensor_zero>(dA)) {
      auto indices_copy = visitable.indices();
      m_result = permute_indices(std::move(dA), std::move(indices_copy));
    }
  }

  // --- Cross-domain / complex nodes: declared here, defined in .cpp ---

  void operator()(tensor_pow const &visitable) override;
  void operator()(tensor_mul const &visitable) override;
  void operator()(simple_outer_product const &visitable) override;
  void operator()(tensor_inv const &visitable) override;
  void operator()(inner_product_wrapper const &visitable) override;
  void operator()(outer_product_wrapper const &visitable) override;
  void operator()(tensor_to_scalar_with_tensor_mul const &visitable) override;

private:
  scalar_holder_t const &m_arg;
  std::size_t m_dim{0};
  std::size_t m_rank_result{0};
  tensor_holder_t m_result;
};

} // namespace numsim::cas
#endif // TENSOR_DIFFERENTIATION_WRT_SCALAR_H
