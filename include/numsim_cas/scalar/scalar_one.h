#ifndef SCALAR_ONE_H
#define SCALAR_ONE_H

#include <numsim_cas/core/hash_functions.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_one final : public scalar_node_base_t<scalar_one> {
public:
  using base = scalar_node_base_t<scalar_one>;
  scalar_one() { hash_combine(this->m_hash_value, base::get_id()); }
  scalar_one(scalar_one &&data) : base(std::move(static_cast<base &&>(data))) {}
  scalar_one(scalar_one const &data) : base(static_cast<base const &>(data)) {}
  ~scalar_one() = default;
  const scalar_one &operator=(scalar_one &&) = delete;

  friend bool operator<(scalar_one const &lhs, scalar_one const &rhs);

  friend bool operator>(scalar_one const &lhs, scalar_one const &rhs);

  friend bool operator==(scalar_one const &lhs, scalar_one const &rhs);

  friend bool operator!=(scalar_one const &lhs, scalar_one const &rhs);

  virtual void update_hash_value() const override {
    hash_combine(base::m_hash_value, base::get_id());
  }
};

} // namespace numsim::cas

#endif // SCALAR_ONE_H
