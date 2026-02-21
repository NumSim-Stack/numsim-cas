#ifndef SUBSTITUTE_H
#define SUBSTITUTE_H

#include <type_traits>

#include <numsim_cas/core/tag_invoke.h>
#include <numsim_cas/numsim_cas_forward.h>

namespace numsim::cas::detail {

struct substitute_fn {
  // explicit typed call
  template <class ExprBase, class TargetBase>
  constexpr auto operator()(std::type_identity<ExprBase>,
                            std::type_identity<TargetBase>,
                            expression_holder<ExprBase> const &expr,
                            expression_holder<TargetBase> const &old_val,
                            expression_holder<TargetBase> const &new_val) const
      noexcept(noexcept(tag_invoke(*this, std::type_identity<ExprBase>{},
                                   std::type_identity<TargetBase>{}, expr,
                                   old_val, new_val)))
          -> tag_invoke_result_t<substitute_fn, std::type_identity<ExprBase>,
                                 std::type_identity<TargetBase>,
                                 expression_holder<ExprBase> const &,
                                 expression_holder<TargetBase> const &,
                                 expression_holder<TargetBase> const &>
  requires tag_invocable<substitute_fn, std::type_identity<ExprBase>,
                         std::type_identity<TargetBase>,
                         expression_holder<ExprBase> const &,
                         expression_holder<TargetBase> const &,
                         expression_holder<TargetBase> const &>
  {
    return tag_invoke(*this, std::type_identity<ExprBase>{},
                      std::type_identity<TargetBase>{}, expr, old_val, new_val);
  }

  // ergonomic call: substitute(expr, old, new)
  template <class ExprBase, class TargetBase>
  constexpr auto operator()(expression_holder<ExprBase> const &expr,
                            expression_holder<TargetBase> const &old_val,
                            expression_holder<TargetBase> const &new_val) const
      noexcept(noexcept((*this)(std::type_identity<ExprBase>{},
                                std::type_identity<TargetBase>{}, expr, old_val,
                                new_val)))
          -> decltype((*this)(std::type_identity<ExprBase>{},
                              std::type_identity<TargetBase>{}, expr, old_val,
                              new_val)) {
    return (*this)(std::type_identity<ExprBase>{},
                   std::type_identity<TargetBase>{}, expr, old_val, new_val);
  }
};

inline constexpr substitute_fn substitute{};

} // namespace numsim::cas::detail

namespace numsim::cas {
using detail::substitute;
} // namespace numsim::cas

#endif // SUBSTITUTE_H
