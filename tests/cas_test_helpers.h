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

// --- ADL-friendly stringifier ----------------------------------------------
// Prefers numsim::cas::to_string(e). If not found, falls back to streaming.
template <class E> inline std::string S(const E &e) {
  // Try ADL to_string in numsim::cas, otherwise stream.
  using std::to_string;
  if constexpr (requires { to_string(e); }) {
    return to_string(e);
  } else {
    std::ostringstream oss;
    oss << e; // requires operator<<(std::ostream&, E)
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

// ---------- Scalar fixture--------------------------------------------------
// template <typename T>
// struct ScalarFixture : ::testing::Test {
//   using value_type = T;
//   using scalar_expr =
//       numsim::cas::expression_holder<numsim::cas::scalar_expression<T>>;

//   scalar_expr x, y, z;
//   scalar_expr a, b, c, d;
//   scalar_expr _1, _2, _3;
//   scalar_expr _zero{numsim::cas::get_scalar_zero<T>()};
//   scalar_expr _one{numsim::cas::get_scalar_one<T>()};

//   ScalarFixture() {
//     std::tie(x, y, z) = numsim::cas::make_scalar_variable<T>("x", "y", "z");
//     std::tie(a, b, c, d) =
//         numsim::cas::make_scalar_variable<T>("a", "b", "c", "d");
//     std::tie(_1, _2, _3) = numsim::cas::make_scalar_constant<T>(1, 2, 3);
//   }
// };

// // ---------- Tensor fixture
// --------------------------------------------------
// // Provides: 2nd-order X,Y,Z; 4th-order A,B,C; scalars x,y,z,a,b,c,_1,_2,_3;
// // identity I (rank-2), and Zero2 (rank-2 zero) for given Dim.
// template <typename T, std::size_t Dim> struct TensorFixture : ::testing::Test
// {
//   using value_type = T;
//   using tensor_expr =
//       numsim::cas::expression_holder<numsim::cas::tensor_expression<T>>;
//   using scalar_expr =
//       numsim::cas::expression_holder<numsim::cas::scalar_expression<T>>;

//   // 2nd order tensors
//   tensor_expr X, Y, Z;
//   // 4th order tensors
//   tensor_expr A, B, C;

//   // scalars
//   scalar_expr x, y, z, a, b, c, _1, _2, _3;
//   scalar_expr _zero{numsim::cas::get_scalar_zero<T>()};
//   scalar_expr _one{numsim::cas::get_scalar_one<T>()};

//   // canonical identities
//   tensor_expr I{
//       numsim::cas::make_expression<numsim::cas::kronecker_delta<T>>(Dim)};
//   tensor_expr Zero2{
//       numsim::cas::make_expression<numsim::cas::tensor_zero<T>>(Dim, 2)};

//   TensorFixture() {
//     std::tie(X, Y, Z) = numsim::cas::make_tensor_variable<T>(
//         std::tuple{"X", Dim, 2}, std::tuple{"Y", Dim, 2},
//         std::tuple{"Z", Dim, 2});
//     std::tie(A, B, C) = numsim::cas::make_tensor_variable<T>(
//         std::tuple{"A", Dim, 4}, std::tuple{"B", Dim, 4},
//         std::tuple{"C", Dim, 4});
//     std::tie(x, y, z) = numsim::cas::make_scalar_variable<T>("x", "y", "z");
//     std::tie(a, b, c) = numsim::cas::make_scalar_variable<T>("a", "b", "c");
//     std::tie(_1, _2, _3) = numsim::cas::make_scalar_constant<T>(1, 2, 3);
//   }
// };

} // namespace testcas

#endif // CAS_TEST_HELPERS_H
