#ifndef SCALAR_ZERO_H
#define SCALAR_ZERO_H

#include "../expression_crtp.h"
#include "scalar_expression.h"

namespace numsim::cas {

template <typename ValueType>
class scalar_zero final : public expression_crtp<scalar_zero<ValueType>,
                                                 scalar_expression<ValueType>> {
public:
  using base =
      expression_crtp<scalar_zero<ValueType>, scalar_expression<ValueType>>;

  scalar_zero() = default;
  scalar_zero(scalar_zero &&data) : base(static_cast<base &&>(data)) {}
  scalar_zero(scalar_zero const &data)
      : base(static_cast<base const &>(data)) {}
  ~scalar_zero() = default;
  const scalar_zero &operator=(scalar_zero &&) = delete;

  template <typename _ValueType>
  friend bool operator<(scalar_zero<_ValueType> const &lhs,
                        scalar_zero<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator>(scalar_zero<_ValueType> const &lhs,
                        scalar_zero<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator==(scalar_zero<_ValueType> const &lhs,
                         scalar_zero<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator!=(scalar_zero<_ValueType> const &lhs,
                         scalar_zero<_ValueType> const &rhs);
};

template <typename ValueType>
bool operator<(scalar_zero<ValueType> const &lhs,
               scalar_zero<ValueType> const &rhs) {
  return lhs.hash_value() < rhs.hash_value();
}

template <typename ValueType>
bool operator>(scalar_zero<ValueType> const &lhs,
               scalar_zero<ValueType> const &rhs) {
  return !(lhs < rhs);
}

template <typename ValueType>
bool operator==(scalar_zero<ValueType> const &lhs,
                scalar_zero<ValueType> const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

template <typename ValueType>
bool operator!=(scalar_zero<ValueType> const &lhs,
                scalar_zero<ValueType> const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // SCALAR_ZERO_H
