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
                        tensor_to_scalar_zero const &rhs) {
    return lhs.hash_value() < rhs.hash_value();
  }

  friend bool operator>(tensor_to_scalar_zero const &lhs,
                        tensor_to_scalar_zero const &rhs) {
    return rhs < lhs;
  }

  friend bool operator==(tensor_to_scalar_zero const &lhs,
                         tensor_to_scalar_zero const &rhs) {
    return lhs.hash_value() == rhs.hash_value();
  }

  friend bool operator!=(tensor_to_scalar_zero const &lhs,
                         tensor_to_scalar_zero const &rhs) {
    return !(lhs == rhs);
  }

  virtual void update_hash_value() const override {
    hash_combine(base::m_hash_value, base::get_id());
  }
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_ZERO_H
