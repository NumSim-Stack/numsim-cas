#ifndef TENSOR_TO_tensor_to_scalar_one_H
#define TENSOR_TO_tensor_to_scalar_one_H

#include "../utility_func.h"
#include "tensor_to_scalar_expression.h"

namespace numsim::cas {

template <typename ValueType>
class tensor_to_scalar_one final
    : public expression_crtp<tensor_to_scalar_one<ValueType>,
                             tensor_to_scalar_expression<ValueType>> {
public:
  using base = expression_crtp<tensor_to_scalar_one<ValueType>,
                               tensor_to_scalar_expression<ValueType>>;
  tensor_to_scalar_one() { hash_combine(this->m_hash_value, base::get_id()); }
  tensor_to_scalar_one(tensor_to_scalar_one &&data)
      : base(std::move(static_cast<base &&>(data))) {}
  tensor_to_scalar_one(tensor_to_scalar_one const &data)
      : base(static_cast<base const &>(data)) {}
  ~tensor_to_scalar_one() = default;
  const tensor_to_scalar_one &operator=(tensor_to_scalar_one &&) = delete;
  template <typename _ValueType>
  friend bool operator<(tensor_to_scalar_one<_ValueType> const &lhs,
                        tensor_to_scalar_one<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator>(tensor_to_scalar_one<_ValueType> const &lhs,
                        tensor_to_scalar_one<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator==(tensor_to_scalar_one<_ValueType> const &lhs,
                         tensor_to_scalar_one<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator!=(tensor_to_scalar_one<_ValueType> const &lhs,
                         tensor_to_scalar_one<_ValueType> const &rhs);

  virtual void update_hash_value() const override {
    hash_combine(base::m_hash_value, base::get_id());
  }
};

template <typename _ValueType>
bool operator<([[maybe_unused]] tensor_to_scalar_one<_ValueType> const &lhs,
               [[maybe_unused]] tensor_to_scalar_one<_ValueType> const &rhs) {
  return true;
}

template <typename _ValueType>
bool operator>(tensor_to_scalar_one<_ValueType> const &lhs,
               tensor_to_scalar_one<_ValueType> const &rhs) {
  return !(lhs < rhs);
}

template <typename _ValueType>
bool operator==([[maybe_unused]] tensor_to_scalar_one<_ValueType> const &lhs,
                [[maybe_unused]] tensor_to_scalar_one<_ValueType> const &rhs) {
  return true;
}

template <typename _ValueType>
bool operator!=(tensor_to_scalar_one<_ValueType> const &lhs,
                tensor_to_scalar_one<_ValueType> const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // TENSOR_TO_tensor_to_scalar_one_H
