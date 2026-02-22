#ifndef SCALAR_NEGATIVE_H
#define SCALAR_NEGATIVE_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_negative final
    : public unary_op<scalar_node_base_t<scalar_negative>> {
public:
  using base = unary_op<scalar_node_base_t<scalar_negative>>;

  using base::base;
  scalar_negative(scalar_negative &&data) noexcept
      : base(std::move(static_cast<base &&>(data))) {}
  scalar_negative(scalar_negative const &data)
      : base(static_cast<base const &>(data)) {}
  scalar_negative() = delete;
  ~scalar_negative() override = default;
  const scalar_negative &operator=(scalar_negative &&) = delete;
};

} // namespace numsim::cas

#endif // SCALAR_NEGATIVE_H
