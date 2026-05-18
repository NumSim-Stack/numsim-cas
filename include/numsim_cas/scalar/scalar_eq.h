#ifndef SCALAR_EQ_H
#define SCALAR_EQ_H

#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

// Scalar numerical equality `a == b`. Evaluates to 1.0 / 0.0.
// Distinct from `expression_holder::operator==`, which compares the
// symbolic AST structure (hash-based). `scalar_eq(a, b)` is a symbolic
// node that *represents* the numerical predicate "a equals b at runtime".
//
// Constructed via `eq(a, b)`.
class scalar_eq final : public binary_op<scalar_node_base_t<scalar_eq>> {
public:
  using base = binary_op<scalar_node_base_t<scalar_eq>>;

  using base::base;
  scalar_eq(scalar_eq const &expr) : base(static_cast<base const &>(expr)) {}
  scalar_eq(scalar_eq &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  scalar_eq() = delete;
  ~scalar_eq() override = default;
  const scalar_eq &operator=(scalar_eq &&) = delete;
};

} // namespace numsim::cas

#endif // SCALAR_EQ_H
