#ifndef TENSOR_SYMMETRY_H
#define TENSOR_SYMMETRY_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <vector>

namespace numsim::cas {

class tensor_symmetry final
    : public unary_op<tensor_node_base_t<tensor_symmetry>> {
public:
  using base = unary_op<tensor_node_base_t<tensor_symmetry>>;

  template <typename Expr>
  explicit tensor_symmetry(Expr &&_expr)
      : base(std::forward<Expr>(_expr), _expr.get().dim(), _expr.get().rank()),
        m_symmetries() {}

  template <typename Expr, typename Indices>
  tensor_symmetry(Expr &&_expr, Indices &&_indices)
      : base(std::forward<Expr>(_expr)),
        m_symmetries(std::forward<Indices>(_indices)) {}

  auto const &symmetries() const { return m_symmetries; }

  auto &symmetries() { return m_symmetries; }

private:
  std::vector<std::vector<std::size_t>> m_symmetries;
};

} // namespace numsim::cas

#endif // TENSOR_SYMMETRY_H
