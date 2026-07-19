#ifndef TENSOR_ISOTROPIC_FUNCTION_TANGENT_H
#define TENSOR_ISOTROPIC_FUNCTION_TANGENT_H

#include <cstddef>

#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor/isotropic_kind.h>
#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

// ∂f(A)/∂A — the local fourth-order derivative of the isotropic tensor
// function f(A) w.r.t. A (#326). A rank-4 tensor evaluated by the
// coincidence-safe Daleckii–Krein formula (the divided-difference guard
// lives in the evaluator, so the tangent stays finite through eigenvalue
// coalescence). Opaque: its own differentiation is not implemented (higher
// spectral derivatives); it composes by evaluation + contraction.
class tensor_isotropic_function_tangent final
    : public unary_op<tensor_node_base_t<tensor_isotropic_function_tangent>> {
public:
  using base = unary_op<tensor_node_base_t<tensor_isotropic_function_tangent>>;

  template <typename Expr>
  tensor_isotropic_function_tangent(Expr &&e, isotropic_kind kind)
      : base(std::forward<Expr>(e), e.get().dim(), std::size_t{4}),
        m_kind(kind) {
    if (this->expr().get().rank() != 2)
      throw invalid_expression_error(
          "tensor_isotropic_function_tangent: input must be a rank-2 tensor");
  }

  tensor_isotropic_function_tangent(tensor_isotropic_function_tangent const &e)
      : base(static_cast<base const &>(e)), m_kind(e.m_kind) {}
  tensor_isotropic_function_tangent(
      tensor_isotropic_function_tangent &&e) noexcept
      : base(std::move(static_cast<base &&>(e))), m_kind(e.m_kind) {}
  tensor_isotropic_function_tangent() = delete;
  ~tensor_isotropic_function_tangent() override = default;
  const tensor_isotropic_function_tangent &
  operator=(tensor_isotropic_function_tangent &&) = delete;

  [[nodiscard]] isotropic_kind kind() const noexcept { return m_kind; }

protected:
  void update_hash_value() const noexcept override {
    this->m_hash_value = 0;
    hash_combine(this->m_hash_value, this->get_id());
    if (this->expr().is_valid())
      hash_combine(this->m_hash_value, this->expr().get().hash_value());
    hash_combine(this->m_hash_value, static_cast<std::size_t>(m_kind));
  }

private:
  isotropic_kind m_kind;
};

} // namespace numsim::cas

#endif // TENSOR_ISOTROPIC_FUNCTION_TANGENT_H
