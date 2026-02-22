#ifndef SCALAR_ONE_H
#define SCALAR_ONE_H

#include <numsim_cas/core/hash_functions.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_one final : public scalar_node_base_t<scalar_one> {
public:
  using base = scalar_node_base_t<scalar_one>;
  scalar_one() {}
  scalar_one(scalar_one &&data) : base(std::move(static_cast<base &&>(data))) {}
  scalar_one(scalar_one const &data) : base(static_cast<base const &>(data)) {}
  ~scalar_one() = default;
  const scalar_one &operator=(scalar_one &&) = delete;

  friend inline bool operator<([[maybe_unused]] scalar_one const &lhs,
                               [[maybe_unused]] scalar_one const &rhs) {
    return false;
  }
  friend inline bool operator>(scalar_one const &lhs, scalar_one const &rhs) {
    return rhs < lhs;
  }
  friend inline bool operator==([[maybe_unused]] scalar_one const &lhs,
                                [[maybe_unused]] scalar_one const &rhs) {
    return true;
  }
  friend inline bool operator!=(scalar_one const &lhs, scalar_one const &rhs) {
    return !(lhs == rhs);
  }

private:
  void update_hash_value() const override {
    hash_combine(base::m_hash_value, base::get_id());
  }
};

} // namespace numsim::cas

#endif // SCALAR_ONE_H
