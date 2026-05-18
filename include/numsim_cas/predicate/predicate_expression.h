#ifndef PREDICATE_EXPRESSION_H
#define PREDICATE_EXPRESSION_H

#include <numsim_cas/core/expression.h>
#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/predicate/predicate_visitor_typedef.h>

namespace numsim::cas {

// Base type for the predicate (boolean) domain. Predicates carry a logical
// truth value; they have no arithmetic and are not differentiable. They
// flow into `if_then_else` as the condition, and they're produced by
// comparison operators (lt, gt, eq, ...) and logical combinators (and,
// or, not) added in subsequent PRs.
class predicate_expression : public expression {
public:
  using expr_t = predicate_expression;
  using visitor_t = predicate_visitor_t;
  using visitor_const_t = predicate_visitor_const_t;

  template <typename... Args>
  requires(sizeof...(Args) != 1 ||
           !std::is_same_v<std::remove_cvref_t<Args>..., predicate_expression>)
  predicate_expression(Args &&...args)
      : expression(std::forward<Args>(args)...) {}
  predicate_expression(predicate_expression const &data)
      : expression(static_cast<expression const &>(data)) {}
  predicate_expression(predicate_expression &&data) noexcept
      : expression(std::move(static_cast<expression &&>(data))) {}
  ~predicate_expression() override = default;
  const predicate_expression &operator=(predicate_expression const &) = delete;
};

template <class T>
concept predicate_expr_holder =
    requires { typename std::remove_cvref_t<T>::expr_type; } &&
    std::is_base_of_v<predicate_expression,
                      typename std::remove_cvref_t<T>::expr_type>;

} // namespace numsim::cas

#endif // PREDICATE_EXPRESSION_H
