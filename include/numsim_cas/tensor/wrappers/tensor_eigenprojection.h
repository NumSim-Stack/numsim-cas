#ifndef TENSOR_EIGENPROJECTION_H
#define TENSOR_EIGENPROJECTION_H

#include <cstddef>

#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor/tensor_assume.h>
#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

// #226 — the i-th eigenprojection E_i(A) = n_i ⊗ n_i of a symmetric rank-2
// tensor A, where n_i is the i-th (ascending) eigenvector. A rank-2
// symmetric tensor, kept opaque at the AST level and materialised at
// evaluation via tmech's eigenbasis. Unlike raw eigenvectors, E_i is
// unique (no ± sign freedom), which is exactly what the spectral
// derivative dλ_i/dA = E_i and isotropic functions f(A) = Σ f(λ_i) E_i
// (#227) consume. `index` is 0-based into the ascending eigenvalues.
class tensor_eigenprojection final
    : public unary_op<tensor_node_base_t<tensor_eigenprojection>> {
public:
  using base = unary_op<tensor_node_base_t<tensor_eigenprojection>>;

  template <typename Expr>
  tensor_eigenprojection(Expr &&_expr, std::size_t index)
      // Output is always rank 2 (E_i lives in the same space as A), with
      // the dimension of the input.
      : base(std::forward<Expr>(_expr), _expr.get().dim(), std::size_t{2}),
        m_index(index) {
    if (this->expr().get().rank() != 2)
      throw invalid_expression_error(
          "tensor_eigenprojection: input must be a rank-2 tensor");
    // E_i = n_i ⊗ n_i is symmetric by construction.
    this->set_space({Symmetric{}, AnyTraceTag{}});
  }

  tensor_eigenprojection(tensor_eigenprojection const &e)
      : base(static_cast<base const &>(e)), m_index(e.m_index) {}
  tensor_eigenprojection(tensor_eigenprojection &&e) noexcept
      : base(std::move(static_cast<base &&>(e))), m_index(e.m_index) {}
  tensor_eigenprojection() = delete;
  ~tensor_eigenprojection() override = default;
  const tensor_eigenprojection &operator=(tensor_eigenprojection &&) = delete;

  [[nodiscard]] std::size_t index() const noexcept { return m_index; }

protected:
  // Fold the index in: two eigenprojections of the same tensor differ
  // only by index (mirrors tensor_to_scalar_eigenvalue).
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

#endif // TENSOR_EIGENPROJECTION_H
