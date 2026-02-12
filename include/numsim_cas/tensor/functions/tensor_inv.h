#ifndef TENSOR_INV_H
#define TENSOR_INV_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>
namespace numsim::cas {

class tensor_inv final : public unary_op<tensor_node_base_t<tensor_inv>> {
public:
  using base = unary_op<tensor_node_base_t<tensor_inv>>;

  template <typename Expr>
  explicit tensor_inv(Expr &&_expr)
      : base(std::forward<Expr>(_expr), _expr.get().dim(), _expr.get().rank()) {
  }
};

} // namespace numsim::cas

#endif // TENSOR_INV_H
