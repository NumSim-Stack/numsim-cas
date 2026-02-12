#ifndef TENSOR_DEVIATORIC_H
#define TENSOR_DEVIATORIC_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

class tensor_deviatoric final
    : public unary_op<tensor_node_base_t<tensor_deviatoric>> {
public:
  using base = unary_op<tensor_node_base_t<tensor_deviatoric>>;

  template <typename Expr>
  explicit tensor_deviatoric(Expr &&_expr)
      : base(std::forward<Expr>(_expr), _expr.get().dim(), _expr.get().rank()) {
  }
};

} // namespace numsim::cas

#endif // TENSOR_DEVIATORIC_H
