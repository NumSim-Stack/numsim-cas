#ifndef TENSOREXPRESSIONTEST_H
#define TENSOREXPRESSIONTEST_H

#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"

using TestTypes =
    ::testing::Types<double, float, int /*, std::complex<double>*/>;

template <typename T> class TensorExpressionTest : public ::testing::Test {
protected:
  using tensor_t =
      numsim::cas::expression_holder<numsim::cas::tensor_expression<T>>;
  using scalar_t =
      numsim::cas::expression_holder<numsim::cas::scalar_expression<T>>;

  TensorExpressionTest() {
    std::tie(X, Y, Z) = numsim::cas::make_tensor_variable<T>(
        std::tuple{"X", 3, 2}, std::tuple{"Y", 3, 2}, std::tuple{"Z", 3, 2});
    std::tie(A, B, C) = numsim::cas::make_tensor_variable<T>(
        std::tuple{"A", 3, 4}, std::tuple{"B", 3, 4}, std::tuple{"C", 3, 4});
    std::tie(x, y, z) = numsim::cas::make_scalar_variable<T>("x", "y", "z");
    std::tie(a, b, c) = numsim::cas::make_scalar_variable<T>("a", "b", "c");
    std::tie(_1, _2, _3) = numsim::cas::make_scalar_constant<T>(1, 2, 3);
  }

  tensor_t X, Y, Z;
  tensor_t A, B, C;
  scalar_t x, y, z;
  scalar_t a, b, c;
  scalar_t _1, _2, _3;
  scalar_t _zero{numsim::cas::get_scalar_zero<T>()};
  scalar_t _one{numsim::cas::get_scalar_one<T>()};
};

TYPED_TEST_SUITE(TensorExpressionTest, TestTypes);

TYPED_TEST(TensorExpressionTest, TensorExpressionTestPrint) {
  auto &X = this->X;
  auto &Y = this->Y;
  auto &Z = this->Z;
  //    auto& x = this->x;
  //    auto& y = this->y;
  //    auto& z = this->z;
  //    auto& _1 = this->_1;
  auto &_2 = this->_2;
  // auto& _3 = this->_3;
  //     auto& zero{this->_zero};
  //     auto& one{this->_one};

  EXPECT_EQ(std::to_string(X), "X");
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
  EXPECT_EQ(std::to_string(Y * X), "X*Y");
  EXPECT_EQ(std::to_string(Y * X * Y), "X*pow(Y,2)");
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
  auto &X = this->X;
  // auto& Y = this->Y;
  // auto& Z = this->Z;
  auto &A = this->A;
  auto &B = this->B;
  auto &x = this->x;

  EXPECT_EQ(std::to_string(numsim::cas::inner_product(X, seq{2}, X, seq{1})),
            "X*X");
  EXPECT_EQ(
      std::to_string(numsim::cas::inner_product(A, seq{3, 4}, X, seq{1, 2})),
      "A:X");
  EXPECT_EQ(std::to_string(numsim::cas::dot_product(A, seq{1, 2, 3, 4}, B,
                                                    seq{1, 2, 3, 4})),
            "A::B");
  EXPECT_EQ(std::to_string(numsim::cas::dot_product(A + B, seq{1, 2, 3, 4},
                                                    B + A, seq{1, 2, 3, 4})),
            "(A+B)::(A+B)");
  EXPECT_EQ(
      std::to_string(numsim::cas::dot_product(A, seq{1, 2}, X, seq{1, 2})),
      "A:X");
  EXPECT_EQ(
      std::to_string(numsim::cas::dot_product(A, seq{2, 1}, X, seq{1, 2})),
      "dot(A, [2,1], X, [1,2])");
  EXPECT_EQ(std::to_string(
                numsim::cas::dot_product(A + X, seq{1, 2}, X * x, seq{1, 2})),
            "(A+X):x*X");
  EXPECT_EQ(std::to_string(numsim::cas::dot_product(A, seq{1, 3, 2, 4}, B,
                                                    seq{1, 2, 3, 4})),
            "dot(A, [1,3,2,4], B, [1,2,3,4])");
  EXPECT_EQ(std::to_string(numsim::cas::dot_product(A + B, seq{1, 3, 2, 4},
                                                    B + A, seq{1, 2, 3, 4})),
            "dot(A+B, [1,3,2,4], A+B, [1,2,3,4])");
}

#endif // TENSOREXPRESSIONTEST_H
