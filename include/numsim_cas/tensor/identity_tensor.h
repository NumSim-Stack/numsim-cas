#ifndef IDENTITY_TENSOR_H
#define IDENTITY_TENSOR_H

#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

class identity_tensor final : public tensor_node_base_t<identity_tensor> {
public:
  using base = tensor_node_base_t<identity_tensor>;

  identity_tensor() = delete;
  identity_tensor(std::size_t dim, std::size_t rank) : base(dim, rank) {}
  identity_tensor(identity_tensor &&data) noexcept
      : base(static_cast<base &&>(data), data.dim(), data.rank()) {}
  identity_tensor(identity_tensor const &data)
      : base(static_cast<base const &>(data), data.dim(), data.rank()) {}
  ~identity_tensor() override = default;
  const identity_tensor &operator=(identity_tensor &&) = delete;

  friend bool operator<(identity_tensor const &lhs, identity_tensor const &rhs);
  friend bool operator>(identity_tensor const &lhs, identity_tensor const &rhs);
  friend bool operator==(identity_tensor const &lhs,
                         identity_tensor const &rhs);
  friend bool operator!=(identity_tensor const &lhs,
                         identity_tensor const &rhs);

  void update_hash_value() const override {
    base::m_hash_value = 0;
    hash_combine(base::m_hash_value, base::get_id());
    hash_combine(base::m_hash_value, this->dim());
    hash_combine(base::m_hash_value, this->rank());
  }
};

inline bool operator<(identity_tensor const &lhs, identity_tensor const &rhs) {
  return lhs.hash_value() < rhs.hash_value();
}

inline bool operator>(identity_tensor const &lhs, identity_tensor const &rhs) {
  return rhs < lhs;
}

inline bool operator==(identity_tensor const &lhs, identity_tensor const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

inline bool operator!=(identity_tensor const &lhs, identity_tensor const &rhs) {
  return !(lhs == rhs);
}
} // namespace numsim::cas

#endif // IDENTITY_TENSOR_H
