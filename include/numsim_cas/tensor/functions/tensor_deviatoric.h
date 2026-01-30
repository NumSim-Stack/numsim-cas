#ifndef TENSOR_DEVIATORIC_H
#define TENSOR_DEVIATORIC_H

#include "../../unary_op.h"
#include <vector>

namespace numsim::cas {

class tensor_deviatoric final
    : public unary_op<tensor_deviatoric, tensor_expression> {
public:
  using base = unary_op<tensor_deviatoric, tensor_expression>;

  template <typename Expr>
  explicit tensor_deviatoric(Expr &&_expr)
      : base(std::forward<Expr>(_expr), call_tensor::dim(_expr),
             call_tensor::rank(_expr)) {}
};

} // namespace numsim::cas

#endif // TENSOR_DEVIATORIC_H
