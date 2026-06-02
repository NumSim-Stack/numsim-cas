#ifndef TENSOR_TO_SCALAR_ONE_H
#define TENSOR_TO_SCALAR_ONE_H

#include <numsim_cas/core/hash_functions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

class tensor_to_scalar_one final
    : public tensor_to_scalar_node_base_t<tensor_to_scalar_one> {
public:
  using base = tensor_to_scalar_node_base_t<tensor_to_scalar_one>;
  // The constant 1 is mathematically: positive, nonnegative, nonzero,
  // real, integer, rational. Pre-annotate so downstream queries
  // (e.g. is_positive(det(orthogonal R))) see the right answer without
  // relying on a separate fold to insert these tags. Closes #261 and
  // the H1 inconsistency surfaced by the 1.0-α cross-PR review
  // (det(orth) returned an un-annotated 1 while det(PD) carried
  // positive — same det() function with semantically different result
  // annotations).
  tensor_to_scalar_one() {
    auto &a = this->assumptions();
    a.insert(positive{});
    a.insert(nonnegative{});
    a.insert(nonzero{});
    a.insert(real_tag{});
    a.insert(integer{});
    a.insert(rational{});
    a.set_inferred();
  }
  tensor_to_scalar_one(tensor_to_scalar_one &&data) noexcept
      : base(std::move(static_cast<base &&>(data))) {}
  tensor_to_scalar_one(tensor_to_scalar_one const &data)
      : base(static_cast<base const &>(data)) {}
  ~tensor_to_scalar_one() override = default;
  const tensor_to_scalar_one &operator=(tensor_to_scalar_one &&) = delete;

  friend inline bool
  operator<([[maybe_unused]] tensor_to_scalar_one const &lhs,
            [[maybe_unused]] tensor_to_scalar_one const &rhs) {
    return false;
  }
  friend inline bool operator>(tensor_to_scalar_one const &lhs,
                               tensor_to_scalar_one const &rhs) {
    return rhs < lhs;
  }
  friend inline bool
  operator==([[maybe_unused]] tensor_to_scalar_one const &lhs,
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
