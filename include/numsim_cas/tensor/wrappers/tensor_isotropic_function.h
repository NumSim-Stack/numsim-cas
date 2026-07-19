#ifndef TENSOR_ISOTROPIC_FUNCTION_H
#define TENSOR_ISOTROPIC_FUNCTION_H

#include <cstddef>

#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor/isotropic_kind.h>
#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

// f(A) = Σ f(λ_i) E_i for a symmetric rank-2 tensor A (#227): the scalar
// function `kind` applied spectrally. A rank-2 symmetric tensor, opaque at
// the AST level (materialised at evaluation via the spectral decomposition).
// Being a node — not a raw composition — lets differentiation dispatch to
// the coincidence-safe Daleckii–Krein tangent (tensor_isotropic_function_
// tangent), which the bare composition cannot express (#326).
class tensor_isotropic_function final
    : public unary_op<tensor_node_base_t<tensor_isotropic_function>> {
public:
  using base = unary_op<tensor_node_base_t<tensor_isotropic_function>>;

  template <typename Expr>
  tensor_isotropic_function(Expr &&e, isotropic_kind kind)
      : base(std::forward<Expr>(e), e.get().dim(), std::size_t{2}),
        m_kind(kind) {
    if (this->expr().get().rank() != 2)
      throw invalid_expression_error(
          "tensor_isotropic_function: input must be a rank-2 tensor");
    // f(A) is symmetric when A is (it is evaluated on sym(A)).
    this->set_space({Symmetric{}, AnyTraceTag{}});
  }

  tensor_isotropic_function(tensor_isotropic_function const &e)
      : base(static_cast<base const &>(e)), m_kind(e.m_kind) {}
  tensor_isotropic_function(tensor_isotropic_function &&e) noexcept
      : base(std::move(static_cast<base &&>(e))), m_kind(e.m_kind) {}
  tensor_isotropic_function() = delete;
  ~tensor_isotropic_function() override = default;
  const tensor_isotropic_function &
  operator=(tensor_isotropic_function &&) = delete;

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

#endif // TENSOR_ISOTROPIC_FUNCTION_H
