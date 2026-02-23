#ifndef CAS_TEST_HELPERS_H
#define CAS_TEST_HELPERS_H

#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"

#include <cmath>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>

namespace testcas {

template <class E> inline std::string S(const E &e) {
  using std::to_string;
  if constexpr (requires { to_string(e); }) {
    return to_string(e);
  } else {
    std::ostringstream oss;
    oss << e;
    return oss.str();
  }
}

// --- Handy assertions -------------------------------------------------------
#ifndef EXPECT_PRINT
#define EXPECT_PRINT(expr, expected)                                           \
  EXPECT_EQ(::testcas::S((expr)), std::string(expected))
#endif

#ifndef EXPECT_SAME_PRINT
#define EXPECT_SAME_PRINT(lhs, rhs)                                            \
  EXPECT_EQ(::testcas::S((lhs)), ::testcas::S((rhs)))
#endif

} // namespace testcas

#endif // CAS_TEST_HELPERS_H
