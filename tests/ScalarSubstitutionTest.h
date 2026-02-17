#ifndef SCALARSUBSTITUTIONTEST_H
#define SCALARSUBSTITUTIONTEST_H

#pragma once

#include <gtest/gtest.h>

#include <numsim_cas/core/substitute.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/scalar/visitors/scalar_substitution.h>

namespace numsim::cas {

TEST(ScalarSubstitution, LeafMatch) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");

  // subs(x, x, y) -> y
  auto result = substitute(x, x, y);
  EXPECT_TRUE(result == y) << "Expected y, got: " << to_string(result);
}

TEST(ScalarSubstitution, NoMatch) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  auto z = make_expression<scalar>("z");

  // subs(x, y, z) -> x
  auto result = substitute(x, y, z);
  EXPECT_TRUE(result == x) << "Expected x, got: " << to_string(result);
}

TEST(ScalarSubstitution, UnarySin) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");

  // subs(sin(x), x, y) -> sin(y)
  auto result = substitute(sin(x), x, y);
  auto expected = sin(y);
  EXPECT_TRUE(result == expected)
      << "Expected " << to_string(expected) << ", got: " << to_string(result);
}

TEST(ScalarSubstitution, BinaryPow) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  auto z = make_expression<scalar>("z");

  // subs(pow(x, y), x, z) -> pow(z, y)
  auto result = substitute(pow(x, y), x, z);
  auto expected = pow(z, y);
  EXPECT_TRUE(result == expected)
      << "Expected " << to_string(expected) << ", got: " << to_string(result);
}

TEST(ScalarSubstitution, NaryAdd) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  auto z = make_expression<scalar>("z");
  auto a = make_expression<scalar>("a");

  // subs(x+y+z, y, a) -> x+a+z
  auto result = substitute(x + y + z, y, a);
  auto expected = x + a + z;
  EXPECT_TRUE(result == expected)
      << "Expected " << to_string(expected) << ", got: " << to_string(result);
}

TEST(ScalarSubstitution, NaryMul) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  auto z = make_expression<scalar>("z");
  auto a = make_expression<scalar>("a");

  // subs(x*y*z, y, a) -> x*a*z
  auto result = substitute(x * y * z, y, a);
  auto expected = x * a * z;
  EXPECT_TRUE(result == expected)
      << "Expected " << to_string(expected) << ", got: " << to_string(result);
}

TEST(ScalarSubstitution, Nested) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  auto a = make_expression<scalar>("a");
  auto b = make_expression<scalar>("b");

  // subs(sin(x*y), x, a+b) -> sin((a+b)*y)
  auto result = substitute(sin(x * y), x, a + b);
  auto expected = sin((a + b) * y);
  EXPECT_TRUE(result == expected)
      << "Expected " << to_string(expected) << ", got: " << to_string(result);
}

TEST(ScalarSubstitution, CompositeMatch) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");

  // subs(sin(x)+cos(x), sin(x), y) -> y+cos(x)
  auto result = substitute(sin(x) + cos(x), sin(x), y);
  auto expected = y + cos(x);
  EXPECT_TRUE(result == expected)
      << "Expected " << to_string(expected) << ", got: " << to_string(result);
}

TEST(ScalarSubstitution, ConstantSubstitution) {
  auto x = make_expression<scalar>("x");
  auto zero = get_scalar_zero();

  // subs(x+1, x, 0) -> 1
  auto expr = x + make_expression<scalar_constant>(1);
  auto result = substitute(expr, x, zero);
  auto expected = make_expression<scalar_constant>(1);
  EXPECT_TRUE(result == expected)
      << "Expected " << to_string(expected) << ", got: " << to_string(result);
}

} // namespace numsim::cas

#endif // SCALARSUBSTITUTIONTEST_H
