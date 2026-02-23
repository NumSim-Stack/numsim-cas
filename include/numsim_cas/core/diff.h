#ifndef DIFF_H
#define DIFF_H

#include <type_traits>

#include <numsim_cas/core/tag_invoke.h>
#include <numsim_cas/numsim_cas_forward.h>

namespace numsim::cas::detail {

struct diff_fn {
  // explicit typed call
  template <class ExprBase, class ArgBase>
  constexpr auto operator()(std::type_identity<ExprBase>,
                            std::type_identity<ArgBase>,
                            expression_holder<ExprBase> const &expr,
                            expression_holder<ArgBase> const &arg) const
      noexcept(noexcept(tag_invoke(*this, std::type_identity<ExprBase>{},
                                   std::type_identity<ArgBase>{}, expr, arg)))
          -> tag_invoke_result_t<diff_fn, std::type_identity<ExprBase>,
                                 std::type_identity<ArgBase>,
                                 expression_holder<ExprBase> const &,
                                 expression_holder<ArgBase> const &>
  requires tag_invocable<
      diff_fn, std::type_identity<ExprBase>, std::type_identity<ArgBase>,
      expression_holder<ExprBase> const &, expression_holder<ArgBase> const &>

  {
    return tag_invoke(*this, std::type_identity<ExprBase>{},
                      std::type_identity<ArgBase>{}, expr, arg);
  }

  // ergonomic call: diff(expr, arg)
  template <class ExprBase, class ArgBase>
  constexpr auto operator()(expression_holder<ExprBase> const &expr,
                            expression_holder<ArgBase> const &arg) const
      noexcept(noexcept((*this)(std::type_identity<ExprBase>{},
                                std::type_identity<ArgBase>{}, expr, arg)))
          -> decltype((*this)(std::type_identity<ExprBase>{},
                              std::type_identity<ArgBase>{}, expr, arg)) {
    return (*this)(std::type_identity<ExprBase>{},
                   std::type_identity<ArgBase>{}, expr, arg);
  }
};

inline constexpr diff_fn diff{};

} // namespace numsim::cas::detail

namespace numsim::cas {
using detail::diff;
} // namespace numsim::cas

#endif // DIFF_H
