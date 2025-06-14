#ifndef TENSOR_TO_SCALAR_ADD_H
#define TENSOR_TO_SCALAR_ADD_H

#include "../../../n_ary_tree.h"
#include "../../../numsim_cas_type_traits.h"
#include "../../tensor_to_scalar_expression.h"

namespace numsim::cas {

template <typename ValueType>
class tensor_to_scalar_add final
    : public n_ary_tree<tensor_to_scalar_expression<ValueType>,
                        tensor_to_scalar_add<ValueType>> {
public:
  using base = n_ary_tree<tensor_to_scalar_expression<ValueType>,
                          tensor_to_scalar_add<ValueType>>;
  using base_expr = expression_crtp<tensor_to_scalar_add<ValueType>,
                                    tensor_to_scalar_expression<ValueType>>;
  using base::base;

  tensor_to_scalar_add() : base() {}
  ~tensor_to_scalar_add() = default;
  tensor_to_scalar_add(tensor_to_scalar_add const &data)
      : base(static_cast<base const &>(data)) {}
  tensor_to_scalar_add(tensor_to_scalar_add &&data)
      : base(std::forward<base>(data)) {}

  const tensor_to_scalar_add &operator=(tensor_to_scalar_add &&) = delete;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_ADD_H
