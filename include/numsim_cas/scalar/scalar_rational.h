#ifndef SCALAR_RATIONAL_H
#define SCALAR_RATIONAL_H

#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_rational final
    : public binary_op<scalar_node_base_t<scalar_rational>> {
public:
  using base = binary_op<scalar_node_base_t<scalar_rational>>;

  using base::base;
  scalar_rational() = delete;
  scalar_rational(scalar_rational &&data)
      : base(std::move(static_cast<base &&>(data))) {}
  scalar_rational(scalar_rational const &data)
      : base(static_cast<base const &>(data)) {}
  ~scalar_rational() = default;
  const scalar_rational &operator=(scalar_rational &&) = delete;
};

} // namespace numsim::cas

#endif // SCALAR_RATIONAL_H
