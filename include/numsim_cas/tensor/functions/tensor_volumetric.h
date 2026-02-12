#ifndef TENSOR_VOLUMETRIC_H
#define TENSOR_VOLUMETRIC_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

class tensor_volumetric final
    : public unary_op<tensor_node_base_t<tensor_volumetric>> {
public:
  using base = unary_op<tensor_node_base_t<tensor_volumetric>>;

  template <typename Expr>
  explicit tensor_volumetric(Expr &&_expr)
      : base(std::forward<Expr>(_expr), _expr.get().dim(), _expr.get().rank()) {
  }
};

} // namespace numsim::cas

#endif // TENSOR_VOLUMETRIC_H
