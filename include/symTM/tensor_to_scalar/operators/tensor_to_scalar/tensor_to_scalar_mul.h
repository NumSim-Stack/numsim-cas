#ifndef TENSOR_TO_SCALAR_MUL_H
#define TENSOR_TO_SCALAR_MUL_H

#include "../../../n_ary_tree.h"
#include "../../tensor_to_scalar_expression.h"
#include "../../../symTM_type_traits.h"

namespace symTM {

template <typename ValueType>
class tensor_to_scalar_mul final
    : public n_ary_tree<tensor_to_scalar_expression<ValueType>, tensor_to_scalar_mul<ValueType>>
{
public:
  using base = n_ary_tree<tensor_to_scalar_expression<ValueType>, tensor_to_scalar_mul<ValueType>>;
  using base::base;

  tensor_to_scalar_mul():base(){}
  ~tensor_to_scalar_mul() = default;
  tensor_to_scalar_mul(tensor_to_scalar_mul const& data):base(static_cast<base const&>(data)){}
  tensor_to_scalar_mul(tensor_to_scalar_mul && data):base(std::forward<base>(data)){}

  const tensor_to_scalar_mul &operator=(tensor_to_scalar_mul &&) = delete;
};

} // namespace symTM


#endif // TENSOR_TO_SCALAR_MUL_H
