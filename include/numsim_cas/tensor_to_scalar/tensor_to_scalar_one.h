#ifndef TENSOR_TO_SCALAR_ONE_H
#define TENSOR_TO_SCALAR_ONE_H

#include <numsim_cas/core/hash_functions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

class tensor_to_scalar_one final
    : public tensor_to_scalar_node_base_t<tensor_to_scalar_one> {
public:
  using base = tensor_to_scalar_node_base_t<tensor_to_scalar_one>;
  tensor_to_scalar_one() {}
  tensor_to_scalar_one(tensor_to_scalar_one &&data)
      : base(std::move(static_cast<base &&>(data))) {}
  tensor_to_scalar_one(tensor_to_scalar_one const &data)
      : base(static_cast<base const &>(data)) {}
  ~tensor_to_scalar_one() = default;
  const tensor_to_scalar_one &operator=(tensor_to_scalar_one &&) = delete;

  friend inline bool operator<([[maybe_unused]] tensor_to_scalar_one const &lhs,
                               [[maybe_unused]] tensor_to_scalar_one const &rhs) {
    return false;
  }
  friend inline bool operator>(tensor_to_scalar_one const &lhs,
                               tensor_to_scalar_one const &rhs) {
    return rhs < lhs;
  }
  friend inline bool operator==([[maybe_unused]] tensor_to_scalar_one const &lhs,
                                [[maybe_unused]] tensor_to_scalar_one const &rhs) {
    return true;
  }
  friend inline bool operator!=(tensor_to_scalar_one const &lhs,
                                tensor_to_scalar_one const &rhs) {
    return !(lhs == rhs);
  }

  void update_hash_value() const override {
    hash_combine(base::m_hash_value, base::get_id());
  }
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_ONE_H
