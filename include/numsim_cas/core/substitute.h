#ifndef SUBSTITUTE_H
#define SUBSTITUTE_H

#include <type_traits>

#include <numsim_cas/core/tag_invoke.h>
#include <numsim_cas/numsim_cas_forward.h>

namespace numsim::cas::detail {

// Domains that attach assumptions to symbols overload this for their holder
// type (found by ADL); this fallback covers the rest.
template <class TargetBase>
inline void validate_substitution(expression_holder<TargetBase> const &,
                                  expression_holder<TargetBase> const &) {}

struct substitute_fn {
  // explicit typed call — skips the assumption check, so the substitution
  // visitors use it to recurse into children
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

  // ergonomic call: substitute(expr, old, new) — the public entry, which
  // rejects a replacement that does not carry the assumptions asserted on
  // the symbol it replaces
  template <class ExprBase, class TargetBase>
  auto operator()(expression_holder<ExprBase> const &expr,
                  expression_holder<TargetBase> const &old_val,
                  expression_holder<TargetBase> const &new_val) const
      -> decltype((*this)(std::type_identity<ExprBase>{},
                          std::type_identity<TargetBase>{}, expr, old_val,
                          new_val)) {
    validate_substitution(old_val, new_val);
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
