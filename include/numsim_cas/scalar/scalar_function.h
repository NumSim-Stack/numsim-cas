#ifndef SCALAR_FUNCTION_H
#define SCALAR_FUNCTION_H

#include "../expression_holder.h"
#include "../unary_op.h"
#include "scalar_expression.h"

namespace numsim::cas {

template <typename ValueType>
class scalar_function final : public unary_op<scalar_function<ValueType>,
                                              scalar_expression<ValueType>> {
public:
  using value_type = ValueType;
  using base =
      unary_op<scalar_function<ValueType>, scalar_expression<ValueType>>;
  using base::base;
  using expression = expression_holder<typename base::expr_type>;

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
