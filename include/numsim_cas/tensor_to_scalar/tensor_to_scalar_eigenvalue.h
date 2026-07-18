#ifndef TENSOR_TO_SCALAR_EIGENVALUE_H
#define TENSOR_TO_SCALAR_EIGENVALUE_H

#include <cstddef>

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

// #226 — the i-th eigenvalue of a symmetric rank-2 tensor A. Stays an
// opaque scalar at the AST level (materialised only at evaluation, via
// tmech's eigendecomposition), so the symbolic chain rule survives.
// `index` is 0-based into the ascending-sorted eigenvalues.
class tensor_to_scalar_eigenvalue final
    : public unary_op<tensor_to_scalar_node_base_t<tensor_to_scalar_eigenvalue>,
                      tensor_expression> {
public:
  using base =
      unary_op<tensor_to_scalar_node_base_t<tensor_to_scalar_eigenvalue>,
               tensor_expression>;

  template <typename E>
  tensor_to_scalar_eigenvalue(E &&e, std::size_t index)
      : base(std::forward<E>(e)), m_index(index) {}

  tensor_to_scalar_eigenvalue(tensor_to_scalar_eigenvalue const &e)
      : base(static_cast<base const &>(e)), m_index(e.m_index) {}
  tensor_to_scalar_eigenvalue(tensor_to_scalar_eigenvalue &&e) noexcept
      : base(std::move(static_cast<base &&>(e))), m_index(e.m_index) {}
  tensor_to_scalar_eigenvalue() = delete;
  ~tensor_to_scalar_eigenvalue() override = default;
  const tensor_to_scalar_eigenvalue &
  operator=(tensor_to_scalar_eigenvalue &&) = delete;

  [[nodiscard]] std::size_t index() const noexcept { return m_index; }

protected:
  // Fold the index in (mirrors inner_product_wrapper #266): two
  // eigenvalues of the same tensor differ only by index.
  void update_hash_value() const noexcept override {
    this->m_hash_value = 0;
    hash_combine(this->m_hash_value, this->get_id());
    if (this->expr().is_valid())
      hash_combine(this->m_hash_value, this->expr().get().hash_value());
    hash_combine(this->m_hash_value, m_index);
  }

private:
  std::size_t m_index;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_EIGENVALUE_H
