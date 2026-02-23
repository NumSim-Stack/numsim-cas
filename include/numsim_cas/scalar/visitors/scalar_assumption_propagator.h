#ifndef SCALAR_ASSUMPTION_PROPAGATOR_H
#define SCALAR_ASSUMPTION_PROPAGATOR_H

#include <numsim_cas/core/assumptions.h>
#include <numsim_cas/scalar/scalar_all.h>

namespace numsim::cas {

class scalar_assumption_propagator final : public scalar_visitor_const_t {
public:
  using expr_holder_t = expression_holder<scalar_expression>;

  numeric_assumption_manager apply(expr_holder_t const &expr);

  // Leaf nodes
  void operator()(scalar const &) override;
  void operator()(scalar_zero const &) override;
  void operator()(scalar_one const &) override;
  void operator()(scalar_constant const &) override;

  // Arithmetic
  void operator()(scalar_add const &) override;
  void operator()(scalar_mul const &) override;
  void operator()(scalar_negative const &) override;
  void operator()(scalar_pow const &) override;
  // Functions
  void operator()(scalar_sin const &) override;
  void operator()(scalar_cos const &) override;
  void operator()(scalar_tan const &) override;
  void operator()(scalar_exp const &) override;
  void operator()(scalar_log const &) override;
  void operator()(scalar_sqrt const &) override;
  void operator()(scalar_abs const &) override;
  void operator()(scalar_sign const &) override;
  void operator()(scalar_asin const &) override;
  void operator()(scalar_acos const &) override;
  void operator()(scalar_atan const &) override;
  void operator()(scalar_named_expression const &) override;

private:
  numeric_assumption_manager m_result;
};

/// Convenience: propagate assumptions bottom-up through the expression tree.
numeric_assumption_manager
propagate_assumptions(expression_holder<scalar_expression> const &expr);

} // namespace numsim::cas

#endif // SCALAR_ASSUMPTION_PROPAGATOR_H
