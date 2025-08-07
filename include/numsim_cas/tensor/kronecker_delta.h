#ifndef KRONECKER_DELTA_H
#define KRONECKER_DELTA_H

#include "../numsim_cas_type_traits.h"

namespace numsim::cas {

template <typename ValueType>
class kronecker_delta final
    : public expression_crtp<kronecker_delta<ValueType>,
                             tensor_expression<ValueType>> {
public:
  using base_expr =
      expression_crtp<kronecker_delta<ValueType>, tensor_expression<ValueType>>;
  using value_type = ValueType;
  kronecker_delta(std::size_t dim) : base_expr(dim, 2) {}
  kronecker_delta(kronecker_delta const &data) : base_expr(data.m_dim, 2) {}
  template <typename _ValueType>
  friend bool operator<(kronecker_delta<_ValueType> const &lhs,
                        kronecker_delta<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator>(kronecker_delta<_ValueType> const &lhs,
                        kronecker_delta<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator==(kronecker_delta<_ValueType> const &lhs,
                         kronecker_delta<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator!=(kronecker_delta<_ValueType> const &lhs,
                         kronecker_delta<_ValueType> const &rhs);
};

template <typename ValueType>
bool operator<(kronecker_delta<ValueType> const &lhs,
               kronecker_delta<ValueType> const &rhs) {
  return lhs.hash_value() < rhs.hash_value();
}

template <typename ValueType>
bool operator>(kronecker_delta<ValueType> const &lhs,
               kronecker_delta<ValueType> const &rhs) {
  return !(lhs < rhs);
}

template <typename ValueType>
bool operator==(kronecker_delta<ValueType> const &lhs,
                kronecker_delta<ValueType> const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

template <typename ValueType>
bool operator!=(kronecker_delta<ValueType> const &lhs,
                kronecker_delta<ValueType> const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // KRONECKER_DELTA_H
