#ifndef TENSOREXPRESSIONTEST_H
#define TENSOREXPRESSIONTEST_H

#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"

using TestTypesTensor =
    ::testing::Types<std::tuple<float, std::index_sequence<1>>,
                     std::tuple<float, std::index_sequence<2>>,
                     std::tuple<float, std::index_sequence<3>>,
                     std::tuple<int, std::index_sequence<1>>,
                     std::tuple<int, std::index_sequence<2>>,
                     std::tuple<int, std::index_sequence<3>>,
                     std::tuple<double, std::index_sequence<1>>,
                     std::tuple<double, std::index_sequence<2>>,
                     std::tuple<double, std::index_sequence<3>>
                     /*std::complex<double>*/>;

template <std::size_t Number>
constexpr inline long unsigned int
get(std::integer_sequence<long unsigned int, Number> const &) {
  return Number;
}

template <typename T> class TensorExpressionTest : public ::testing::Test {
protected:
  using value_type = std::decay_t<decltype(std::get<0>(T()))>;
  using sequence = decltype(std::get<1>(T()));
  using tensor_t = numsim::cas::expression_holder<
      numsim::cas::tensor_expression<value_type>>;
  using scalar_t = numsim::cas::expression_holder<
      numsim::cas::scalar_expression<value_type>>;

  TensorExpressionTest() {
    const auto Dim{get(std::get<1>(T()))};
    std::tie(X, Y, Z) = numsim::cas::make_tensor_variable<value_type>(
        std::tuple{"X", Dim, 2}, std::tuple{"Y", Dim, 2},
        std::tuple{"Z", Dim, 2});
    std::tie(A, B, C) = numsim::cas::make_tensor_variable<value_type>(
        std::tuple{"A", Dim, 4}, std::tuple{"B", Dim, 4},
        std::tuple{"C", Dim, 4});
    std::tie(x, y, z) =
        numsim::cas::make_scalar_variable<value_type>("x", "y", "z");
    std::tie(a, b, c) =
        numsim::cas::make_scalar_variable<value_type>("a", "b", "c");
    std::tie(_1, _2, _3) =
        numsim::cas::make_scalar_constant<value_type>(1, 2, 3);
  }

  tensor_t X, Y, Z;
  tensor_t A, B, C;
  scalar_t x, y, z;
  scalar_t a, b, c;
  scalar_t _1, _2, _3;
  scalar_t _zero{numsim::cas::get_scalar_zero<value_type>()};
  scalar_t _one{numsim::cas::get_scalar_one<value_type>()};
  tensor_t _Zero{
      numsim::cas::make_expression<numsim::cas::tensor_zero<value_type>>(
          get(std::get<1>(T())), 2)};
  tensor_t _One{
      numsim::cas::make_expression<numsim::cas::kronecker_delta<value_type>>(
          get(std::get<1>(T())))};
};

TYPED_TEST_SUITE(TensorExpressionTest, TestTypesTensor);

TYPED_TEST(TensorExpressionTest, TensorExpressionTestPrint) {
  auto &X = this->X;
  auto &Y = this->Y;
  auto &Z = this->Z;
  auto &A = this->A;
  //    auto& x = this->x;
  //    auto& y = this->y;
  //    auto& z = this->z;
  //    auto& _1 = this->_1;
  auto &_2 = this->_2;
  // auto& _3 = this->_3;
  auto &Zero{this->_Zero};
  auto &One{this->_One};

  EXPECT_EQ(std::to_string(One * X), "X");
  EXPECT_EQ(std::to_string(One * A), "A");
  EXPECT_EQ(std::to_string(Zero * X), "0");
  EXPECT_EQ(std::to_string(Zero * X), "0");
  EXPECT_EQ(std::to_string(One), "I");
  EXPECT_EQ(std::to_string(Y), "Y");
  EXPECT_EQ(std::to_string(Z), "Z");
  EXPECT_EQ(std::to_string(X + X), "2*X");
  EXPECT_EQ(std::to_string(Z + X + Y), "X+Y+Z");
  EXPECT_EQ(std::to_string(Z + X + Y + X), "2*X+Y+Z");
  EXPECT_EQ(std::to_string(Z + X + Y + Z), "X+Y+2*Z");
  EXPECT_EQ(std::to_string(Z + X + Y + Y), "X+2*Y+Z");
  EXPECT_EQ(std::to_string((Z + X + Y) + (Z + X + Y)), "2*X+2*Y+2*Z");
  EXPECT_EQ(std::to_string(X * X), "pow(X,2)");
  EXPECT_EQ(std::to_string(X * X * X), "pow(X,3)");
  EXPECT_EQ(std::to_string(Y * X), "Y*X");
  EXPECT_EQ(std::to_string((Y * X) * X), "Y*pow(X,2)");
  EXPECT_EQ(std::to_string(Y * X * Y), "Y*X*Y");
  EXPECT_EQ(std::to_string(std::pow(X, _2) * X), "pow(X,3)");
  EXPECT_EQ(std::to_string(X * std::pow(X, _2)), "pow(X,3)");
  EXPECT_EQ(std::to_string(X * (X * X)), "pow(X,3)");
  EXPECT_EQ(std::to_string((X * X) * X), "pow(X,3)");
}

