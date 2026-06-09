// Marker check for tmech version compatibility.
//
// numsim_cas requires the post-Sep-2025 `inverse_wrapper_base` rewrite of
// tmech (see #283/#248): without it, rank-4 `tmech::inv` silently returns
// NaN through the Voigt path. This file is a `try_compile` probe used by
// the top-level CMakeLists.txt to reject stale system installs that would
// otherwise shadow the pinned FetchContent copy.
//
// A compile failure here means the found tmech is too old; CMake emits a
// `FATAL_ERROR` pointing the user at `-DCMAKE_DISABLE_FIND_PACKAGE_tmech=TRUE`
// or a system-install upgrade.

#include <tmech/tmech.h>
#include <type_traits>

int main() {
  static_assert(
      std::is_class_v<tmech::detail::inverse_wrapper_base<double>>,
      "tmech::detail::inverse_wrapper_base missing — install predates the "
      "rank-4 inverse rewrite");
  return 0;
}
