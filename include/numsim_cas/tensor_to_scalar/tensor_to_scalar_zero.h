#ifndef TENSOR_TO_SCALAR_ZERO_H
#define TENSOR_TO_SCALAR_ZERO_H

#include "../expression_crtp.h"
#include "tensor_to_scalar_expression.h"

namespace numsim::cas {

template <typename ValueType>
class tensor_to_scalar_zero final
    : public expression_crtp<tensor_to_scalar_zero<ValueType>,
                             tensor_to_scalar_expression<ValueType>> {
public:
  using base = expression_crtp<tensor_to_scalar_zero<ValueType>,
                               tensor_to_scalar_expression<ValueType>>;

  tensor_to_scalar_zero() = default;
  tensor_to_scalar_zero(tensor_to_scalar_zero &&data)
      : base(static_cast<base &&>(data)) {}
  tensor_to_scalar_zero(tensor_to_scalar_zero const &data)
      : base(static_cast<base const &>(data)) {}
  ~tensor_to_scalar_zero() = default;
  const tensor_to_scalar_zero &operator=(tensor_to_scalar_zero &&) = delete;

  template <typename _ValueType>
  friend bool operator<(tensor_to_scalar_zero<_ValueType> const &lhs,
                        tensor_to_scalar_zero<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator>(tensor_to_scalar_zero<_ValueType> const &lhs,
                        tensor_to_scalar_zero<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator==(tensor_to_scalar_zero<_ValueType> const &lhs,
                         tensor_to_scalar_zero<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator!=(tensor_to_scalar_zero<_ValueType> const &lhs,
                         tensor_to_scalar_zero<_ValueType> const &rhs);

  virtual void update_hash_value() const override {
    hash_combine(base::m_hash_value, base::get_id());
  }
};

template <typename ValueType>
bool operator<(tensor_to_scalar_zero<ValueType> const &lhs,
               tensor_to_scalar_zero<ValueType> const &rhs) {
  return lhs.hash_value() < rhs.hash_value();
}

template <typename ValueType>
bool operator>(tensor_to_scalar_zero<ValueType> const &lhs,
               tensor_to_scalar_zero<ValueType> const &rhs) {
  return !(lhs < rhs);
}

template <typename ValueType>
bool operator==(tensor_to_scalar_zero<ValueType> const &lhs,
                tensor_to_scalar_zero<ValueType> const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

template <typename ValueType>
bool operator!=(tensor_to_scalar_zero<ValueType> const &lhs,
                tensor_to_scalar_zero<ValueType> const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_ZERO_H