TYPED_TEST(TensorExpressionTest, TensorScalarExpressionTestPrint) {
  auto &X = this->X;
  auto &Y = this->Y;
  auto &Z = this->Z;
  auto &x = this->x;
  auto &y = this->y;
  auto &z = this->z;
  auto &_1 = this->_1;
  // auto& _2 = this->_2; //auto& _3 = this->_3;
  //     auto& zero{this->_zero};
  auto &one{this->_one};

  EXPECT_EQ(std::to_string(x * X), "x*X");
  EXPECT_EQ(std::to_string(X * x), "x*X");
  EXPECT_EQ(std::to_string(x * x * X), "pow(x,2)*X");
  EXPECT_EQ(std::to_string(x * X * x), "pow(x,2)*X");
  EXPECT_EQ(std::to_string(x * x * y * z * X), "pow(x,2)*y*z*X");
  EXPECT_EQ(std::to_string(x * y * z * y * X), "x*pow(y,2)*z*X");
  EXPECT_EQ(std::to_string(z * x * y * z * X), "x*y*pow(z,2)*X");
  EXPECT_EQ(std::to_string(std::pow(x, x) * y * z * X), "pow(x,x)*y*z*X");
  EXPECT_EQ(std::to_string(Y), "Y");
  EXPECT_EQ(std::to_string(Z), "Z");
  EXPECT_EQ(std::to_string(X + X), "2*X");
  EXPECT_EQ(std::to_string(Z + X + Y), "X+Y+Z");
  EXPECT_EQ(std::to_string(Z + X + Y + X), "2*X+Y+Z");
  EXPECT_EQ(std::to_string(Z + X + Y + Z), "X+Y+2*Z");
  EXPECT_EQ(std::to_string(Z + X + Y + Y), "X+2*Y+Z");
  EXPECT_EQ(std::to_string(X / _1), "X");
  EXPECT_EQ(std::to_string(X / one), "X");
  EXPECT_EQ(std::to_string(X / x), "X/x");
  EXPECT_EQ(std::to_string(X / x / y), "X/(x*y)");
  EXPECT_EQ(std::to_string(X / (x * y * z)), "X/(x*y*z)");
}

TYPED_TEST(TensorExpressionTest, TensorSubtractionAndNegationPrint) {
  auto &X = this->X;
  auto &Y = this->Y;
  auto &Z = this->Z;
  EXPECT_EQ(std::to_string(-X), "-X");
  EXPECT_EQ(std::to_string(X - X), "0");
  EXPECT_EQ(std::to_string(-X - X), "-2*X");
  EXPECT_EQ(std::to_string(X - X - Y - Z), "-(Y+Z)");
  EXPECT_EQ(std::to_string(-X - X - Y - Z), "-(2*X+Y+Z)");
  EXPECT_EQ(std::to_string(-X - Y - Z - Y), "-(X+2*Y+Z)");
  EXPECT_EQ(std::to_string(-X - Y - Z - Z), "-(X+Y+2*Z)");
}

TYPED_TEST(TensorExpressionTest, TensorInnerproductPrint) {
  using seq = numsim::cas::sequence;
  using numsim::cas::dot_product;
  using numsim::cas::inner_product;

  auto &X = this->X;
  // auto& Y = this->Y;
  // auto& Z = this->Z;
  auto &A = this->A;
  auto &B = this->B;
  auto &x = this->x;

  EXPECT_EQ(std::to_string(inner_product(X, seq{2}, X, seq{1})), "X*X");
  EXPECT_EQ(std::to_string(inner_product(A, seq{3, 4}, X, seq{1, 2})), "A:X");
  EXPECT_EQ(std::to_string(dot_product(A, seq{1, 2, 3, 4}, B, seq{1, 2, 3, 4})),
            "A::B");
  EXPECT_EQ(std::to_string(
                dot_product(A + B, seq{1, 2, 3, 4}, B + A, seq{1, 2, 3, 4})),
            "(A+B)::(A+B)");
  EXPECT_EQ(std::to_string(dot_product(A, seq{1, 2}, X, seq{1, 2})), "A:X");
  EXPECT_EQ(std::to_string(dot_product(A, seq{2, 1}, X, seq{1, 2})),
            "dot(A, [2,1], X, [1,2])");
  EXPECT_EQ(std::to_string(dot_product(A + X, seq{1, 2}, X * x, seq{1, 2})),
            "(A+X):x*X");
  EXPECT_EQ(std::to_string(dot_product(A, seq{1, 3, 2, 4}, B, seq{1, 2, 3, 4})),
            "dot(A, [1,3,2,4], B, [1,2,3,4])");
  EXPECT_EQ(std::to_string(
                dot_product(A + B, seq{1, 3, 2, 4}, B + A, seq{1, 2, 3, 4})),
            "dot(A+B, [1,3,2,4], A+B, [1,2,3,4])");
}

