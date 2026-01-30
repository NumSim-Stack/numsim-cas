#ifndef TYPE_LIST_H
#define TYPE_LIST_H

#include <cstdint>
#include <numsim_cas/numsim_cas_forward.h>
#include <type_traits>

namespace numsim::cas {

template <class... Ts> struct type_list {};

template <class T, class List> struct index_of;

template <class T, class... Ts>
struct index_of<T, type_list<T, Ts...>>
    : std::integral_constant<std::uint16_t, 0> {};

template <class T, class U, class... Ts>
struct index_of<T, type_list<U, Ts...>>
    : std::integral_constant<std::uint16_t,
                             1 + index_of<T, type_list<Ts...>>::value> {};

template <class T, class List>
inline constexpr std::uint16_t index_of_v = index_of<T, List>::value;

// Unique ID mapping
template <typename T, typename Base> struct get_index; // primary template

template <typename Type> struct get_derived {
  using derived_type = Type;
};

template <typename Derived, typename... Args>
struct get_derived<unary_op<Derived, Args...>> {
  using derived_type = Derived;
};

template <typename Derived, typename... Args>
struct get_derived<binary_op<Derived, Args...>> {
  using derived_type = Derived;
};

} // namespace numsim::cas

#endif // TYPE_LIST_H
