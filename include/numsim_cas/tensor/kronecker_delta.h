#ifndef KRONECKER_DELTA_H
#define KRONECKER_DELTA_H

#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

class kronecker_delta final : public tensor_node_base_t<kronecker_delta> {
public:
  using base = tensor_node_base_t<kronecker_delta>;
  kronecker_delta(std::size_t dim) : base(dim, 2) {}
  kronecker_delta(kronecker_delta const &data) : base(data.m_dim, 2) {}

  void update_hash_value() const override {
    base::m_hash_value = 0;
    hash_combine(base::m_hash_value, base::get_id());
    hash_combine(base::m_hash_value, this->dim());
  }

  friend bool operator<(kronecker_delta const &lhs, kronecker_delta const &rhs);
  friend bool operator>(kronecker_delta const &lhs, kronecker_delta const &rhs);
  friend bool operator==(kronecker_delta const &lhs,
                         kronecker_delta const &rhs);
  friend bool operator!=(kronecker_delta const &lhs,
                         kronecker_delta const &rhs);
};

inline bool operator<(kronecker_delta const &lhs, kronecker_delta const &rhs) {
  return lhs.hash_value() < rhs.hash_value();
}

inline bool operator>(kronecker_delta const &lhs, kronecker_delta const &rhs) {
  return rhs < lhs;
}

inline bool operator==(kronecker_delta const &lhs, kronecker_delta const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

inline bool operator!=(kronecker_delta const &lhs, kronecker_delta const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // KRONECKER_DELTA_H
