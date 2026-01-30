#ifndef KRONECKER_DELTA_H
#define KRONECKER_DELTA_H

#include "../numsim_cas_type_traits.h"

namespace numsim::cas {

class kronecker_delta final
    : public expression_crtp<kronecker_delta, tensor_expression> {
public:
  using base = expression_crtp<kronecker_delta, tensor_expression>;
  kronecker_delta(std::size_t dim) : base(dim, 2) {}
  kronecker_delta(kronecker_delta const &data) : base(data.m_dim, 2) {}

  virtual void update_hash_value() const override {
    hash_combine(base::m_hash_value, base::get_id());
  }

  friend bool operator<(kronecker_delta const &lhs, kronecker_delta const &rhs);
  friend bool operator>(kronecker_delta const &lhs, kronecker_delta const &rhs);
  friend bool operator==(kronecker_delta const &lhs,
                         kronecker_delta const &rhs);
  friend bool operator!=(kronecker_delta const &lhs,
                         kronecker_delta const &rhs);
};

bool operator<(kronecker_delta const &lhs, kronecker_delta const &rhs) {
  return lhs.hash_value() < rhs.hash_value();
}

bool operator>(kronecker_delta const &lhs, kronecker_delta const &rhs) {
  return rhs < lhs;
}

bool operator==(kronecker_delta const &lhs, kronecker_delta const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

bool operator!=(kronecker_delta const &lhs, kronecker_delta const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // KRONECKER_DELTA_H
