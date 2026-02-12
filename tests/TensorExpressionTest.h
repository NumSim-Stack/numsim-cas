#ifndef TENSOREXPRESSIONTEST_H
#define TENSOREXPRESSIONTEST_H

#include "gtest/gtest.h"
#include <numsim_cas/numsim_cas.h>

#include <cmath>
#include <cstddef>
#include <string>
#include <tuple>
#include <type_traits>

namespace {
using TestDims = ::testing::Types<std::integral_constant<std::size_t, 1>,
                                  std::integral_constant<std::size_t, 2>,
                                  std::integral_constant<std::size_t, 3>>;
} // namespace

template <typename DimTag> class TensorExpressionTest : public ::testing::Test {
protected:
  static constexpr std::size_t Dim = DimTag::value;

  using tensor_t =
      numsim::cas::expression_holder<numsim::cas::tensor_expression>;
  using scalar_t =
      numsim::cas::expression_holder<numsim::cas::scalar_expression>;

  TensorExpressionTest() {
    std::tie(X, Y, Z) = numsim::cas::make_tensor_variable(
        std::tuple{"X", Dim, 2}, std::tuple{"Y", Dim, 2},
        std::tuple{"Z", Dim, 2});

    std::tie(A, B, C) = numsim::cas::make_tensor_variable(
        std::tuple{"A", Dim, 4}, std::tuple{"B", Dim, 4},
        std::tuple{"C", Dim, 4});

    std::tie(x, y, z) = numsim::cas::make_scalar_variable("x", "y", "z");
    std::tie(a, b, c) = numsim::cas::make_scalar_variable("a", "b", "c");

    std::tie(_1, _2, _3) = numsim::cas::make_scalar_constant(1, 2, 3);

    _zero = scalar_t{numsim::cas::get_scalar_zero()};
    _one = scalar_t{numsim::cas::get_scalar_one()};

    _Zero = tensor_t{
        numsim::cas::make_expression<numsim::cas::tensor_zero>(Dim, 2)};
    _One = tensor_t{
        numsim::cas::make_expression<numsim::cas::kronecker_delta>(Dim)};
  }

  tensor_t X, Y, Z;
  tensor_t A, B, C;

  scalar_t x, y, z;
  scalar_t a, b, c;

  scalar_t _1, _2, _3;
  scalar_t _zero;
  scalar_t _one;

  tensor_t _Zero;
  tensor_t _One;
};

TYPED_TEST_SUITE(TensorExpressionTest, TestDims);

// -----------------------------------------------------------------------------
// Printing / canonicalization
// -----------------------------------------------------------------------------
TYPED_TEST(TensorExpressionTest, TensorExpressionTestPrint) {
  auto &X = this->X;
  auto &Y = this->Y;
  auto &Z = this->Z;
  auto &A = this->A;
  auto &Zero = this->_Zero;
  auto &One = this->_One;

  EXPECT_PRINT(One * X, "X");
  EXPECT_PRINT(One * A, "A");
  EXPECT_PRINT(Zero * X, "0");
  EXPECT_PRINT(One, "I");
  EXPECT_PRINT(Y, "Y");
  EXPECT_PRINT(Z, "Z");
  EXPECT_PRINT(X + X, "2*X");
  EXPECT_PRINT(Z + X + Y, "X+Y+Z");
  EXPECT_PRINT(Z + X + Y + X, "2*X+Y+Z");
  EXPECT_PRINT(Z + X + Y + Z, "X+Y+2*Z");
  EXPECT_PRINT(Z + X + Y + Y, "X+2*Y+Z");
  EXPECT_PRINT((Z + X + Y) + (Z + X + Y), "2*X+2*Y+2*Z");
  EXPECT_PRINT(X * X, "pow(X,2)");
  EXPECT_PRINT(X * X * X, "pow(X,3)");
  EXPECT_PRINT(Y * X, "Y*X");
  EXPECT_PRINT((Y * X) * X, "Y*pow(X,2)");
  EXPECT_PRINT(Y * X * Y, "Y*X*Y");
  EXPECT_PRINT(pow(X, 2) * X, "pow(X,3)");
  EXPECT_PRINT(X * pow(X, 2), "pow(X,3)");
  EXPECT_PRINT(X * (X * X), "pow(X,3)");
  EXPECT_PRINT((X * X) * X, "pow(X,3)");
}

