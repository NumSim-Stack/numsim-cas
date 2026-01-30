#ifndef SCALAR_ZERO_H
#define SCALAR_ZERO_H

#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_zero final : public scalar_node_base_t<scalar_zero> {
public:
  using base = scalar_node_base_t<scalar_zero>;

  scalar_zero() {}
  scalar_zero(scalar_zero &&data) : base(static_cast<base &&>(data)) {}
  scalar_zero(scalar_zero const &data)
      : base(static_cast<base const &>(data)) {}
  ~scalar_zero() = default;
  const scalar_zero &operator=(scalar_zero &&) = delete;

  friend bool operator<(scalar_zero const &lhs, scalar_zero const &rhs);
  friend bool operator>(scalar_zero const &lhs, scalar_zero const &rhs);
  friend bool operator==(scalar_zero const &lhs, scalar_zero const &rhs);
  friend bool operator!=(scalar_zero const &lhs, scalar_zero const &rhs);

private:
  void update_hash_value() const override;
};

} // namespace numsim::cas

#endif // SCALAR_ZERO_H
