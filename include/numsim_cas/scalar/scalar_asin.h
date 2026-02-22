#ifndef SCALAR_ASIN_H
#define SCALAR_ASIN_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_asin final : public unary_op<scalar_node_base_t<scalar_asin>> {
public:
  using base = unary_op<scalar_node_base_t<scalar_asin>>;

  using base::base;
  scalar_asin(scalar_asin const &expr)
      : base(static_cast<base const &>(expr)) {}
  scalar_asin(scalar_asin &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  scalar_asin() = delete;
  ~scalar_asin() override = default;
  const scalar_asin &operator=(scalar_asin &&) = delete;

private:
};
} // namespace numsim::cas

#endif // SCALAR_ASIN_H
