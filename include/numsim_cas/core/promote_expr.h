#ifndef PROMOTE_EXPR_H
#define PROMOTE_EXPR_H

#include <numsim_cas/core/tag_invoke.h>
#include <type_traits>
#include <utility>

namespace numsim::cas::detail {

// Promote expression_holder<From> -> expression_holder<To>
struct promote_expr_fn {
  template <class ToBase, class Expr>
  requires tag_invocable<promote_expr_fn, std::type_identity<ToBase>, Expr &&>
  constexpr auto operator()(std::type_identity<ToBase>, Expr &&expr) const
      noexcept(noexcept(tag_invoke(*this, std::type_identity<ToBase>{},
                                   std::forward<Expr>(expr))))
          -> tag_invoke_result_t<promote_expr_fn, std::type_identity<ToBase>,
                                 Expr &&> {
    return tag_invoke(*this, std::type_identity<ToBase>{},
                      std::forward<Expr>(expr));
  }
};

inline constexpr promote_expr_fn promote_expr{};

} // namespace numsim::cas::detail

#endif // PROMOTE_EXPR_H
