#ifndef SCALAR_NAMED_EXPRESSION_H
#define SCALAR_NAMED_EXPRESSION_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_named_expression final
    : public unary_op<scalar_node_base_t<scalar_named_expression>> {
public:
  using base = unary_op<scalar_node_base_t<scalar_named_expression>>;
  using base::base;
  using expression = expression_holder<typename base::expr_t>;

  scalar_named_expression() : base(), m_name() {}
  scalar_named_expression(std::string const &name, expression &&expr)
      : base(std::move(expr)), m_name(name) {}
  scalar_named_expression(std::string const &name, expression const &expr)
      : base(expr), m_name(name) {}

  auto &name() { return m_name; }
  const auto &name() const { return m_name; }

private:
  std::string m_name;
};
} // namespace numsim::cas

#endif // SCALAR_NAMED_EXPRESSION_H
