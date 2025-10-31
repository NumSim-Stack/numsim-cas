#ifndef TENSOR_ZERO_H
#define TENSOR_ZERO_H

#include "../expression_crtp.h"
#include "tensor_expression.h"

namespace numsim::cas {
template <typename ValueType>
class tensor_zero final : public expression_crtp<tensor_zero<ValueType>,
                                                 tensor_expression<ValueType>> {
public:
  using base =
      expression_crtp<tensor_zero<ValueType>, tensor_expression<ValueType>>;

  tensor_zero() = delete;
  tensor_zero(std::size_t dim, std::size_t rank) : base(dim, rank) {}
  tensor_zero(tensor_zero &&data)
      : base(static_cast<base &&>(data), data.dim(), data.rank()) {}
  tensor_zero(tensor_zero const &data)
      : base(static_cast<base const &>(data), data.dim(), data.rank()) {}
  ~tensor_zero() = default;
  const tensor_zero &operator=(tensor_zero &&) = delete;

  template <typename _ValueType>
  friend bool operator<(tensor_zero<_ValueType> const &lhs,
                        tensor_zero<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator>(tensor_zero<_ValueType> const &lhs,
                        tensor_zero<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator==(tensor_zero<_ValueType> const &lhs,
                         tensor_zero<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator!=(tensor_zero<_ValueType> const &lhs,
                         tensor_zero<_ValueType> const &rhs);

  virtual void update_hash_value() const override {
    hash_combine(base::m_hash_value, base::get_id());
  }
};

template <typename ValueType>
bool operator<(tensor_zero<ValueType> const &lhs,
               tensor_zero<ValueType> const &rhs) {
  return lhs.hash_value() < rhs.hash_value();
}

template <typename ValueType>
bool operator>(tensor_zero<ValueType> const &lhs,
               tensor_zero<ValueType> const &rhs) {
  return !(lhs < rhs);
}

template <typename ValueType>
bool operator==(tensor_zero<ValueType> const &lhs,
                tensor_zero<ValueType> const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

template <typename ValueType>
bool operator!=(tensor_zero<ValueType> const &lhs,
                tensor_zero<ValueType> const &rhs) {
  return !(lhs == rhs);
}
} // namespace numsim::cas

#endif // TENSOR_ZERO_H