TYPED_TEST(TensorExpressionTest, TensorScalarExpressionTestPrint) {
  auto &X = this->X;
  auto &Y = this->Y;
  auto &Z = this->Z;
  auto &x = this->x;
  auto &y = this->y;
  auto &z = this->z;
  auto &_1 = this->_1;
  auto &one = this->_one;

  EXPECT_PRINT(x * X, "x*X");
  EXPECT_PRINT(X * x, "x*X");
  EXPECT_PRINT(x * x * X, "pow(x,2)*X");
  EXPECT_PRINT(x * X * x, "pow(x,2)*X");
  EXPECT_PRINT(x * x * y * z * X, "pow(x,2)*y*z*X");
  EXPECT_PRINT(x * y * z * y * X, "x*pow(y,2)*z*X");
  EXPECT_PRINT(z * x * y * z * X, "x*y*pow(z,2)*X");
  EXPECT_PRINT(pow(x, x) * y * z * X, "pow(x,x)*y*z*X");

  EXPECT_PRINT(Y, "Y");
  EXPECT_PRINT(Z, "Z");
  EXPECT_PRINT(X + X, "2*X");
  EXPECT_PRINT(Z + X + Y, "X+Y+Z");
  EXPECT_PRINT(Z + X + Y + X, "2*X+Y+Z");
  EXPECT_PRINT(Z + X + Y + Z, "X+Y+2*Z");
  EXPECT_PRINT(Z + X + Y + Y, "X+2*Y+Z");

  EXPECT_PRINT(X / _1, "X");
  EXPECT_PRINT(X / 1, "X");
  EXPECT_PRINT(X * pow(x, -2), "X/pow(x,2)");
  EXPECT_PRINT(X / one, "X");
  EXPECT_PRINT(X / x, "X/x");
  EXPECT_PRINT(X / x / y, "X/(x*y)");
  EXPECT_PRINT((X / x) / y, "X/(x*y)");
  EXPECT_PRINT(X / (x * y * z), "X/(x*y*z)");
  EXPECT_PRINT(X / (x + y + z), "X/(x+y+z)");
  EXPECT_PRINT(X / (x * (y + z)), "X/(x*(y+z))");
}

TYPED_TEST(TensorExpressionTest, TensorSubtractionAndNegationPrint) {
  auto &X = this->X;
  auto &Y = this->Y;
  auto &Z = this->Z;

  EXPECT_PRINT(-X, "-X");
  EXPECT_PRINT(X - X, "0");
  EXPECT_PRINT(-X - X, "-2*X");
  EXPECT_PRINT(X - X - Y - Z, "-(Y+Z)");
  EXPECT_PRINT(-X - X - Y - Z, "-(2*X+Y+Z)");
  EXPECT_PRINT(-X - Y - Z - Y, "-(X+2*Y+Z)");
  EXPECT_PRINT(-X - Y - Z - Z, "-(X+Y+2*Z)");
}

