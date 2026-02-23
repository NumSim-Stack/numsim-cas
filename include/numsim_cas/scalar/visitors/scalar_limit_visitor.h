#ifndef SCALAR_LIMIT_VISITOR_H
#define SCALAR_LIMIT_VISITOR_H

#include <numsim_cas/core/limit_algebra.h>
#include <numsim_cas/core/limit_result.h>
#include <numsim_cas/scalar/scalar_all.h>

namespace numsim::cas {

class scalar_limit_visitor final : public scalar_visitor_const_t,
                                   protected limit_algebra {
public:
  using expr_holder_t = expression_holder<scalar_expression>;

  scalar_limit_visitor(expr_holder_t const &limit_var, limit_target target);

  limit_result apply(expr_holder_t const &expr);

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
  expr_holder_t m_limit_var;
  limit_target m_target;
  limit_result m_result;
};

} // namespace numsim::cas

#endif // SCALAR_LIMIT_VISITOR_H
