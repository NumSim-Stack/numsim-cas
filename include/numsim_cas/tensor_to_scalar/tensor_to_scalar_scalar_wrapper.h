#ifndef TENSOR_TO_SCALAR_SCALAR_WRAPPER_H
#define TENSOR_TO_SCALAR_SCALAR_WRAPPER_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

class tensor_to_scalar_scalar_wrapper final
    : public unary_op<
          tensor_to_scalar_node_base_t<tensor_to_scalar_scalar_wrapper>,
          scalar_expression> {
public:
  using base =
      unary_op<tensor_to_scalar_node_base_t<tensor_to_scalar_scalar_wrapper>,
               scalar_expression>;
  using base::base;

  tensor_to_scalar_scalar_wrapper(tensor_to_scalar_scalar_wrapper &&data)
      : base(std::move(static_cast<base &&>(data))) {}
  tensor_to_scalar_scalar_wrapper(tensor_to_scalar_scalar_wrapper const &data)
      : base(data) {}
  ~tensor_to_scalar_scalar_wrapper() = default;

  const tensor_to_scalar_scalar_wrapper &
  operator=(tensor_to_scalar_scalar_wrapper &&) = delete;

  friend bool operator<(tensor_to_scalar_scalar_wrapper const &lhs,
                        tensor_to_scalar_scalar_wrapper const &rhs) {
    return lhs.hash_value() < rhs.hash_value();
  }

  friend bool operator>(tensor_to_scalar_scalar_wrapper const &lhs,
                        tensor_to_scalar_scalar_wrapper const &rhs) {
    return rhs < lhs;
  }

  friend bool operator==(tensor_to_scalar_scalar_wrapper const &lhs,
                         tensor_to_scalar_scalar_wrapper const &rhs) {
    return lhs.hash_value() == rhs.hash_value();
  }

  friend bool operator!=(tensor_to_scalar_scalar_wrapper const &lhs,
                         tensor_to_scalar_scalar_wrapper const &rhs) {
    return !(lhs == rhs);
  }

  virtual void update_hash_value() const noexcept override {
    base::m_hash_value = 0;
    hash_combine(base::m_hash_value, base::get_id());
    if (this->expr().is_valid()) {
      hash_combine(base::m_hash_value, this->expr().get().hash_value());
    }
  }
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_SCALAR_WRAPPER_H