// -----------------------------------------------------------------------------
// Inner/dot products (printing rules)
// -----------------------------------------------------------------------------
TYPED_TEST(TensorExpressionTest, TensorInnerproductPrint) {
  using seq = numsim::cas::sequence;
  // using numsim::cas::dot_product;
  using numsim::cas::inner_product;

  auto &X = this->X;
  auto &A = this->A;
  // auto &B = this->B;
  // auto &x = this->x;

  EXPECT_PRINT(inner_product(X, seq{2}, X, seq{1}), "X*X");
  EXPECT_PRINT(inner_product(A, seq{3, 4}, X, seq{1, 2}), "A:X");
  // EXPECT_PRINT(dot_product(A, seq{1, 2, 3, 4}, B, seq{1, 2, 3, 4}),
  //           "A::B");

  // EXPECT_PRINT(dot_product(A + B, seq{1, 2, 3, 4}, B + A,
  //                                      seq{1, 2, 3, 4}),
  //           "(A+B)::(A+B)");

  // EXPECT_PRINT(dot_product(A, seq{1, 2}, X, seq{1, 2}), "A:X");
  // EXPECT_PRINT(dot_product(A, seq{2, 1}, X, seq{1, 2}),
  //           "dot(A, [2,1], X, [1,2])");

  // EXPECT_PRINT(dot_product(A + X, seq{1, 2}, X * x, seq{1, 2}),
  //           "(A+X):x*X");

  // EXPECT_PRINT(dot_product(A, seq{1, 3, 2, 4}, B, seq{1, 2, 3, 4}),
  //           "dot(A, [1,3,2,4], B, [1,2,3,4])");

  // EXPECT_PRINT(dot_product(A + B, seq{1, 3, 2, 4}, B + A,
  //                                      seq{1, 2, 3, 4}),
  //           "dot(A+B, [1,3,2,4], A+B, [1,2,3,4])");
}

TYPED_TEST(TensorExpressionTest, TensorIdentityAndZeroMore) {
  auto &X = this->X;
  auto &A = this->A;
  auto &Zero = this->_Zero;
  auto &One = this->_One;

  EXPECT_PRINT(X * One, "X"); // right identity
  EXPECT_PRINT(One * X, "X"); // left identity
  EXPECT_PRINT(A * One, "A");
  EXPECT_PRINT(One * A, "A");

  EXPECT_PRINT(X + Zero, "X"); // additive identity
  EXPECT_PRINT(Zero + X, "X");

  EXPECT_PRINT(X * Zero, "0"); // annihilator (right)
  EXPECT_PRINT(Zero * X, "0"); // annihilator (left)
}

TYPED_TEST(TensorExpressionTest, TensorAdditionAssociativityCanonical) {
  auto &X = this->X;
  auto &Y = this->Y;
  auto &Z = this->Z;

  EXPECT_PRINT(X + (Y + Z), "X+Y+Z"); // flattening
  EXPECT_PRINT((X + Y) + Z, "X+Y+Z"); // associativity (print)
  EXPECT_PRINT(X + X + X, "3*X");     // coefficient collection
  EXPECT_PRINT(Z + Y + X + Y + Z + X, "2*X+2*Y+2*Z");
}

TYPED_TEST(TensorExpressionTest, TensorScalarCoeffCollection) {
  auto &X = this->X;
  auto &_2 = this->_2;
  auto &_3 = this->_3;
  auto &x = this->x;
  auto &y = this->y;

  EXPECT_PRINT(_2 * X + _3 * X, "5*X"); // numeric coeffs
  EXPECT_PRINT(x * X + x * X, "2*x*X"); // same symbolic coeff
  EXPECT_PRINT(y * x * X + x * y * X, "2*x*y*X");
}

TYPED_TEST(TensorExpressionTest, TensorProductOrderAndPowersMore) {
  auto &X = this->X;
  auto &Y = this->Y;
  auto &x = this->x;
  auto &y = this->y;

  EXPECT_PRINT(x * y * X * Y, "x*y*X*Y");
  EXPECT_PRINT(y * x * X * Y, "x*y*X*Y");

  // EXPECT_PRINT(pow(X, 2) * pow(X, 2), "pow(X,4)");
  // EXPECT_PRINT(pow(X, 2) * X * Y, "pow(X,3)*Y");
}

TYPED_TEST(TensorExpressionTest, TensorDivisionFormattingMore) {
  auto &X = this->X;
  auto &x = this->x;
  auto &y = this->y;

  EXPECT_PRINT(X / (y * x), "X/(x*y)");
  EXPECT_PRINT((X / x) / y, "X/(x*y)");
}

TYPED_TEST(TensorExpressionTest, TensorNegationCombinations) {
  auto &X = this->X;
  auto &Y = this->Y;

  EXPECT_PRINT(X + (-X), "0");
  EXPECT_PRINT((-X) + (-X), "-2*X");
  EXPECT_PRINT(-(X + Y), "-(X+Y)");
}

