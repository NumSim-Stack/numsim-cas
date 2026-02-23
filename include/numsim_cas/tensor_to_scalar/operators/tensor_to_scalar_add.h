#ifndef TENSOR_TO_SCALAR_ADD_H
#define TENSOR_TO_SCALAR_ADD_H

#include <numsim_cas/core/n_ary_tree.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

class tensor_to_scalar_add final
    : public n_ary_tree<tensor_to_scalar_node_base_t<tensor_to_scalar_add>> {
public:
  using expr_t = tensor_to_scalar_expression;
  using base = n_ary_tree<tensor_to_scalar_node_base_t<tensor_to_scalar_add>>;
  using base::base;

  tensor_to_scalar_add() : base() {}
  ~tensor_to_scalar_add() override = default;
  tensor_to_scalar_add(tensor_to_scalar_add const &data)
      : base(static_cast<base const &>(data)) {}
  tensor_to_scalar_add(tensor_to_scalar_add &&data) noexcept
      : base(std::forward<base>(data)) {}

  const tensor_to_scalar_add &operator=(tensor_to_scalar_add &&) = delete;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_ADD_H
