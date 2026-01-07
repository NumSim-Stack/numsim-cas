#ifndef TENSOR_TO_SCALAR_SCALAR_WRAPPER_H
#define TENSOR_TO_SCALAR_SCALAR_WRAPPER_H

#include "../scalar/scalar_expression.h"
#include "tensor_to_scalar_expression.h"

namespace numsim::cas {

template <typename ValueType>
class tensor_to_scalar_scalar_wrapper final
    : public expression_crtp<tensor_to_scalar_scalar_wrapper<ValueType>,
                             tensor_to_scalar_expression<ValueType>> {
public:
  using base = expression_crtp<tensor_to_scalar_scalar_wrapper<ValueType>,
                               tensor_to_scalar_expression<ValueType>>;

  tensor_to_scalar_scalar_wrapper(scalar_expression<ValueType> &&expr)
      : m_expr(std::move(expr)) {}
  tensor_to_scalar_scalar_wrapper(tensor_to_scalar_scalar_wrapper &&data)
      : base(static_cast<base &&>(data)) {}
  tensor_to_scalar_scalar_wrapper(tensor_to_scalar_scalar_wrapper const &data)
      : base(static_cast<base const &>(data)) {}
  ~tensor_to_scalar_scalar_wrapper() = default;
  const tensor_to_scalar_scalar_wrapper &
  operator=(tensor_to_scalar_scalar_wrapper &&) = delete;

  template <typename _ValueType>
  friend bool operator<(tensor_to_scalar_scalar_wrapper<_ValueType> const &lhs,
                        tensor_to_scalar_scalar_wrapper<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator>(tensor_to_scalar_scalar_wrapper<_ValueType> const &lhs,
                        tensor_to_scalar_scalar_wrapper<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool
  operator==(tensor_to_scalar_scalar_wrapper<_ValueType> const &lhs,
             tensor_to_scalar_scalar_wrapper<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool
  operator!=(tensor_to_scalar_scalar_wrapper<_ValueType> const &lhs,
             tensor_to_scalar_scalar_wrapper<_ValueType> const &rhs);

  virtual void update_hash_value() const override {
    hash_combine(base::m_hash_value, base::get_id());
  }

  const auto &expr() const { return m_expr; }

  auto &expr() { return m_expr; }

private:
  expression_holder<scalar_expression<ValueType>> m_expr;
};

template <typename ValueType>
bool operator<(tensor_to_scalar_scalar_wrapper<ValueType> const &lhs,
               tensor_to_scalar_scalar_wrapper<ValueType> const &rhs) {
  return lhs.hash_value() < rhs.hash_value();
}

template <typename ValueType>
bool operator>(tensor_to_scalar_scalar_wrapper<ValueType> const &lhs,
               tensor_to_scalar_scalar_wrapper<ValueType> const &rhs) {
  return !(lhs < rhs);
}

template <typename ValueType>
bool operator==(tensor_to_scalar_scalar_wrapper<ValueType> const &lhs,
                tensor_to_scalar_scalar_wrapper<ValueType> const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

template <typename ValueType>
bool operator!=(tensor_to_scalar_scalar_wrapper<ValueType> const &lhs,
                tensor_to_scalar_scalar_wrapper<ValueType> const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_SCALAR_WRAPPER_H
