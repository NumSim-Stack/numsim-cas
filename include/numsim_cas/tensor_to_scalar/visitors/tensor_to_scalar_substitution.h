#ifndef TENSOR_TO_SCALAR_SUBSTITUTION_H
#define TENSOR_TO_SCALAR_SUBSTITUTION_H

#include <numsim_cas/core/substitute.h>
#include <numsim_cas/scalar/visitors/scalar_substitution.h>
#include <numsim_cas/tensor/visitors/tensor_substitution.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_rebuild_visitor.h>

namespace numsim::cas {

// Forward declare t2s tag_invoke overloads.
template <class TargetBase>
expression_holder<tensor_to_scalar_expression>
tag_invoke(detail::substitute_fn,
           std::type_identity<tensor_to_scalar_expression>,
           std::type_identity<TargetBase>,
           expression_holder<tensor_to_scalar_expression> const &,
           expression_holder<TargetBase> const &,
           expression_holder<TargetBase> const &);

template <typename TargetBase>
class tensor_to_scalar_substitution final
    : public tensor_to_scalar_rebuild_visitor {
public:
  using target_holder_t = expression_holder<TargetBase>;

  tensor_to_scalar_substitution(target_holder_t const &old_val,
                                target_holder_t const &new_val)
      : m_old(old_val), m_new(new_val) {}

  t2s_holder_t apply(t2s_holder_t const &expr) override {
    if constexpr (std::is_same_v<TargetBase, tensor_to_scalar_expression>) {
      if (expr.is_valid() && expr == m_old)
        return m_new;
    }
    return tensor_to_scalar_rebuild_visitor::apply(expr);
  }

  scalar_holder_t
  apply_scalar(scalar_holder_t const &expr) override {
    if constexpr (std::is_same_v<TargetBase, scalar_expression>) {
      return substitute(expr, m_old, m_new);
    } else {
      return expr;
    }
  }

  tensor_holder_t
  apply_tensor(tensor_holder_t const &expr) override {
    if constexpr (std::is_same_v<TargetBase, tensor_expression>) {
      return substitute(expr, m_old, m_new);
    } else if constexpr (std::is_same_v<TargetBase, scalar_expression>) {
      return substitute(expr, m_old, m_new);
    } else {
      return expr;
    }
  }

private:
  target_holder_t m_old;
  target_holder_t m_new;
};

template <class TargetBase>
inline expression_holder<tensor_to_scalar_expression>
tag_invoke(detail::substitute_fn,
           std::type_identity<tensor_to_scalar_expression>,
           std::type_identity<TargetBase>,
           expression_holder<tensor_to_scalar_expression> const &expr,
           expression_holder<TargetBase> const &old_val,
           expression_holder<TargetBase> const &new_val) {
  tensor_to_scalar_substitution<TargetBase> visitor(old_val, new_val);
  return visitor.apply(expr);
}

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_SUBSTITUTION_H
