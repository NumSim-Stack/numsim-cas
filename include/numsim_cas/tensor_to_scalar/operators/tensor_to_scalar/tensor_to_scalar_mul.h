#ifndef TENSOR_TO_SCALAR_MUL_H
#define TENSOR_TO_SCALAR_MUL_H

#include "../../../n_ary_tree.h"
#include "../../../numsim_cas_type_traits.h"
#include "../../tensor_to_scalar_expression.h"

namespace numsim::cas {

class tensor_to_scalar_mul final
    : public n_ary_tree<tensor_to_scalar_expression, tensor_to_scalar_mul> {
public:
  using base = n_ary_tree<tensor_to_scalar_expression, tensor_to_scalar_mul>;
  using base::base;

  tensor_to_scalar_mul() : base() {}
  ~tensor_to_scalar_mul() = default;
  tensor_to_scalar_mul(tensor_to_scalar_mul const &data)
      : base(static_cast<base const &>(data)) {}
  tensor_to_scalar_mul(tensor_to_scalar_mul &&data)
      : base(std::forward<base>(data)) {}

  const tensor_to_scalar_mul &operator=(tensor_to_scalar_mul &&) = delete;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_MUL_H
