#ifndef TENSOR_VOLUMETRIC_H
#define TENSOR_VOLUMETRIC_H

#include "../../unary_op.h"
#include <vector>

namespace numsim::cas {

template <typename ValueType>
class tensor_volumetric final : public unary_op<tensor_volumetric<ValueType>,
                                                tensor_expression<ValueType>> {
public:
  using base =
      unary_op<tensor_volumetric<ValueType>, tensor_expression<ValueType>>;

  template <typename Expr>
  explicit tensor_volumetric(Expr &&_expr)
      : base(std::forward<Expr>(_expr), call_tensor::dim(_expr),
             call_tensor::rank(_expr)) {}
};

} // namespace numsim::cas

#endif // TENSOR_VOLUMETRIC_H
