#ifndef SCALAR_SIGN_H
#define SCALAR_SIGN_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_sign final : public unary_op<scalar_node_base_t<scalar_sign>> {
public:
  using base = unary_op<scalar_node_base_t<scalar_sign>>;

  using base::base;
  scalar_sign(scalar_sign const &expr)
      : base(static_cast<base const &>(expr)) {}
  scalar_sign(scalar_sign &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  scalar_sign() = delete;
  ~scalar_sign() override = default;
  const scalar_sign &operator=(scalar_sign &&) = delete;

private:
};
} // namespace numsim::cas

#endif // SCALAR_SIGN_H