TYPED_TEST(TensorExpressionTest, TensorInnerProductScalarFactor) {
  // using seq = numsim::cas::sequence;
  // using numsim::cas::inner_product;

  // auto &X = this->X;
  // auto &x = this->x;

  //        // pull-through of scalar factors (no distribution over sums)
  // EXPECT_PRINT(inner_product(x * X, seq{2}, X, seq{1}), "x*X*X");
  // EXPECT_PRINT(inner_product(X, seq{2}, x * X, seq{1}), "x*X*X");
}

TYPED_TEST(TensorExpressionTest, TensorDotProductNoDistribution) {
  // using seq = numsim::cas::sequence;
  // using numsim::cas::dot_product;

  // auto &A = this->A;
  // auto &B = this->B;
  // auto &X = this->X;
  // auto &Y = this->Y;
  // auto &x = this->x;

  // EXPECT_PRINT(dot_product(A + B, seq{1, 2, 3, 4}, B,
  //                                      seq{1, 2, 3, 4}),
  //           "(A+B)::B");
  // EXPECT_PRINT(dot_product(A, seq{1, 2}, X + Y, seq{1, 2}),
  //           "A:(X+Y)");
  // EXPECT_PRINT(dot_product(A, seq{1, 2}, x * X, seq{1, 2}),
  //           "A:x*X");
}

TYPED_TEST(TensorExpressionTest, TensorDecomposition) {
  // using seq = numsim::cas::sequence;
  //  using numsim::cas::dot_product;

  // auto &A = this->A;
  // auto &B = this->B;
  auto &X = this->X;
  // auto &Y = this->Y;
  // auto &x = this->x;

  EXPECT_PRINT(numsim::cas::dev(X), "dev(X)");
  // EXPECT_PRINT(dot_product(A + B, seq{1, 2, 3, 4}, B,
  //                                      seq{1, 2, 3, 4}),
  //           "(A+B)::B");
  // EXPECT_PRINT(dot_product(A, seq{1, 2}, X + Y, seq{1, 2}),
  //           "A:(X+Y)");
  // EXPECT_PRINT(dot_product(A, seq{1, 2}, x * X, seq{1, 2}),
  //           "A:x*X");
}

// -----------------------------------------------------------------------------
// Differentiation
// -----------------------------------------------------------------------------
TYPED_TEST(TensorExpressionTest, TensorDiff) {
  // auto &X = this->X;
  // auto &Y = this->Y;
  // auto &x = this->x;

  // EXPECT_PRINT(numsim::cas::diff(X + X, X), "2*I{4}");
  // EXPECT_PRINT(numsim::cas::diff(X + Y, X), "I{4}");
  // EXPECT_PRINT(numsim::cas::diff(X + Y, Y), "I{4}");

  // EXPECT_PRINT(numsim::cas::diff(x * X, X), "x*I{4}");
  // EXPECT_PRINT(numsim::cas::diff(X * x, X), "x*I{4}");
  // EXPECT_PRINT(numsim::cas::diff(X / x, X), "I{4}/x");

  // EXPECT_PRINT(numsim::cas::diff(X * numsim::cas::dot(X, X),
  //           "dot(X)*I{4}+outer(X, [1,2], 2*X, [3,4])");

  // EXPECT_PRINT(numsim::cas::diff(X / numsim::cas::dot(X, X)),
  //           "I{4}/dot(X)+outer(X, [1,2], 2*X, [3,4])/pow(dot(X),2)");

  // if constexpr (TensorExpressionTest<TypeParam>::Dim == 1) {
  //   EXPECT_PRINT(numsim::cas::diff(numsim::cas::dev(X), X)),
  //             "I{4}+outer(I, [1,2], I, [3,4])");
  // } else {
  //   EXPECT_PRINT(numsim::cas::diff(numsim::cas::dev(X), X)),
  //             "I{4}+outer(I, [1,2], I, [3,4])/" +
  //                 std::to_string(TensorExpressionTest<TypeParam>::Dim));
  // }
}

#endif // TENSOREXPRESSIONTEST_H
