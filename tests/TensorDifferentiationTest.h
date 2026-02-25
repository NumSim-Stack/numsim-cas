#ifndef TENSORDIFFERENTIATIONTEST_H
#define TENSORDIFFERENTIATIONTEST_H

#include "cas_test_helpers.h"
#include <gtest/gtest.h>

#include <numsim_cas/core/diff.h>
#include <numsim_cas/numsim_cas.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor/tensor_std.h>

namespace numsim::cas {

class TensorDifferentiationTest : public ::testing::Test {
protected:
  static constexpr std::size_t dim = 3;
  static constexpr std::size_t rank = 2;

  using tensor_t = expression_holder<tensor_expression>;
  using scalar_t = expression_holder<scalar_expression>;

  TensorDifferentiationTest() {
    std::tie(X, Y) = make_tensor_variable(std::tuple{"X", dim, rank},
                                          std::tuple{"Y", dim, rank});
    I = make_expression<kronecker_delta>(dim);
    Zero = make_expression<tensor_zero>(dim, rank);
    Zero4 = make_expression<tensor_zero>(dim, 4);
  }

  tensor_t X, Y;
  tensor_t I;
  tensor_t Zero;
  tensor_t Zero4;
};

// d(X)/d(X) = identity_tensor
TEST_F(TensorDifferentiationTest, VariableSelf) {
  auto d = diff(X, X);
  EXPECT_TRUE(is_same<identity_tensor>(d))
      << "Expected identity_tensor, got: " << to_string(d);
}

// d(Y)/d(X) = zero
TEST_F(TensorDifferentiationTest, VariableOther) {
  auto d = diff(Y, X);
  EXPECT_TRUE(is_same<tensor_zero>(d))
      << "Expected tensor_zero, got: " << to_string(d);
}

// d(X + Y)/d(X) = identity_tensor
TEST_F(TensorDifferentiationTest, AdditionRule) {
  auto d = diff(X + Y, X);
  EXPECT_TRUE(is_same<identity_tensor>(d))
      << "Expected identity_tensor, got: " << to_string(d);
}

// d(-X)/d(X) = -identity_tensor
TEST_F(TensorDifferentiationTest, NegationRule) {
  auto d = diff(-X, X);
  EXPECT_TRUE(d.is_valid()) << "Expected valid result";
  // Should be negative of identity_tensor
  EXPECT_TRUE(is_same<tensor_negative>(d))
      << "Expected tensor_negative, got: " << to_string(d);
}

// d(2*X)/d(X) = 2 * identity_tensor
TEST_F(TensorDifferentiationTest, ScalarMulRule) {
  auto two = make_expression<scalar_constant>(2.0);
  auto d = diff(two * X, X);
  EXPECT_TRUE(d.is_valid()) << "Expected valid result";
}

// d(Zero)/d(X) = zero
TEST_F(TensorDifferentiationTest, ZeroDerivative) {
  auto d = diff(Zero, X);
  EXPECT_TRUE(is_same<tensor_zero>(d))
      << "Expected tensor_zero, got: " << to_string(d);
}

// d(I)/d(X) = zero (kronecker delta is constant)
TEST_F(TensorDifferentiationTest, KroneckerDeltaConstant) {
  auto d = diff(I, X);
  EXPECT_TRUE(is_same<tensor_zero>(d))
      << "Expected tensor_zero, got: " << to_string(d);
}

// d(pow(X, 2))/d(X) = otimesu(I, X):dX + otimesu(X, I):dX
// which simplifies to a tensor_add of two inner_product terms
TEST_F(TensorDifferentiationTest, PowRule) {
  auto p = pow(X, 2);
  auto d = diff(p, X);
  EXPECT_TRUE(d.is_valid()) << "Expected valid result for pow diff";
  EXPECT_TRUE(is_same<tensor_add>(d))
      << "Expected tensor_add, got: " << to_string(d);
}

// d(F*trans(F))/dF — product rule; zero terms must simplify away
TEST_F(TensorDifferentiationTest, ProductRuleNoZeroArtifacts) {
  auto C = X * trans(X);
  auto d = diff(C, X);
  auto s = to_string(d);
  // Must not contain "0*" or "*0" or "permute_indices(0" patterns
  EXPECT_EQ(s.find("0*"), std::string::npos)
      << "Found '0*' artifact in: " << s;
  EXPECT_EQ(s.find("*0"), std::string::npos)
      << "Found '*0' artifact in: " << s;
  EXPECT_EQ(s.find("permute_indices(0"), std::string::npos)
      << "Found 'permute_indices(0' artifact in: " << s;
}

} // namespace numsim::cas

#endif // TENSORDIFFERENTIATIONTEST_H
