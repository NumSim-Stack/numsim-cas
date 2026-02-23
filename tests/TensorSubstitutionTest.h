#ifndef TENSORSUBSTITUTIONTEST_H
#define TENSORSUBSTITUTIONTEST_H

#pragma once

#include "cas_test_helpers.h"
#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"

#include <numsim_cas/core/substitute.h>
#include <numsim_cas/tensor/visitors/tensor_substitution.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_substitution.h>

namespace {

using TensorSubTestDims =
    ::testing::Types<std::integral_constant<std::size_t, 1>,
                     std::integral_constant<std::size_t, 2>,
                     std::integral_constant<std::size_t, 3>>;

} // namespace

template <typename DimTag>
class TensorSubstitutionTest : public ::testing::Test {
protected:
  static constexpr std::size_t Dim = DimTag::value;

  using tensor_t =
      numsim::cas::expression_holder<numsim::cas::tensor_expression>;
  using scalar_t =
      numsim::cas::expression_holder<numsim::cas::scalar_expression>;
  using t2s_t =
      numsim::cas::expression_holder<numsim::cas::tensor_to_scalar_expression>;

  TensorSubstitutionTest() {
    std::tie(X, Y, Z) = numsim::cas::make_tensor_variable(
        std::tuple{"X", Dim, 2}, std::tuple{"Y", Dim, 2},
        std::tuple{"Z", Dim, 2});

    std::tie(x, y, z) = numsim::cas::make_scalar_variable("x", "y", "z");
  }

  tensor_t X, Y, Z;
  scalar_t x, y, z;
};

TYPED_TEST_SUITE(TensorSubstitutionTest, TensorSubTestDims);

// tensor -> tensor substitution: subs(X+Y, X, Z) -> Z+Y
TYPED_TEST(TensorSubstitutionTest, TensorForTensor) {
  auto &X = this->X;
  auto &Y = this->Y;
  auto &Z = this->Z;

  auto expr = X + Y;
  auto result = numsim::cas::substitute(expr, X, Z);
  auto expected = Z + Y;
  EXPECT_TRUE(result == expected)
      << "Expected " << testcas::S(expected) << ", got: " << testcas::S(result);
}

// scalar -> scalar in tensor: subs(x*X, x, y) -> y*X
TYPED_TEST(TensorSubstitutionTest, ScalarInTensor) {
  auto &X = this->X;
  auto &x = this->x;
  auto &y = this->y;

  auto expr = x * X;
  auto result = numsim::cas::substitute(expr, x, y);
  auto expected = y * X;
  EXPECT_TRUE(result == expected)
      << "Expected " << testcas::S(expected) << ", got: " << testcas::S(result);
}

// t2s -> t2s in tensor: subs(trace(X)*Y, trace(X), det(X)) -> det(X)*Y
TYPED_TEST(TensorSubstitutionTest, T2sInTensor) {
  auto &X = this->X;
  auto &Y = this->Y;

  auto trX = numsim::cas::trace(X);
  auto detX = numsim::cas::det(X);

  auto expr = numsim::cas::make_expression<
      numsim::cas::tensor_to_scalar_with_tensor_mul>(Y, trX);
  auto result = numsim::cas::substitute(expr, trX, detX);
  auto expected = numsim::cas::make_expression<
      numsim::cas::tensor_to_scalar_with_tensor_mul>(Y, detX);
  EXPECT_TRUE(result == expected)
      << "Expected " << testcas::S(expected) << ", got: " << testcas::S(result);
}

// Cross-domain depth: scalar sub reaches through t2s then tensor
// subs(trace(x*X)*Y, x, z)
TYPED_TEST(TensorSubstitutionTest, CrossDomainDepth) {
  auto &X = this->X;
  auto &Y = this->Y;
  auto &x = this->x;
  auto &z = this->z;

  auto inner = x * X;
  auto trInner = numsim::cas::trace(inner);
  auto expr = numsim::cas::make_expression<
      numsim::cas::tensor_to_scalar_with_tensor_mul>(Y, trInner);

  auto result = numsim::cas::substitute(expr, x, z);

  auto expectedInner = z * X;
  auto expectedTr = numsim::cas::trace(expectedInner);
  auto expected = numsim::cas::make_expression<
      numsim::cas::tensor_to_scalar_with_tensor_mul>(Y, expectedTr);
  EXPECT_TRUE(result == expected)
      << "Expected " << testcas::S(expected) << ", got: " << testcas::S(result);
}

#endif // TENSORSUBSTITUTIONTEST_H
