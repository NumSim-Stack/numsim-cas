#ifndef TENSOR_TO_SCALAR_SCALAR_WRAPPER_H
#define TENSOR_TO_SCALAR_SCALAR_WRAPPER_H

#include "../scalar/scalar_expression.h"
#include "tensor_to_scalar_expression.h"

namespace numsim::cas {

class tensor_to_scalar_scalar_wrapper final
    : public expression_crtp<tensor_to_scalar_scalar_wrapper,
                             tensor_to_scalar_expression> {
public:
  using base = expression_crtp<tensor_to_scalar_scalar_wrapper,
                               tensor_to_scalar_expression>;

  tensor_to_scalar_scalar_wrapper(scalar_expression &&expr)
      : m_expr(std::move(expr)) {}
  tensor_to_scalar_scalar_wrapper(tensor_to_scalar_scalar_wrapper &&data)
      : base(static_cast<base &&>(data)) {}
  tensor_to_scalar_scalar_wrapper(tensor_to_scalar_scalar_wrapper const &data)
      : base(static_cast<base const &>(data)) {}
  ~tensor_to_scalar_scalar_wrapper() = default;
  const tensor_to_scalar_scalar_wrapper &
  operator=(tensor_to_scalar_scalar_wrapper &&) = delete;

  friend bool operator<(tensor_to_scalar_scalar_wrapper const &lhs,
                        tensor_to_scalar_scalar_wrapper const &rhs);
  friend bool operator>(tensor_to_scalar_scalar_wrapper const &lhs,
                        tensor_to_scalar_scalar_wrapper const &rhs);
  friend bool operator==(tensor_to_scalar_scalar_wrapper const &lhs,
                         tensor_to_scalar_scalar_wrapper const &rhs);
  friend bool operator!=(tensor_to_scalar_scalar_wrapper const &lhs,
                         tensor_to_scalar_scalar_wrapper const &rhs);

  virtual void update_hash_value() const override {
    hash_combine(base::m_hash_value, base::get_id());
  }

  const auto &expr() const { return m_expr; }

  auto &expr() { return m_expr; }

private:
  expression_holder<scalar_expression> m_expr;
};

bool operator<(tensor_to_scalar_scalar_wrapper const &lhs,
               tensor_to_scalar_scalar_wrapper const &rhs) {
  return lhs.hash_value() < rhs.hash_value();
}

bool operator>(tensor_to_scalar_scalar_wrapper const &lhs,
               tensor_to_scalar_scalar_wrapper const &rhs) {
  return rhs < lhs;
}

bool operator==(tensor_to_scalar_scalar_wrapper const &lhs,
                tensor_to_scalar_scalar_wrapper const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

bool operator!=(tensor_to_scalar_scalar_wrapper const &lhs,
                tensor_to_scalar_scalar_wrapper const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_SCALAR_WRAPPER_H
