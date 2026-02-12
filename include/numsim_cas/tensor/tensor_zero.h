#ifndef TENSOR_ZERO_H
#define TENSOR_ZERO_H

#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

class tensor_zero final : public tensor_node_base_t<tensor_zero> {
public:
  using base = tensor_node_base_t<tensor_zero>;

  tensor_zero() = delete;
  tensor_zero(std::size_t dim, std::size_t rank) : base(dim, rank) {}
  tensor_zero(tensor_zero &&data)
      : base(static_cast<base &&>(data), data.dim(), data.rank()) {}
  tensor_zero(tensor_zero const &data)
      : base(static_cast<base const &>(data), data.dim(), data.rank()) {}
  ~tensor_zero() = default;
  const tensor_zero &operator=(tensor_zero &&) = delete;

  // friend bool operator<(tensor_zero const &lhs, tensor_zero const &rhs);
  // friend bool operator>(tensor_zero const &lhs, tensor_zero const &rhs);
  // friend bool operator==(tensor_zero const &lhs, tensor_zero const &rhs);
  // friend bool operator!=(tensor_zero const &lhs, tensor_zero const &rhs);

  friend bool operator<(tensor_zero const &lhs, tensor_zero const &rhs) {
    return lhs.hash_value() < rhs.hash_value();
  }

  friend bool operator>(tensor_zero const &lhs, tensor_zero const &rhs) {
    return rhs < lhs;
  }

  friend bool operator==(tensor_zero const &lhs, tensor_zero const &rhs) {
    return lhs.hash_value() == rhs.hash_value();
  }

  friend bool operator!=(tensor_zero const &lhs, tensor_zero const &rhs) {
    return !(lhs == rhs);
  }

  virtual void update_hash_value() const override {
    hash_combine(base::m_hash_value, base::get_id());
  }
};

} // namespace numsim::cas

#endif // TENSOR_ZERO_H
