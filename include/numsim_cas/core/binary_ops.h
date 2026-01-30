#ifndef BINARY_OPS_H
#define BINARY_OPS_H

#include <numsim_cas/core/tag_invoke.h>
#include <utility>

namespace numsim::cas::detail {

struct add_fn {
  template <class L, class R>
  requires tag_invocable<add_fn, L &&, R &&>
  constexpr auto operator()(L &&l, R &&r) const
      noexcept(noexcept(tag_invoke(*this, std::forward<L>(l),
                                   std::forward<R>(r))))
          -> tag_invoke_result_t<add_fn, L &&, R &&> {
    return tag_invoke(*this, std::forward<L>(l), std::forward<R>(r));
  }
};
struct sub_fn {
  template <class L, class R>
  requires tag_invocable<sub_fn, L &&, R &&>
  constexpr auto operator()(L &&l, R &&r) const
      noexcept(noexcept(tag_invoke(*this, std::forward<L>(l),
                                   std::forward<R>(r))))
          -> tag_invoke_result_t<sub_fn, L &&, R &&> {
    return tag_invoke(*this, std::forward<L>(l), std::forward<R>(r));
  }
};
struct mul_fn {
  template <class L, class R>
  requires tag_invocable<mul_fn, L &&, R &&>
  constexpr auto operator()(L &&l, R &&r) const
      noexcept(noexcept(tag_invoke(*this, std::forward<L>(l),
                                   std::forward<R>(r))))
          -> tag_invoke_result_t<mul_fn, L &&, R &&> {
    return tag_invoke(*this, std::forward<L>(l), std::forward<R>(r));
  }
};
struct div_fn {
  template <class L, class R>
  requires tag_invocable<div_fn, L &&, R &&>
  constexpr auto operator()(L &&l, R &&r) const
      noexcept(noexcept(tag_invoke(*this, std::forward<L>(l),
                                   std::forward<R>(r))))
          -> tag_invoke_result_t<div_fn, L &&, R &&> {
    return tag_invoke(*this, std::forward<L>(l), std::forward<R>(r));
  }
};

inline constexpr add_fn binary_add{};
inline constexpr sub_fn binary_sub{};
inline constexpr mul_fn binary_mul{};
inline constexpr div_fn binary_div{};

} // namespace numsim::cas::detail

#endif // BINARY_OPS_H
