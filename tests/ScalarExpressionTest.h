#ifndef SCALAREXPRESSIONTEST_H
#define SCALAREXPRESSIONTEST_H

#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"

template <typename T> class ScalarExpressionTest : public ::testing::Test {
protected:
  using value_type = T;
  using expr_type =
      numsim::cas::expression_holder<numsim::cas::scalar_expression<T>>;

  ScalarExpressionTest() {
    std::tie(x, y, z) = numsim::cas::make_scalar_variable<T>("x", "y", "z");
    std::tie(a, b, c, d) =
        numsim::cas::make_scalar_variable<T>("a", "b", "c", "d");
    std::tie(_1, _2, _3) = numsim::cas::make_scalar_constant<T>(1, 2, 3);
  }

  expr_type x, y, z;
  expr_type a, b, c, d;
  expr_type _1, _2, _3;
  expr_type _zero{numsim::cas::get_scalar_zero<T>()};
  expr_type _one{numsim::cas::get_scalar_one<T>()};
};

using TestTypes =
    ::testing::Types<double, float, int /*, std::complex<double>*/>;
TYPED_TEST_SUITE(ScalarExpressionTest, TestTypes);

TYPED_TEST(ScalarExpressionTest, ScalarFundamentalsPrint) {
  auto &x = this->x;
  auto &y = this->y;
  auto &z = this->z;
  auto &_1 = this->_1;
  auto &_2 = this->_2;
  auto &_3 = this->_3;
  auto &zero{this->_zero};
  auto &one{this->_one};
  EXPECT_EQ(std::to_string(_1), "1");
  EXPECT_EQ(std::to_string(_1 + _2 + _3), "6");
  EXPECT_EQ(std::to_string(zero + x), "x");
  EXPECT_EQ(std::to_string(x + zero), "x");
  EXPECT_EQ(std::to_string(zero + (x + y + z)), "x+y+z");
  EXPECT_EQ(std::to_string((x + y + z) + zero), "x+y+z");

  EXPECT_EQ(std::to_string(zero * one), "0");
  EXPECT_EQ(std::to_string(one * zero), "0");
  EXPECT_EQ(std::to_string(zero * x), "0");
  EXPECT_EQ(std::to_string(x * zero), "0");
  EXPECT_EQ(std::to_string(zero * (x + y + z)), "0");
  EXPECT_EQ(std::to_string((x + y + z) * zero), "0");

  EXPECT_EQ(std::to_string(one * x), "x");
  EXPECT_EQ(std::to_string(x * one), "x");
  EXPECT_EQ(std::to_string(one * (x + y + z)), "x+y+z");
  EXPECT_EQ(std::to_string((x + y + z) * one), "x+y+z");
}

TYPED_TEST(ScalarExpressionTest, ScalarAdditionPrint) {
  auto &x = this->x;
  auto &y = this->y;
  auto &z = this->z;
  auto &_1 = this->_1;
  auto &_2 = this->_2;
  auto &_3 = this->_3;
  EXPECT_EQ(std::to_string(_2 + x), "2+x");
  EXPECT_EQ(std::to_string(x + _2), "2+x");
  EXPECT_EQ(std::to_string(_1 + x + _3), "4+x");
  EXPECT_EQ(std::to_string(_1 + x + _3 + y + _2), "6+x+y");
  EXPECT_EQ(std::to_string(x + x), "2*x");
  EXPECT_EQ(std::to_string(x + x + _1), "1+2*x");
  EXPECT_EQ(std::to_string(x + x + _1 + y), "1+2*x+y");
  EXPECT_EQ(std::to_string((x + y + z) + (x + y + z)), "2*x+2*y+2*z");
}

TYPED_TEST(ScalarExpressionTest, ScalarMultiplicationPrint) {
  auto &x = this->x;
  auto &y = this->y;
  auto &_1 = this->_1;
  auto &_2 = this->_2;
  auto &_3 = this->_3;
  EXPECT_EQ(std::to_string(_2 * x + x), "3*x");
  EXPECT_EQ(std::to_string(x + _2 * x), "3*x");
  EXPECT_EQ(std::to_string(y * x + x), "x+x*y");
  EXPECT_EQ(std::to_string(y * x + x * y), "2*x*y");
  EXPECT_EQ(std::to_string(_2 * y * x + _3 * x * y), "5*x*y");
  EXPECT_EQ(std::to_string(_2 * y * x + _3 * x * y + _1 * x * y), "6*x*y");
}

TYPED_TEST(ScalarExpressionTest, ScalarDivPrint) {
  auto &x = this->x;
  auto &y = this->y;
  auto &a = this->a;
  auto &b = this->b;
  auto &c = this->c;
  auto &d = this->d;
  auto &_1 = this->_1;
  auto &_2 = this->_2; // auto& _3 = this->_3;
  if constexpr (std::is_floating_point_v<
                    typename decltype(this->x)::value_type>) {
    EXPECT_EQ(std::to_string(_1 / _2), "0.5");
  } else {
    EXPECT_EQ(std::to_string(_1 / _2), "1/2");
  }
  EXPECT_EQ(std::to_string(x / y), "x/y");
  EXPECT_EQ(std::to_string(a / b / c / d), "a/(b*c*d)");
  EXPECT_EQ(std::to_string((a / b) / (c / d)), "a*d/(b*c)");
  EXPECT_EQ(std::to_string((a + b) / (c + d)), "(a+b)/(c+d)");
}

