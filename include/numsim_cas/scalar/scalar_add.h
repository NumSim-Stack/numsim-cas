#ifndef SCALAR_ADD_H
#define SCALAR_ADD_H

#include <numsim_cas/core/n_ary_tree.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_add final : public n_ary_tree<scalar_node_base_t<scalar_add>> {
public:
  using base = n_ary_tree<scalar_node_base_t<scalar_add>>;
  using base::base;
  using base::expr_t;

  scalar_add() : base() {}
  template <typename... Expr>
  scalar_add(Expr &&...expr) : base(std::forward<Expr>(expr)...) {}
  ~scalar_add() = default;
  scalar_add(scalar_add const &add) : base(static_cast<base const &>(add)) {}
  scalar_add(scalar_add &&add) : base(std::forward<base>(add)) {}
  const scalar_add &operator=(scalar_add &&) = delete;
};
} // namespace numsim::cas

#endif // SCALAR_ADD_H
