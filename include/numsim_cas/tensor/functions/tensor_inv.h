#ifndef TENSOR_INV_H
#define TENSOR_INV_H

#include "../../unary_op.h"

namespace numsim::cas {

class tensor_inv final : public unary_op<tensor_inv, tensor_expression> {
public:
  using base = unary_op<tensor_inv, tensor_expression>;

  template <typename Expr>
  explicit tensor_inv(Expr &&_expr)
      : base(std::forward<Expr>(_expr), call_tensor::dim(_expr),
             call_tensor::rank(_expr)) {}
};

} // namespace numsim::cas

#endif // TENSOR_INV_H