TYPED_TEST(ScalarExpressionTest, ScalarSubtractionAndNegationPrint) {
  auto &x = this->x;
  auto &y = this->y;
  auto &z = this->z;
  EXPECT_EQ(std::to_string(-x), "-x");
  EXPECT_EQ(std::to_string(x - x), "0");
  EXPECT_EQ(std::to_string(-x - x), "-2*x");
  EXPECT_EQ(std::to_string(x - x - y - z), "-(y+z)");
  EXPECT_EQ(std::to_string(-x - x - y - z), "-(2*x+y+z)");
  EXPECT_EQ(std::to_string(-x - y - z - y), "-(x+2*y+z)");
  EXPECT_EQ(std::to_string(-x - y - z - z), "-(x+y+2*z)");
}

TYPED_TEST(ScalarExpressionTest, ScalarMixedOperationsPrint) {
  auto &x = this->x;
  auto &y = this->y;
  auto &z = this->z;
  auto &_1 = this->_1;
  auto &_2 = this->_2;
  auto &_3 = this->_3;
  EXPECT_EQ(std::to_string(x * _2), "2*x");
  EXPECT_EQ(std::to_string(x * _2 + x), "3*x");
  EXPECT_EQ(std::to_string((x * _2 + x) + x + _2), "2+4*x");
  EXPECT_EQ(std::to_string(_2 * (x + y + z)), "2*(x+y+z)");
  EXPECT_EQ(std::to_string(_2 * _3), "6");
  EXPECT_EQ(std::to_string(_2 * _3 * x), "6*x");
  EXPECT_EQ(std::to_string(x * _2 * _3), "6*x");
  EXPECT_EQ(std::to_string(_2 * x * _3), "6*x");
  EXPECT_EQ(std::to_string(x * x), "pow(x,2)");
  EXPECT_EQ(std::to_string(x * x * x), "pow(x,3)");
  EXPECT_EQ(std::to_string(x * x * x * x), "pow(x,4)");
  EXPECT_EQ(std::to_string((x + y + z) + (x + y + z)), "2*x+2*y+2*z");
  EXPECT_EQ(std::to_string(std::pow(x, _2) * x), "pow(x,3)");
  EXPECT_EQ(std::to_string(std::pow(x, _2) * std::pow(x, _2)), "pow(x,4)");
  EXPECT_EQ(std::to_string(std::pow(x, x) * x), "pow(x,1+x)");
  EXPECT_EQ(std::to_string(std::pow(x, _1 + x) * x), "pow(x,2+x)");
}

TYPED_TEST(ScalarExpressionTest, ScalarFunctionPrint) {
  [[maybe_unused]] auto &x = this->x;
  [[maybe_unused]] auto &y = this->y;
  [[maybe_unused]] auto &z = this->z;
  [[maybe_unused]] auto &_1 = this->_1;
  [[maybe_unused]] auto &_2 = this->_2;
  [[maybe_unused]] auto &_3 = this->_3;

  EXPECT_EQ(std::to_string(std::pow(x, _2) * x), "pow(x,3)");
  EXPECT_EQ(std::to_string(std::pow(x, _2) * std::pow(x, _2)), "pow(x,4)");
  EXPECT_EQ(std::to_string(std::pow(x, x) * x), "pow(x,1+x)");
  EXPECT_EQ(std::to_string(std::pow(x, _1 + x) * x), "pow(x,2+x)");

  EXPECT_EQ(std::to_string(std::cos(x)), "cos(x)");
  EXPECT_EQ(std::to_string(std::sin(x)), "sin(x)");
  EXPECT_EQ(std::to_string(std::tan(x)), "tan(x)");
  EXPECT_EQ(std::to_string(std::exp(x)), "exp(x)");
}

TYPED_TEST(ScalarExpressionTest, ScalarFundamentalDerivatives) {
  auto &x = this->x;
  auto &y = this->y;
  auto &z = this->z;
  auto &_1 = this->_1;
  auto &zero{this->_zero};
  auto &one{this->_one};
  EXPECT_EQ(std::to_string(numsim::cas::diff(zero, x)), "0");
  EXPECT_EQ(std::to_string(numsim::cas::diff(one, x)), "0");
  EXPECT_EQ(std::to_string(numsim::cas::diff(_1, x)), "0");
  EXPECT_EQ(std::to_string(numsim::cas::diff(x, x)), "1");
  EXPECT_EQ(std::to_string(numsim::cas::diff(x * y * z, x)), "y*z");
  // EXPECT_EQ(std::to_string(numsim::cas::diff(x/x, x)), "2");
  EXPECT_EQ(std::to_string(numsim::cas::diff(x + x, x)), "2");
  EXPECT_EQ(std::to_string(numsim::cas::diff(x * x, x)), "2*x");
  EXPECT_EQ(std::to_string(numsim::cas::diff(x * x * x, x)), "3*pow(x,2)");
  EXPECT_EQ(std::to_string(numsim::cas::diff(x * x * x * x, x)), "4*pow(x,3)");

  //  EXPECT_EQ(std::to_string(numsim::cas::diff(-x - x - y - z, x)), "-2");
  //  EXPECT_EQ(std::to_string(numsim::cas::diff(-x - x - y - z, y)), "-1");
  //  EXPECT_EQ(std::to_string(numsim::cas::diff(-x - x - y - z, z)), "-1");
}

#endif // SCALAREXPRESSIONTEST_H
