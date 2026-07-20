#ifndef TENSOR_TO_SCALAR_DIVIDED_DIFFERENCE_H
#define TENSOR_TO_SCALAR_DIVIDED_DIFFERENCE_H

#include <algorithm>
#include <cstddef>
#include <vector>

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor/isotropic_kind.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

// [f; λ_{i0}, ..., λ_{ik}] — the divided difference of the scalar function
// `kind` over a multiset of eigenvalues of the wrapped symmetric rank-2
// tensor (#326). A scalar node. It is the building block of the symbolic
// spectral tangent of an isotropic tensor function:
//   ∂f(A)/∂A = Σ_{i,j} [f; λ_i, λ_j] (E_i ⊙ E_j).
// It evaluates safely at coincident eigenvalues (confluent divided
// difference → f^{(m)}/m!) and differentiates to a higher divided difference
//   ∂/∂λ_m [f; …] = [f; …, λ_m, λ_m, …],
// so the isotropic function is differentiable to arbitrary order. `indices`
// is stored sorted (the divided difference is symmetric in its points).
class tensor_to_scalar_divided_difference final
    : public unary_op<
          tensor_to_scalar_node_base_t<tensor_to_scalar_divided_difference>,
          tensor_expression> {
public:
  using base = unary_op<
      tensor_to_scalar_node_base_t<tensor_to_scalar_divided_difference>,
      tensor_expression>;

  template <typename E>
  tensor_to_scalar_divided_difference(E &&e, isotropic_kind kind,
                                      std::vector<std::size_t> indices)
      : base(std::forward<E>(e)), m_kind(kind), m_indices(std::move(indices)) {
    std::sort(m_indices.begin(), m_indices.end());
  }

  tensor_to_scalar_divided_difference(
      tensor_to_scalar_divided_difference const &e)
      : base(static_cast<base const &>(e)), m_kind(e.m_kind),
        m_indices(e.m_indices) {}
  tensor_to_scalar_divided_difference(
      tensor_to_scalar_divided_difference &&e) noexcept
      : base(std::move(static_cast<base &&>(e))), m_kind(e.m_kind),
        m_indices(std::move(e.m_indices)) {}
  tensor_to_scalar_divided_difference() = delete;
  ~tensor_to_scalar_divided_difference() override = default;
  const tensor_to_scalar_divided_difference &
  operator=(tensor_to_scalar_divided_difference &&) = delete;

  [[nodiscard]] isotropic_kind kind() const noexcept { return m_kind; }
  [[nodiscard]] std::vector<std::size_t> const &indices() const noexcept {
    return m_indices;
  }

protected:
  void update_hash_value() const noexcept override {
    this->m_hash_value = 0;
    hash_combine(this->m_hash_value, this->get_id());
    if (this->expr().is_valid())
      hash_combine(this->m_hash_value, this->expr().get().hash_value());
    hash_combine(this->m_hash_value, static_cast<std::size_t>(m_kind));
    for (std::size_t idx : m_indices)
      hash_combine(this->m_hash_value, idx);
  }

private:
  isotropic_kind m_kind;
  std::vector<std::size_t> m_indices;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_DIVIDED_DIFFERENCE_H
