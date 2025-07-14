#ifndef SCALAR_ACOS_H
#define SCALAR_ACOS_H

#include "../unary_op.h"
namespace numsim::cas {
template <typename ValueType>
class scalar_acos final
    : public unary_op<scalar_acos<ValueType>, scalar_expression<ValueType>> {
public:
  using base = unary_op<scalar_acos<ValueType>, scalar_expression<ValueType>>;

  using base::base;
  scalar_acos(scalar_acos const &expr)
      : base(static_cast<base const &>(expr)) {}
  scalar_acos(scalar_acos &&expr)
      : base(std::move(static_cast<base &&>(expr))) {}
  scalar_acos() = delete;
  ~scalar_acos() = default;
  const scalar_acos &operator=(scalar_acos &&) = delete;
  template <typename _ValueType>
  friend bool operator<(scalar_acos<_ValueType> const &lhs,
                        scalar_acos<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator>(scalar_acos<_ValueType> const &lhs,
                        scalar_acos<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator==(scalar_acos<_ValueType> const &lhs,
                         scalar_acos<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator!=(scalar_acos<_ValueType> const &lhs,
                         scalar_acos<_ValueType> const &rhs);

private:
};

template <typename _ValueType>
bool operator>(scalar_acos<_ValueType> const &lhs,
               scalar_acos<_ValueType> const &rhs) {
  return !(lhs < rhs);
}

template <typename _ValueType>
bool operator==(scalar_acos<_ValueType> const &lhs,
                scalar_acos<_ValueType> const &rhs) {
  return lhs.expr() == rhs.expr();
}

template <typename _ValueType>
bool operator!=(scalar_acos<_ValueType> const &lhs,
                scalar_acos<_ValueType> const &rhs) {
  return !(lhs == rhs);
}
} // namespace numsim::cas

#endif // SCALAR_ACOS_H
