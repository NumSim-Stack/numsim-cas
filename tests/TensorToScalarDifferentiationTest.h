#ifndef TENSORTOSCALARDIFFERENTIATIONTEST_H
#define TENSORTOSCALARDIFFERENTIATIONTEST_H

#include "cas_test_helpers.h"
#include <gtest/gtest.h>

#include <numsim_cas/core/diff.h>
#include <numsim_cas/numsim_cas.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_diff.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_std.h>

namespace numsim::cas {

class TensorToScalarDifferentiationTest : public ::testing::Test {
protected:
  static constexpr std::size_t dim = 3;
  static constexpr std::size_t rank = 2;

  using t_expr = expression_holder<tensor_expression>;
  using t2s_expr = expression_holder<tensor_to_scalar_expression>;
  using s_expr = expression_holder<scalar_expression>;

  TensorToScalarDifferentiationTest() {
    std::tie(Y, X) = make_tensor_variable(std::tuple{"Y", dim, rank},
                                           std::tuple{"X", dim, rank});
    trY = trace(Y);
    trX = trace(X);
    nY = norm(Y);
    detY = det(Y);

    I = make_expression<kronecker_delta>(dim);
    Zero = make_expression<tensor_zero>(dim, rank);
  }

  t_expr Y, X;
  t_expr I;
  t_expr Zero;
  t2s_expr trY, trX, nY, detY;
};

// d(tr(Y))/d(Y) should produce identity (kronecker delta)
TEST_F(TensorToScalarDifferentiationTest, Node_Trace) {
  auto d = diff(trY, Y);
  EXPECT_TRUE(d.is_valid()) << "Expected valid result for trace diff";
  EXPECT_SAME_PRINT(d, I);
}

// d(tr(X))/d(Y) = zero (no dependency)
TEST_F(TensorToScalarDifferentiationTest, Node_Trace_NonDependencyGivesZero) {
  auto d = diff(trX, Y);
  EXPECT_SAME_PRINT(d, Zero);
}

// d(-tr(Y))/d(Y) should be -I
TEST_F(TensorToScalarDifferentiationTest, Node_Negative) {
  auto f = -trY;
  auto d = diff(f, Y);
  EXPECT_TRUE(d.is_valid()) << "Expected valid result for neg diff";
  EXPECT_SAME_PRINT(d, -I);
}

// d(tr(Y) + norm(Y))/d(Y) should be valid
TEST_F(TensorToScalarDifferentiationTest, Node_Add) {
  auto f = trY + nY;
  auto d = diff(f, Y);
  EXPECT_TRUE(d.is_valid()) << "Expected valid result for add diff";
}

// d(1)/d(Y) = zero
TEST_F(TensorToScalarDifferentiationTest, Node_OneIsZeroDerivative) {
  auto one = make_expression<tensor_to_scalar_one>();
  auto d = diff(one, Y);
  EXPECT_SAME_PRINT(d, Zero);
}

// d(0)/d(Y) = zero
TEST_F(TensorToScalarDifferentiationTest, Node_ZeroIsZeroDerivative) {
  auto zero = make_expression<tensor_to_scalar_zero>();
  auto d = diff(zero, Y);
  EXPECT_SAME_PRINT(d, Zero);
}

// d(log(tr(Y)))/d(Y) should be valid (chain rule)
TEST_F(TensorToScalarDifferentiationTest, Node_Log) {
  auto f = log(trY);
  auto d = diff(f, Y);
  EXPECT_TRUE(d.is_valid()) << "Expected valid result for log diff";
}

// d(det(Y))/d(Y) should be valid
TEST_F(TensorToScalarDifferentiationTest, Node_Det) {
  auto d = diff(detY, Y);
  EXPECT_TRUE(d.is_valid()) << "Expected valid result for det diff";
}

// d(dot(Y))/d(Y) should be valid and equal to 2*Y
TEST_F(TensorToScalarDifferentiationTest, Node_Dot) {
  auto f = dot(Y);
  auto d = diff(f, Y);
  EXPECT_TRUE(d.is_valid()) << "Expected valid result for dot diff";
  EXPECT_SAME_PRINT(d, 2 * Y);
}

// d(norm(Y))/d(Y) should be valid
TEST_F(TensorToScalarDifferentiationTest, Node_Norm) {
  auto d = diff(nY, Y);
  EXPECT_TRUE(d.is_valid()) << "Expected valid result for norm diff";
}

// Regression: apply must reset state between calls
TEST_F(TensorToScalarDifferentiationTest,
       Combo_ApplyMustResetStateBetweenCalls) {
  auto d1 = diff(trY, Y);
  EXPECT_SAME_PRINT(d1, I);

  auto d2 = diff(trX, Y);
  EXPECT_SAME_PRINT(d2, Zero);
}

} // namespace numsim::cas

#endif // TENSORTOSCALARDIFFERENTIATIONTEST_H
