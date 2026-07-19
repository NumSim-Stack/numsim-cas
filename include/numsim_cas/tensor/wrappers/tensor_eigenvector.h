#ifndef TENSOR_EIGENVECTOR_H
#define TENSOR_EIGENVECTOR_H

#include <cstddef>

#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

// #226 — the i-th eigenvector (unit normal direction) n_i of a symmetric
// rank-2 tensor A, ordered by ascending eigenvalue. A rank-1 tensor, kept
// opaque at the AST level and materialised at evaluation via tmech's
// eigendecomposition.
//
// Sign caveat: an eigenvector is only defined up to sign (±n_i span the
// same eigenline). Evaluation uses tmech's deterministic convention, but
// the sign is not a stable symbolic invariant — the eigenprojection
// E_i = n_i ⊗ n_i (see eigen_decomposition::basis) is the sign-free
// quantity and should be preferred for differentiation.
class tensor_eigenvector final
    : public unary_op<tensor_node_base_t<tensor_eigenvector>> {
public:
  using base = unary_op<tensor_node_base_t<tensor_eigenvector>>;

  template <typename Expr>
  tensor_eigenvector(Expr &&_expr, std::size_t index)
      // Output is a rank-1 tensor of the input's dimension.
      : base(std::forward<Expr>(_expr), _expr.get().dim(), std::size_t{1}),
        m_index(index) {
    if (this->expr().get().rank() != 2)
      throw invalid_expression_error(
          "tensor_eigenvector: input must be a rank-2 tensor");
  }

  tensor_eigenvector(tensor_eigenvector const &e)
      : base(static_cast<base const &>(e)), m_index(e.m_index) {}
  tensor_eigenvector(tensor_eigenvector &&e) noexcept
      : base(std::move(static_cast<base &&>(e))), m_index(e.m_index) {}
  tensor_eigenvector() = delete;
  ~tensor_eigenvector() override = default;
  const tensor_eigenvector &operator=(tensor_eigenvector &&) = delete;

  [[nodiscard]] std::size_t index() const noexcept { return m_index; }

protected:
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

#endif // TENSOR_EIGENVECTOR_H
