#ifndef TENSOR_TO_SCALAR_ZERO_H
#define TENSOR_TO_SCALAR_ZERO_H

#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

class tensor_to_scalar_zero final
    : public tensor_to_scalar_node_base_t<tensor_to_scalar_zero> {
public:
  using base = tensor_to_scalar_node_base_t<tensor_to_scalar_zero>;

  tensor_to_scalar_zero() = default;
  tensor_to_scalar_zero(tensor_to_scalar_zero &&data)
      : base(static_cast<base &&>(data)) {}
  tensor_to_scalar_zero(tensor_to_scalar_zero const &data)
      : base(static_cast<base const &>(data)) {}
  ~tensor_to_scalar_zero() = default;
  const tensor_to_scalar_zero &operator=(tensor_to_scalar_zero &&) = delete;

  friend bool operator<(tensor_to_scalar_zero const &lhs,
                        tensor_to_scalar_zero const &rhs);
  friend bool operator>(tensor_to_scalar_zero const &lhs,
                        tensor_to_scalar_zero const &rhs);
  friend bool operator==(tensor_to_scalar_zero const &lhs,
                         tensor_to_scalar_zero const &rhs);
  friend bool operator!=(tensor_to_scalar_zero const &lhs,
                         tensor_to_scalar_zero const &rhs);

  void update_hash_value() const override;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_ZERO_H
