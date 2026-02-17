#ifndef MAKE_NEGATIVE_H
#define MAKE_NEGATIVE_H

#include <type_traits>

#include <numsim_cas/core/tag_invoke.h>
#include <numsim_cas/numsim_cas_forward.h>

namespace numsim::cas::detail {

struct neg_fn {
  template <class ExprBase>
  constexpr auto operator()(expression_holder<ExprBase> const &e) const
      noexcept(noexcept(tag_invoke(*this, std::type_identity<ExprBase>{}, e)))
          -> tag_invoke_result_t<neg_fn, std::type_identity<ExprBase>,
                                 expression_holder<ExprBase> const &>
  requires tag_invocable<neg_fn, std::type_identity<ExprBase>,
                         expression_holder<ExprBase> const &>
  {
    return tag_invoke(*this, std::type_identity<ExprBase>{}, e);
  }
};

inline constexpr neg_fn neg{};

} // namespace numsim::cas::detail

#endif // MAKE_NEGATIVE_H
