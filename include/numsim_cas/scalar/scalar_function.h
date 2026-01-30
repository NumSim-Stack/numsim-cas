#ifndef SCALAR_FUNCTION_H
#define SCALAR_FUNCTION_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_function final
    : public unary_op<scalar_node_base_t<scalar_function>> {
public:
  using base = unary_op<scalar_node_base_t<scalar_function>>;
  using base::base;
  using expression = expression_holder<typename base::expr_t>;

  scalar_function() : base(), m_name() {}
  scalar_function(std::string const &name, expression &&expr)
      : base(std::move(expr)), m_name(name) {}
  scalar_function(std::string const &name, expression const &expr)
      : base(expr), m_name(name) {}

  auto &name() { return m_name; }
  const auto &name() const { return m_name; }

private:
  std::string m_name;
};
} // namespace numsim::cas

#endif // SCALAR_FUNCTION_H