TYPED_TEST(TensorExpressionTest, TensorIdentityAndZeroMore) {
  auto &X = this->X, &A = this->A;
  auto &Zero = this->_Zero, &One = this->_One;

  EXPECT_EQ(std::to_string(X * One), "X"); // right identity
  EXPECT_EQ(std::to_string(One * X), "X"); // left identity (already have)
  EXPECT_EQ(std::to_string(A * One), "A");
  EXPECT_EQ(std::to_string(One * A), "A");

  EXPECT_EQ(std::to_string(X + Zero), "X"); // additive identity
  EXPECT_EQ(std::to_string(Zero + X), "X");

  EXPECT_EQ(std::to_string(X * Zero), "0"); // annihilator (right)
  EXPECT_EQ(std::to_string(Zero * X), "0"); // annihilator (left)
}

TYPED_TEST(TensorExpressionTest, TensorAdditionAssociativityCanonical) {
  auto &X = this->X, &Y = this->Y, &Z = this->Z;

  EXPECT_EQ(std::to_string(X + (Y + Z)), "X+Y+Z"); // flattening
  EXPECT_EQ(std::to_string((X + Y) + Z), "X+Y+Z"); // associativity (print)
  EXPECT_EQ(std::to_string(X + X + X), "3*X");     // coefficient collection
  EXPECT_EQ(std::to_string(Z + Y + X + Y + Z + X),
            "2*X+2*Y+2*Z"); // ordering + grouping
}

TYPED_TEST(TensorExpressionTest, TensorScalarCoeffCollection) {
  auto &X = this->X;
  auto &_2 = this->_2, &_3 = this->_3;
  auto &x = this->x, &y = this->y;

  EXPECT_EQ(std::to_string(_2 * X + _3 * X), "5*X"); // numeric coeffs
  EXPECT_EQ(std::to_string(x * X + x * X), "2*x*X"); // same symbolic coeff
  EXPECT_EQ(std::to_string(y * x * X + x * y * X),
            "2*x*y*X"); // product canon + merge
}

TYPED_TEST(TensorExpressionTest, TensorProductOrderAndPowersMore) {
  auto &X = this->X, &Y = this->Y;
  auto &x = this->x, &y = this->y;
  auto &_2 = this->_2, &_3 = this->_3;

  EXPECT_EQ(std::to_string(x * y * X * Y),
            "x*y*X*Y"); // scalars before tensors; keep tensor order
  EXPECT_EQ(std::to_string(y * x * X * Y),
            "x*y*X*Y"); // canonical product ordering
  EXPECT_EQ(std::to_string(std::pow(X, _2) * std::pow(X, _2)),
            "pow(X,4)"); // power add
  EXPECT_EQ(std::to_string(std::pow(X, _2) * X * Y),
            "pow(X,3)*Y"); // assoc + pow bump
}

TYPED_TEST(TensorExpressionTest, TensorDivisionFormattingMore) {
  auto &X = this->X;
  auto &x = this->x, &y = this->y;

  EXPECT_EQ(std::to_string(X / (y * x)),
            "X/(x*y)"); // denominator canonicalization
  EXPECT_EQ(std::to_string((X / x) / y), "X/(x*y)"); // flatten chain
}

TYPED_TEST(TensorExpressionTest, TensorNegationCombinations) {
  auto &X = this->X, &Y = this->Y;

  EXPECT_EQ(std::to_string(X + (-X)), "0");       // additive inverse
  EXPECT_EQ(std::to_string((-X) + (-X)), "-2*X"); // negative factor combine
  EXPECT_EQ(std::to_string(-(X + Y)), "-(X+Y)");  // parentheses under
}

TYPED_TEST(TensorExpressionTest, TensorInnerProductScalarFactor) {
  using seq = numsim::cas::sequence;
  using numsim::cas::inner_product;

  auto &X = this->X;
  auto &x = this->x;

  // pull-through of scalar factors (no distribution over sums)
  EXPECT_EQ(std::to_string(inner_product(x * X, seq{2}, X, seq{1})), "x*X*X");
  EXPECT_EQ(std::to_string(inner_product(X, seq{2}, x * X, seq{1})), "x*X*X");
}

TYPED_TEST(TensorExpressionTest, TensorDotProductNoDistribution) {
  using seq = numsim::cas::sequence;
  using numsim::cas::dot_product;

  auto &A = this->A, &B = this->B, &X = this->X, &Y = this->Y;
  auto &x = this->x;

  EXPECT_EQ(
      std::to_string(dot_product(A + B, seq{1, 2, 3, 4}, B, seq{1, 2, 3, 4})),
      "(A+B)::B"); // keep grouping
  EXPECT_EQ(std::to_string(dot_product(A, seq{1, 2}, X + Y, seq{1, 2})),
            "A:(X+Y)"); // no distribution over +
  EXPECT_EQ(std::to_string(dot_product(A, seq{1, 2}, x * X, seq{1, 2})),
            "A:x*X"); // scalar factors pulled cleanly
}

#endif // TENSOREXPRESSIONTEST_H
