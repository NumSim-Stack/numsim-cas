#ifndef TENSOR_ZERO_H
#define TENSOR_ZERO_H

#include <numsim_cas/core/hash_functions.h>
#include <numsim_cas/tensor/tensor_expression.h>

#include <utility>

namespace numsim::cas {

class tensor_zero final : public tensor_node_base_t<tensor_zero> {
public:
  using base = tensor_node_base_t<tensor_zero>;

  tensor_zero() = delete;
  tensor_zero(std::size_t dim, std::size_t rank) : base(dim, rank) {}
  tensor_zero(tensor_zero &&data) noexcept
      : base(static_cast<base &&>(data), data.dim(), data.rank()) {}
  tensor_zero(tensor_zero const &data)
      : base(static_cast<base const &>(data), data.dim(), data.rank()) {}
  ~tensor_zero() override = default;
  const tensor_zero &operator=(tensor_zero &&) = delete;

  // The hash covers only the node id, so shape is the real tiebreak: the
  // additive identity of one dim and rank is not the one of another.
  friend inline bool operator<(tensor_zero const &lhs, tensor_zero const &rhs) {
    if (lhs.hash_value() != rhs.hash_value())
      return lhs.hash_value() < rhs.hash_value();
    return std::pair{lhs.dim(), lhs.rank()} < std::pair{rhs.dim(), rhs.rank()};
  }
  friend inline bool operator>(tensor_zero const &lhs, tensor_zero const &rhs) {
    return rhs < lhs;
  }
  friend inline bool operator==(tensor_zero const &lhs,
                                tensor_zero const &rhs) {
    return lhs.hash_value() == rhs.hash_value() && lhs.dim() == rhs.dim() &&
           lhs.rank() == rhs.rank();
  }
  friend inline bool operator!=(tensor_zero const &lhs,
                                tensor_zero const &rhs) {
    return !(lhs == rhs);
  }

  void update_hash_value() const override {
    base::m_hash_value = 0;
    hash_combine(base::m_hash_value, base::get_id());
  }
};

} // namespace numsim::cas

#endif // TENSOR_ZERO_H
