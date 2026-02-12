#ifndef MAKE_CONSTANT_H
#define MAKE_CONSTANT_H

#include <type_traits>
#include <utility>

namespace numsim::cas {

template <class Base> class expression_holder;

namespace detail {

// ADL anchor (usually you already have this somewhere)
void tag_invoke();

struct make_constant_fn {
  template <class Base, class T>
  constexpr auto operator()(std::type_identity<Base>, T &&v) const
      noexcept(noexcept(tag_invoke(*this, std::type_identity<Base>{},
                                   std::forward<T>(v)))) {
    return tag_invoke(*this, std::type_identity<Base>{}, std::forward<T>(v));
  }
};

inline constexpr make_constant_fn make_constant{};

template <class T> using decay_t = std::remove_cvref_t<T>;

template <class T>
inline constexpr bool is_arithmetic_v = std::is_arithmetic_v<decay_t<T>>;

} // namespace detail
} // namespace numsim::cas

#endif // MAKE_CONSTANT_H
