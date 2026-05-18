#ifndef PREDICATE_EVALUATOR_H
#define PREDICATE_EVALUATOR_H

#include <numsim_cas/predicate/predicate_definitions.h>

namespace numsim::cas {

// Evaluator for the predicate domain. Returns a bool. Used at runtime to
// reduce predicates to a definite truth value — e.g. inside an
// `if_then_else` evaluator that needs to pick a branch, or in tests.
//
// Leaf-only for PR 1; comparison nodes added in #136 will read scalar
// values from a companion scalar_evaluator (composition similar to how
// tensor_to_scalar_evaluator delegates to scalar_evaluator).
class predicate_evaluator final : public predicate_visitor_const_t {
public:
  predicate_evaluator() = default;
  predicate_evaluator(predicate_evaluator const &) = delete;
  predicate_evaluator(predicate_evaluator &&) = delete;
  predicate_evaluator &operator=(predicate_evaluator const &) = delete;

  bool apply(expression_holder<predicate_expression> const &expr) {
    if (!expr.is_valid()) {
      // An invalid predicate has no truth value — default to false to
      // match the scalar evaluator's "default to zero" convention for
      // invalid inputs.
      return false;
    }
    static_cast<predicate_visitable_t const &>(expr.get())
        .accept(static_cast<predicate_visitor_const_t &>(*this));
    return m_result;
  }

  void operator()([[maybe_unused]] predicate_true const &) override {
    m_result = true;
  }

  void operator()([[maybe_unused]] predicate_false const &) override {
    m_result = false;
  }

private:
  bool m_result = false;
};

} // namespace numsim::cas

#endif // PREDICATE_EVALUATOR_H
