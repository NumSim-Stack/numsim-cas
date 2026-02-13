#ifndef TENSOR_ZERO_H
#define TENSOR_ZERO_H

#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

class tensor_zero final : public tensor_node_base_t<tensor_zero> {
public:
  using base = tensor_node_base_t<tensor_zero>;

  tensor_zero() = delete;
  tensor_zero(std::size_t dim, std::size_t rank) : base(dim, rank) {}
  tensor_zero(tensor_zero &&data)
      : base(static_cast<base &&>(data), data.dim(), data.rank()) {}
  tensor_zero(tensor_zero const &data)
      : base(static_cast<base const &>(data), data.dim(), data.rank()) {}
  ~tensor_zero() = default;
  const tensor_zero &operator=(tensor_zero &&) = delete;

  friend bool operator<(tensor_zero const &lhs, tensor_zero const &rhs);
  friend bool operator>(tensor_zero const &lhs, tensor_zero const &rhs);
  friend bool operator==(tensor_zero const &lhs, tensor_zero const &rhs);
  friend bool operator!=(tensor_zero const &lhs, tensor_zero const &rhs);

  void update_hash_value() const override;
};

} // namespace numsim::cas

#endif // TENSOR_ZERO_H
