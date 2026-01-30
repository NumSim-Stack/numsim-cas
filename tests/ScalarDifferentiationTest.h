#ifndef SCALARDIFFERENTIATIONTEST_H
#define SCALARDIFFERENTIATIONTEST_H

#pragma once

#include <gtest/gtest.h>

#include <numsim_cas/core/diff.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_diff.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>

namespace numsim::cas {

namespace {
using expr_t = expression_holder<scalar_expression>;

#define expect_is_zero(e)                                                      \
  EXPECT_TRUE(is_same<scalar_zero>(e)) << "Expected 0, got: " << to_string(e);

#define expect_is_one(e)                                                       \
  EXPECT_TRUE(is_same<scalar_one>(e)) << "Expected 1, got: " << to_string(e);

} // namespace

TEST(ScalarDifferentiation, VariableAndConstant) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  auto c = make_expression<scalar_constant>(5.0);

  expect_is_one(diff(x, x));
  expect_is_zero(diff(y, x));
  expect_is_zero(diff(c, x));
  expect_is_zero(diff(get_scalar_one(), x));
  expect_is_zero(diff(get_scalar_zero(), x));
}

TEST(ScalarDifferentiation, AddAndMul) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");

  // d/dx (x + y) = 1
  {
    auto d = diff(x + y, x);
    expect_is_one(d);
  }

  // d/dx (x * y) = y
  {
    auto d = diff(x * y, x);
    expect_is_zero(d - y);
  }

  // d/dx (x * x) = x + x (or 2*x, depending on simplification)
  {
    auto d = diff(x * x, x);
    std::cout << d << std::endl;
    std::cout << -x - x << std::endl;
    std::cout << d - x - x << std::endl;

    expect_is_zero(d - x - x);
  }
}

TEST(ScalarDifferentiation, DivisionRule) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");

  // d/dx (x / y) = 1 / y
  auto d = diff(x / y, x);
  expect_is_zero(d - (1 / y));
}

TEST(ScalarDifferentiation, TrigonometryChainRule) {
  auto x = make_expression<scalar>("x");

  // d/dx sin(x) = cos(x)
  {
    auto d = diff(sin(x), x);
    expect_is_zero(d - cos(x));
  }

  // d/dx cos(x) = -sin(x)
  {
    auto d = diff(cos(x), x);
    expect_is_zero(d + sin(x));
  }

  // d/dx tan(x) = (1/cos(x))^2
  {
    auto one = get_scalar_one();
    auto two = make_expression<scalar_constant>(2);
    auto expected = pow(1 / cos(x), two);

    auto d = diff(tan(x), x);
    expect_is_zero(d - expected);
  }
}

TEST(ScalarDifferentiation, PowConstantExponent) {
  auto x = make_expression<scalar>("x");

  auto three = make_expression<scalar_constant>(3.0);
  auto two = make_expression<scalar_constant>(2.0);

  // d/dx x^3 = 3 * x^2
  auto expr = pow(x, three);
  auto d = diff(expr, x);

  auto expected = three * pow(x, two);
  expect_is_zero(d - expected);
}

} // namespace numsim::cas

#endif // SCALARDIFFERENTIATIONTEST_H
