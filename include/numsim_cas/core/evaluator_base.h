#ifndef EVALUATOR_BASE_H
#define EVALUATOR_BASE_H

#include <any>
#include <map>
#include <memory>
#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/core/expression.h>
#include <numsim_cas/core/expression_holder.h>

namespace numsim::cas {

template <typename ValueType> class evaluator_base {
public:
  using value_type = ValueType;

  virtual ~evaluator_base() = default;

  template <typename ExprBase>
  void set(expression_holder<ExprBase> const &symbol, value_type val) {
    m_symbols_to_value[to_base_holder(symbol)] = val;
  }

protected:
  void dispatch() {
    auto it = m_symbols_to_value.find(m_current_expr);
    if (it == m_symbols_to_value.end()) {
      throw evaluation_error("evaluator_base: symbol not found");
    }
    m_result = std::any_cast<value_type>(it->second);
  }

  template <typename ExprBase>
  static expression_holder<expression>
  to_base_holder(expression_holder<ExprBase> const &h) {
    return expression_holder<expression>(
        std::static_pointer_cast<expression>(h.data()));
  }

  std::map<expression_holder<expression>, std::any> m_symbols_to_value;
  value_type m_result{};
  expression_holder<expression> m_current_expr;
};

} // namespace numsim::cas

#endif // EVALUATOR_BASE_H
