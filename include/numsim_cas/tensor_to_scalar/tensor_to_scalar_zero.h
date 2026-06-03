#ifndef TENSOR_TO_SCALAR_ZERO_H
#define TENSOR_TO_SCALAR_ZERO_H

#include <numsim_cas/core/hash_functions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

class tensor_to_scalar_zero final
    : public tensor_to_scalar_node_base_t<tensor_to_scalar_zero> {
public:
  using base = tensor_to_scalar_node_base_t<tensor_to_scalar_zero>;

  // The constant 0 is mathematically: nonnegative, nonpositive, real,
  // integer, rational. nonzero is INTENTIONALLY absent — 0 is the
  // additive identity, not nonzero. positive and negative are likewise
  // absent (0 is neither). The omissions are load-bearing for
  // soundness; the matching EXPECT_FALSE checks in
  // TensorAlgebraConstants.TensorToScalarZeroCarriesNonnegativeNonpositive
  // lock them in. Same rationale as tensor_to_scalar_one — pre-annotate
  // so downstream queries see the right answer without depending on a
  // separate fold. Closes #261 (alongside the matching annotation in
  // tensor_to_scalar_one).
  tensor_to_scalar_zero() {
    auto &a = this->assumptions();
    a.insert(nonnegative{});
    a.insert(nonpositive{});
    a.insert(real_tag{});
    a.insert(integer{});
    a.insert(rational{});
    a.set_inferred();
  }
  tensor_to_scalar_zero(tensor_to_scalar_zero &&data) noexcept
      : base(static_cast<base &&>(data)) {}
  tensor_to_scalar_zero(tensor_to_scalar_zero const &data)
      : base(static_cast<base const &>(data)) {}
  ~tensor_to_scalar_zero() override = default;
  const tensor_to_scalar_zero &operator=(tensor_to_scalar_zero &&) = delete;

  friend inline bool operator<(tensor_to_scalar_zero const &lhs,
                               tensor_to_scalar_zero const &rhs) {
    return lhs.hash_value() < rhs.hash_value();
  }
  friend inline bool operator>(tensor_to_scalar_zero const &lhs,
                               tensor_to_scalar_zero const &rhs) {
    return rhs < lhs;
  }
  friend inline bool operator==(tensor_to_scalar_zero const &lhs,
                                tensor_to_scalar_zero const &rhs) {
    return lhs.hash_value() == rhs.hash_value();
  }
  friend inline bool operator!=(tensor_to_scalar_zero const &lhs,
                                tensor_to_scalar_zero const &rhs) {
    return !(lhs == rhs);
  }

  void update_hash_value() const override {
    hash_combine(base::m_hash_value, base::get_id());
  }
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_ZERO_H
