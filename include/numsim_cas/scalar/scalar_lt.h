#ifndef SCALAR_LT_H
#define SCALAR_LT_H

#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

// Scalar less-than comparison `a < b`. Evaluates to 1.0 when the
// comparison holds and 0.0 otherwise — the indicator-function
// representation that lets `(a < b) * x` work as a natural symbolic
// pattern for damage activation, smoothed yield surfaces, etc.
//
// Constructed via the free function `lt(a, b)` declared in scalar_std.h —
// not via `operator<`, which is reserved for the hash-based ordering on
// expression_holder used by std::map and the n_ary_tree simplifier.
class scalar_lt final : public binary_op<scalar_node_base_t<scalar_lt>> {
public:
  using base = binary_op<scalar_node_base_t<scalar_lt>>;

  using base::base;
  scalar_lt(scalar_lt const &expr) : base(static_cast<base const &>(expr)) {}
  scalar_lt(scalar_lt &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
  scalar_lt() = delete;
  ~scalar_lt() override = default;
  const scalar_lt &operator=(scalar_lt &&) = delete;
};

} // namespace numsim::cas

#endif // SCALAR_LT_H
