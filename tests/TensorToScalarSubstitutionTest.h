#ifndef TENSORTOSCALARSUBSTITUTIONTEST_H
#define TENSORTOSCALARSUBSTITUTIONTEST_H

#pragma once

#include "cas_test_helpers.h"
#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"

#include <numsim_cas/core/substitute.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_substitution.h>

namespace {

using T2sSubTestDims =
    ::testing::Types<std::integral_constant<std::size_t, 1>,
                     std::integral_constant<std::size_t, 2>,
                     std::integral_constant<std::size_t, 3>>;

} // namespace

template <typename DimTag>
class TensorToScalarSubstitutionTest : public ::testing::Test {
protected:
  static constexpr std::size_t Dim = DimTag::value;

  using tensor_t =
      numsim::cas::expression_holder<numsim::cas::tensor_expression>;
  using scalar_t =
      numsim::cas::expression_holder<numsim::cas::scalar_expression>;
  using t2s_t =
      numsim::cas::expression_holder<numsim::cas::tensor_to_scalar_expression>;

  TensorToScalarSubstitutionTest() {
    std::tie(X, Y, Z) = numsim::cas::make_tensor_variable(
        std::tuple{"X", Dim, 2}, std::tuple{"Y", Dim, 2},
        std::tuple{"Z", Dim, 2});

    std::tie(x, y, z) = numsim::cas::make_scalar_variable("x", "y", "z");
  }

  tensor_t X, Y, Z;
  scalar_t x, y, z;
};

TYPED_TEST_SUITE(TensorToScalarSubstitutionTest, T2sSubTestDims);

// t2s -> t2s: subs(trace(X)+norm(X), trace(X), det(X)) -> det(X)+norm(X)
TYPED_TEST(TensorToScalarSubstitutionTest, T2sForT2s) {
  auto &X = this->X;

  auto trX = numsim::cas::trace(X);
  auto nX = numsim::cas::norm(X);
  auto detX = numsim::cas::det(X);

  auto expr = trX + nX;
  auto result = numsim::cas::substitute(expr, trX, detX);
  auto expected = detX + nX;
  EXPECT_TRUE(result == expected)
      << "Expected " << testcas::S(expected)
      << ", got: " << testcas::S(result);
}

// scalar -> scalar in t2s: subs(x+trace(X), x, y) -> y+trace(X) (via wrapper)
TYPED_TEST(TensorToScalarSubstitutionTest, ScalarInT2s) {
  auto &X = this->X;
  auto &x = this->x;
  auto &y = this->y;

  auto trX = numsim::cas::trace(X);
  auto expr = x + trX;
  auto result = numsim::cas::substitute(expr, x, y);
  auto expected = y + trX;
  EXPECT_TRUE(result == expected)
      << "Expected " << testcas::S(expected)
      << ", got: " << testcas::S(result);
}

// tensor -> tensor in t2s: subs(trace(X), X, Y) -> trace(Y)
TYPED_TEST(TensorToScalarSubstitutionTest, TensorInT2s) {
  auto &X = this->X;
  auto &Y = this->Y;

  auto expr = numsim::cas::trace(X);
  auto result = numsim::cas::substitute(expr, X, Y);
  auto expected = numsim::cas::trace(Y);
  EXPECT_TRUE(result == expected)
      << "Expected " << testcas::S(expected)
      << ", got: " << testcas::S(result);
}

#endif // TENSORTOSCALARSUBSTITUTIONTEST_H
