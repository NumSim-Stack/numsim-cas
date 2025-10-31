#ifndef TENSOR_IDENTITY_H
#define TENSOR_IDENTITY_H

#include "../expression_crtp.h"
#include "tensor_expression.h"

namespace numsim::cas {
template <typename ValueType>
class tensor_identity final
    : public expression_crtp<tensor_identity<ValueType>,
                             tensor_expression<ValueType>> {
public:
  using base =
      expression_crtp<tensor_identity<ValueType>, tensor_expression<ValueType>>;

  tensor_identity() = delete;
  tensor_identity(std::size_t dim, std::size_t rank) : base(dim, rank) {}
  tensor_identity(tensor_identity &&data)
      : base(static_cast<base &&>(data), data.dim(), data.rank()) {}
  tensor_identity(tensor_identity const &data)
      : base(static_cast<base const &>(data), data.dim(), data.rank()) {}
  ~tensor_identity() = default;
  const tensor_identity &operator=(tensor_identity &&) = delete;

  template <typename _ValueType>
  friend bool operator<(tensor_identity<_ValueType> const &lhs,
                        tensor_identity<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator>(tensor_identity<_ValueType> const &lhs,
                        tensor_identity<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator==(tensor_identity<_ValueType> const &lhs,
                         tensor_identity<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator!=(tensor_identity<_ValueType> const &lhs,
                         tensor_identity<_ValueType> const &rhs);

  virtual void update_hash_value() const override {
    hash_combine(base::m_hash_value, base::get_id());
  }
};

template <typename ValueType>
bool operator<(tensor_identity<ValueType> const &lhs,
               tensor_identity<ValueType> const &rhs) {
  return lhs.hash_value() < rhs.hash_value();
}

template <typename ValueType>
bool operator>(tensor_identity<ValueType> const &lhs,
               tensor_identity<ValueType> const &rhs) {
  return !(lhs < rhs);
}

template <typename ValueType>
bool operator==(tensor_identity<ValueType> const &lhs,
                tensor_identity<ValueType> const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

template <typename ValueType>
bool operator!=(tensor_identity<ValueType> const &lhs,
                tensor_identity<ValueType> const &rhs) {
  return !(lhs == rhs);
}
} // namespace numsim::cas

#endif // TENSOR_IDENTITY_H
