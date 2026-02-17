#ifndef SCALAR_SUBSTITUTION_H
#define SCALAR_SUBSTITUTION_H

#include <numsim_cas/core/substitute.h>
#include <numsim_cas/scalar/visitors/scalar_rebuild_visitor.h>

namespace numsim::cas {

// Forward declare the tag_invoke overload for scalar/scalar substitution.
expression_holder<scalar_expression>
tag_invoke(detail::substitute_fn, std::type_identity<scalar_expression>,
           std::type_identity<scalar_expression>,
           expression_holder<scalar_expression> const &,
           expression_holder<scalar_expression> const &,
           expression_holder<scalar_expression> const &);

class scalar_substitution final : public scalar_rebuild_visitor {
public:
  scalar_substitution(expr_holder_t const &old_val,
                      expr_holder_t const &new_val)
      : m_old(old_val), m_new(new_val) {}

  expr_holder_t apply(expr_holder_t const &expr) noexcept override {
    if (expr.is_valid() && expr == m_old)
      return m_new;
    return scalar_rebuild_visitor::apply(expr);
  }

private:
  expr_holder_t m_old;
  expr_holder_t m_new;
};

inline expression_holder<scalar_expression>
tag_invoke(detail::substitute_fn, std::type_identity<scalar_expression>,
           std::type_identity<scalar_expression>,
           expression_holder<scalar_expression> const &expr,
           expression_holder<scalar_expression> const &old_val,
           expression_holder<scalar_expression> const &new_val) {
  scalar_substitution visitor(old_val, new_val);
  return visitor.apply(expr);
}

} // namespace numsim::cas

#endif // SCALAR_SUBSTITUTION_H
