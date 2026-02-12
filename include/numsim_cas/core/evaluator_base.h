#ifndef EVALUATOR_BASE_H
#define EVALUATOR_BASE_H

#include <any>
#include <exception>
#include <map>
#include <memory>
#include <numsim_cas/core/expression.h>
#include <numsim_cas/core/expression_holder.h>
#include <stdexcept>

namespace numsim::cas {

template <typename ValueType> class evaluator_base {
public:
  using value_type = ValueType;

  template <typename ExprBase>
  void set(expression_holder<ExprBase> const &symbol, value_type val) {
    m_symbols_to_value[to_base_holder(symbol)] = val;
  }

protected:
  void dispatch() noexcept {
    try {
      m_result =
          std::any_cast<value_type>(m_symbols_to_value.at(m_current_expr));
    } catch (...) {
      m_exception = std::current_exception();
    }
  }

  void rethrow_if_needed() {
    if (m_exception) {
      auto ex = m_exception;
      m_exception = nullptr;
      std::rethrow_exception(ex);
    }
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
  std::exception_ptr m_exception{nullptr};
};

} // namespace numsim::cas

#endif // EVALUATOR_BASE_H
