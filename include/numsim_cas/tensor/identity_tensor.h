#ifndef IDENTITY_TENSOR_H
#define IDENTITY_TENSOR_H

#include "../expression_crtp.h"
#include "tensor_expression.h"

namespace numsim::cas {

class identity_tensor final
    : public expression_crtp<identity_tensor, tensor_expression> {
public:
  using base = expression_crtp<identity_tensor, tensor_expression>;

  identity_tensor() = delete;
  identity_tensor(std::size_t dim, std::size_t rank) : base(dim, rank) {}
  identity_tensor(identity_tensor &&data)
      : base(static_cast<base &&>(data), data.dim(), data.rank()) {}
  identity_tensor(identity_tensor const &data)
      : base(static_cast<base const &>(data), data.dim(), data.rank()) {}
  ~identity_tensor() = default;
  const identity_tensor &operator=(identity_tensor &&) = delete;

  friend bool operator<(identity_tensor const &lhs, identity_tensor const &rhs);
  friend bool operator>(identity_tensor const &lhs, identity_tensor const &rhs);
  friend bool operator==(identity_tensor const &lhs,
                         identity_tensor const &rhs);
  friend bool operator!=(identity_tensor const &lhs,
                         identity_tensor const &rhs);

  virtual void update_hash_value() const override {
    hash_combine(base::m_hash_value, base::get_id());
  }
};

bool operator<(identity_tensor const &lhs, identity_tensor const &rhs) {
  return lhs.hash_value() < rhs.hash_value();
}

bool operator>(identity_tensor const &lhs, identity_tensor const &rhs) {
  return rhs < lhs;
}

bool operator==(identity_tensor const &lhs, identity_tensor const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

bool operator!=(identity_tensor const &lhs, identity_tensor const &rhs) {
  return !(lhs == rhs);
}
} // namespace numsim::cas

#endif // IDENTITY_TENSOR_H
