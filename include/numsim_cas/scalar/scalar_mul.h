#ifndef SCALAR_MUL_H
#define SCALAR_MUL_H

#include <numsim_cas/core/n_ary_tree.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_mul final : public n_ary_tree<scalar_node_base_t<scalar_mul>> /*,
public expression_crtp<scalar_mul, scalar_expression>*/
{
public:
  using base = n_ary_tree<scalar_node_base_t<scalar_mul>>;
  // using base_op = n_ary_tree<scalar_expression,
  // scalar_mul>; using base_expr =
  // expression_crtp<scalar_mul, scalar_expression>;

  using base::base;

  scalar_mul() : base() {}
  scalar_mul(scalar_mul const &mul) : base(static_cast<base const &>(mul)) {}
  scalar_mul(scalar_mul &&mul) : base(std::forward<base>(mul)) {}
  ~scalar_mul() = default;

  const scalar_mul &operator=(scalar_mul &&) = delete;
};

} // namespace numsim::cas

#endif // SCALAR_MUL_H
