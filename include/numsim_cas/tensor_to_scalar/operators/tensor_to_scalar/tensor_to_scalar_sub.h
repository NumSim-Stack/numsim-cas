#ifndef TENSOR_TO_SCALAR_SUB_H
#define TENSOR_TO_SCALAR_SUB_H

#include "../../../n_ary_tree.h"
#include "../../tensor_to_scalar_expression.h"
#include "../../../numsim_cas_type_traits.h"

namespace numsim::cas {

template <typename ValueType>
class tensor_to_scalar_sub final
    : public n_ary_tree<tensor_to_scalar_expression<ValueType>, tensor_to_scalar_sub<ValueType>>
{
public:
  using base = n_ary_tree<tensor_to_scalar_expression<ValueType>, tensor_to_scalar_sub<ValueType>>;
  using base_expr = expression_crtp<tensor_to_scalar_sub<ValueType>, tensor_to_scalar_expression<ValueType>>;
  using base::base;

  tensor_to_scalar_sub():base(){}
  ~tensor_to_scalar_sub() = default;
  tensor_to_scalar_sub(tensor_to_scalar_sub const& data):base(static_cast<base const&>(data)){}
  tensor_to_scalar_sub(tensor_to_scalar_sub && data):base(std::forward<base>(data)){}

  const tensor_to_scalar_sub &operator=(tensor_to_scalar_sub &&) = delete;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_SUB_H
