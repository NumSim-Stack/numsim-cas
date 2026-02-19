#ifndef TENSOR_TO_SCALAR_ONE_H
#define TENSOR_TO_SCALAR_ONE_H

#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

class tensor_to_scalar_one final
    : public tensor_to_scalar_node_base_t<tensor_to_scalar_one> {
public:
  using base = tensor_to_scalar_node_base_t<tensor_to_scalar_one>;
  tensor_to_scalar_one() {}
  tensor_to_scalar_one(tensor_to_scalar_one &&data)
      : base(std::move(static_cast<base &&>(data))) {}
  tensor_to_scalar_one(tensor_to_scalar_one const &data)
      : base(static_cast<base const &>(data)) {}
  ~tensor_to_scalar_one() = default;
  const tensor_to_scalar_one &operator=(tensor_to_scalar_one &&) = delete;

  friend bool operator<(tensor_to_scalar_one const &lhs,
                        tensor_to_scalar_one const &rhs);
  friend bool operator>(tensor_to_scalar_one const &lhs,
                        tensor_to_scalar_one const &rhs);
  friend bool operator==(tensor_to_scalar_one const &lhs,
                         tensor_to_scalar_one const &rhs);
  friend bool operator!=(tensor_to_scalar_one const &lhs,
                         tensor_to_scalar_one const &rhs);

  void update_hash_value() const override;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_ONE_H
