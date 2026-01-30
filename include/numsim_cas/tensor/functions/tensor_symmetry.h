#ifndef TENSOR_SYMMETRY_H
#define TENSOR_SYMMETRY_H

#include "../../unary_op.h"
#include <vector>

namespace numsim::cas {

class tensor_symmetry final
    : public unary_op<tensor_symmetry, tensor_expression> {
public:
  using base = unary_op<tensor_symmetry, tensor_expression>;

  template <typename Expr>
  explicit tensor_symmetry(Expr &&_expr)
      : base(std::forward<Expr>(_expr), call_tensor::dim(_expr),
             call_tensor::rank(_expr)),
        m_symmetries() {}

  template <typename Expr, typename Indices>
  tensor_symmetry(Expr &&_expr, Indices &&_indices)
      : base(std::forward<Expr>(_expr), call_tensor::dim(_expr),
             call_tensor::rank(_expr)),
        m_symmetries(std::forward<Indices>(_indices)) {}

  auto const &symmetries() const { return m_symmetries; }

  auto &symmetries() { return m_symmetries; }

private:
  std::vector<std::vector<std::size_t>> m_symmetries;
};

} // namespace numsim::cas

#endif // TENSOR_SYMMETRY_H
