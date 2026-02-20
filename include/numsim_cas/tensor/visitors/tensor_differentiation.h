#ifndef TENSOR_DIFFERENTIATION_H
#define TENSOR_DIFFERENTIATION_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/core/diff.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/tensor/kronecker_delta.h>
#include <numsim_cas/tensor/projection_tensor.h>
#include <numsim_cas/tensor/projector_algebra.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numeric>
#include <ranges>

namespace numsim::cas {

// Forward declare the diff CPOs used recursively
expression_holder<tensor_expression>
tag_invoke(detail::diff_fn, std::type_identity<tensor_expression>,
           std::type_identity<tensor_expression>,
           expression_holder<tensor_expression> const &,
           expression_holder<tensor_expression> const &);

expression_holder<tensor_expression>
tag_invoke(detail::diff_fn, std::type_identity<tensor_to_scalar_expression>,
           std::type_identity<tensor_expression>,
           expression_holder<tensor_to_scalar_expression> const &,
           expression_holder<tensor_expression> const &);

} // namespace numsim::cas

namespace numsim::cas {

class tensor_differentiation final : public tensor_visitor_const_t {
public:
  using tensor_holder_t = expression_holder<tensor_expression>;
  using t2s_holder_t = expression_holder<tensor_to_scalar_expression>;
  using scalar_holder_t = expression_holder<scalar_expression>;

  explicit tensor_differentiation(tensor_holder_t const &arg) : m_arg(arg) {
    m_dim = arg.get().dim();
    m_rank_arg = arg.get().rank();
    m_I = make_expression<kronecker_delta>(m_dim);
  }

  tensor_differentiation(tensor_differentiation const &) = delete;
  tensor_differentiation(tensor_differentiation &&) = delete;
  const tensor_differentiation &
  operator=(tensor_differentiation const &) = delete;

  [[nodiscard]] tensor_holder_t apply(tensor_holder_t const &expr) {
    m_result = tensor_holder_t{};
    if (expr.is_valid()) {
      m_expr = expr;
      m_rank_result = expr.get().rank() + m_rank_arg;
      expr.get<tensor_visitable_t>().accept(*this);
    }
    if (!m_result.is_valid()) {
      return make_expression<tensor_zero>(m_dim, m_rank_result);
    }
    return m_result;
  }

  // --- Simple nodes defined in header ---

  void operator()(tensor const &visitable) override {
    if (visitable.hash_value() == m_arg.get().hash_value()) {
      if (m_rank_arg == 2) {
        if (auto const &sp = m_arg.get().space()) {
          auto kind = classify_space(*sp);
          if (kind != ProjKind::Other) {
            auto d = m_arg.get().dim();
            switch (kind) {
            case ProjKind::Sym:  m_result = P_sym(d);  return;
            case ProjKind::Skew: m_result = P_skew(d); return;
            case ProjKind::Vol:  m_result = P_vol(d);  return;
            case ProjKind::Dev:  m_result = P_devi(d); return;
            default: break;
            }
          }
        }
      }
      m_result = make_expression<identity_tensor>(m_dim, m_rank_result);
    }
    // else m_result stays invalid -> zero
  }

  void operator()(tensor_zero const &) override {
    // derivative of zero is zero (handled by apply returning zero)
  }

  void operator()(kronecker_delta const &) override {
    // constant -> zero
  }

  void operator()(identity_tensor const &) override {
    // constant -> zero
  }

  void operator()(tensor_projector const &) override {
    // constant -> zero
  }

  void operator()(tensor_add const &visitable) override {
    tensor_holder_t sum;
    for (auto &child : visitable.hash_map() | std::views::values) {
      auto d = diff(child, m_arg);
      if (d.is_valid()) {
        sum += d;
      }
    }
    m_result = std::move(sum);
  }

  void operator()(tensor_negative const &visitable) override {
    auto d = diff(visitable.expr(), m_arg);
    if (d.is_valid()) {
      m_result = -d;
    }
  }

  void operator()(tensor_scalar_mul const &visitable) override {
    // (scalar * tensor)' = scalar * tensor' (scalar is constant w.r.t. tensor
    // arg)
    auto dt = diff(visitable.expr_rhs(), m_arg);
    if (dt.is_valid()) {
      m_result = visitable.expr_lhs() * dt;
    }
  }

  void operator()(tensor_pow const &visitable) override {
    // d(pow(A,n))/dX creates a tensor_power_diff node
    m_result = make_expression<tensor_power_diff>(visitable.expr_lhs(),
                                                   visitable.expr_rhs());
  }

  // --- Cross-domain / complex nodes: declared here, defined in .cpp ---

  void operator()(tensor_power_diff const &visitable) override;
  void operator()(tensor_mul const &visitable) override;
  void operator()(simple_outer_product const &visitable) override;
  void operator()(tensor_inv const &visitable) override;
  void operator()(inner_product_wrapper const &visitable) override;
  void operator()(basis_change_imp const &visitable) override;
  void operator()(outer_product_wrapper const &visitable) override;
  void operator()(tensor_to_scalar_with_tensor_mul const &visitable) override;

private:
  tensor_holder_t const &m_arg;
  std::size_t m_dim{0};
  std::size_t m_rank_result{0};
  std::size_t m_rank_arg{0};
  tensor_holder_t m_result;
  tensor_holder_t m_expr;
  tensor_holder_t m_I;
};

} // namespace numsim::cas
#endif // TENSOR_DIFFERENTIATION_H
