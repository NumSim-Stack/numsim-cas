#ifndef TENSORDIFFERENTIATIONTEST_H
#define TENSORDIFFERENTIATIONTEST_H

#include "cas_test_helpers.h"
#include <gtest/gtest.h>
#include <memory>
#include <tmech/tmech.h>

#include <numsim_cas/core/diff.h>
#include <numsim_cas/numsim_cas.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor/visitors/tensor_evaluator.h>

namespace numsim::cas {

namespace {

template <std::size_t Dim, std::size_t Rank>
auto const &as_tmech_diff(tensor_data_base<double> const &data) {
  return static_cast<tensor_data<double, Dim, Rank> const &>(data).data();
}

} // namespace

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

// --- Numerical differentiation tests for tensor_mul and simple_outer_product
// ---

// Helper: evaluate a tensor expression with given variable values
// and compare symbolic derivative against finite differences.

// d(X*Y)/dX — two-factor tensor_mul, numerically verified
TEST_F(TensorDifferentiationTest, TensorMulTwoFactors) {
  auto expr = X * Y;
  auto d = diff(expr, X);
  ASSERT_TRUE(d.is_valid()) << "Expected valid derivative for X*Y";

  tmech::tensor<double, 3, 2> X_t = tmech::randn<double, 3, 2>();
  tmech::tensor<double, 3, 2> Y_t = tmech::randn<double, 3, 2>();
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);
  auto Y_ptr = std::make_shared<tensor_data<double, 3, 2>>(Y_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);
  ev.set(Y, Y_ptr);

  auto result = ev.apply(d);
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->rank(), 4u);

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = x;
        return as_tmech_diff<3, 2>(*ev.apply(expr));
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(tmech::almost_equal(as_tmech_diff<3, 4>(*result), numdiff, 1e-6));
}

// d(X*Y*X)/dX — three-factor tensor_mul with X appearing twice
TEST_F(TensorDifferentiationTest, TensorMulThreeFactors) {
  auto expr = X * Y * X;
  auto d = diff(expr, X);
  ASSERT_TRUE(d.is_valid()) << "Expected valid derivative for X*Y*X";

  tmech::tensor<double, 3, 2> X_t = tmech::randn<double, 3, 2>();
  tmech::tensor<double, 3, 2> Y_t = tmech::randn<double, 3, 2>();
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);
  auto Y_ptr = std::make_shared<tensor_data<double, 3, 2>>(Y_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);
  ev.set(Y, Y_ptr);

  auto result = ev.apply(d);
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->rank(), 4u);

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = x;
        return as_tmech_diff<3, 2>(*ev.apply(expr));
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(tmech::almost_equal(as_tmech_diff<3, 4>(*result), numdiff, 1e-6));
}

// d(u⊗v)/du — two-factor simple_outer_product with rank-1 vectors
TEST_F(TensorDifferentiationTest, SimpleOuterProductTwoFactors) {
  auto u = make_expression<tensor>("u", dim, 1);
  auto v = make_expression<tensor>("v", dim, 1);

  // Build simple_outer_product(u, v)
  auto sop = make_expression<simple_outer_product>(dim, 2);
  sop.template get<simple_outer_product>().push_back(u);
  sop.template get<simple_outer_product>().push_back(v);

  auto d = diff(sop, u);
  ASSERT_TRUE(d.is_valid()) << "Expected valid derivative for u⊗v w.r.t. u";

  tmech::tensor<double, 3, 1> u_t = tmech::randn<double, 3, 1>();
  tmech::tensor<double, 3, 1> v_t = tmech::randn<double, 3, 1>();
  auto u_ptr = std::make_shared<tensor_data<double, 3, 1>>(u_t);
  auto v_ptr = std::make_shared<tensor_data<double, 3, 1>>(v_t);

  tensor_evaluator<double> ev;
  ev.set(u, u_ptr);
  ev.set(v, v_ptr);

  auto result = ev.apply(d);
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->rank(), 3u); // rank-2 result + rank-1 derivative

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3>>(
      [&](auto const &x) {
        u_ptr->data() = x;
        return as_tmech_diff<3, 2>(*ev.apply(sop));
      },
      u_t);
  u_ptr->data() = u_t;

  EXPECT_TRUE(tmech::almost_equal(as_tmech_diff<3, 3>(*result), numdiff, 1e-6));
}

// d(u⊗v⊗u)/du — three-factor simple_outer_product with u appearing twice
TEST_F(TensorDifferentiationTest, SimpleOuterProductThreeFactors) {
  auto u = make_expression<tensor>("u", dim, 1);
  auto v = make_expression<tensor>("v", dim, 1);

  // Build simple_outer_product(u, v, u)
  auto sop = make_expression<simple_outer_product>(dim, 3);
  sop.template get<simple_outer_product>().push_back(u);
  sop.template get<simple_outer_product>().push_back(v);
  sop.template get<simple_outer_product>().push_back(u);

  auto d = diff(sop, u);
  ASSERT_TRUE(d.is_valid()) << "Expected valid derivative for u⊗v⊗u w.r.t. u";

  tmech::tensor<double, 3, 1> u_t = tmech::randn<double, 3, 1>();
  tmech::tensor<double, 3, 1> v_t = tmech::randn<double, 3, 1>();
  auto u_ptr = std::make_shared<tensor_data<double, 3, 1>>(u_t);
  auto v_ptr = std::make_shared<tensor_data<double, 3, 1>>(v_t);

  tensor_evaluator<double> ev;
  ev.set(u, u_ptr);
  ev.set(v, v_ptr);

  auto result = ev.apply(d);
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->rank(), 4u); // rank-3 result + rank-1 derivative

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        u_ptr->data() = x;
        return as_tmech_diff<3, 3>(*ev.apply(sop));
      },
      u_t);
  u_ptr->data() = u_t;

  EXPECT_TRUE(tmech::almost_equal(as_tmech_diff<3, 4>(*result), numdiff, 1e-6));
}

// d(inner_product(X, seq{1}, Y, seq{1}))/dX — contracts first index of each
TEST_F(TensorDifferentiationTest, InnerProductDiff) {
  auto expr = inner_product(X, sequence{1}, Y, sequence{1});
  auto d = diff(expr, X);
  ASSERT_TRUE(d.is_valid()) << "Expected valid derivative for inner_product";

  tmech::tensor<double, 3, 2> X_t = tmech::randn<double, 3, 2>();
  tmech::tensor<double, 3, 2> Y_t = tmech::randn<double, 3, 2>();
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);
  auto Y_ptr = std::make_shared<tensor_data<double, 3, 2>>(Y_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);
  ev.set(Y, Y_ptr);

  auto result = ev.apply(d);
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->rank(), 4u);

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = x;
        return as_tmech_diff<3, 2>(*ev.apply(expr));
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(tmech::almost_equal(as_tmech_diff<3, 4>(*result), numdiff, 1e-6));
}

// d(otimes(X, Y))/dX — outer product differentiation
TEST_F(TensorDifferentiationTest, OuterProductDiff) {
  auto expr = otimes(X, Y);
  auto d = diff(expr, X);
  ASSERT_TRUE(d.is_valid()) << "Expected valid derivative for otimes(X, Y)";

  tmech::tensor<double, 3, 2> X_t = tmech::randn<double, 3, 2>();
  tmech::tensor<double, 3, 2> Y_t = tmech::randn<double, 3, 2>();
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);
  auto Y_ptr = std::make_shared<tensor_data<double, 3, 2>>(Y_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);
  ev.set(Y, Y_ptr);

  auto result = ev.apply(d);
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->rank(), 6u);

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4, 5, 6>>(
      [&](auto const &x) {
        X_ptr->data() = x;
        return as_tmech_diff<3, 4>(*ev.apply(expr));
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(tmech::almost_equal(as_tmech_diff<3, 6>(*result), numdiff, 1e-6));
}

// d(trans(X))/dX — basis change differentiation
TEST_F(TensorDifferentiationTest, BasisChangeDiff) {
  auto expr = trans(X);
  auto d = diff(expr, X);
  ASSERT_TRUE(d.is_valid()) << "Expected valid derivative for trans(X)";

  tmech::tensor<double, 3, 2> X_t = tmech::randn<double, 3, 2>();
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);

  auto result = ev.apply(d);
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->rank(), 4u);

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = x;
        return as_tmech_diff<3, 2>(*ev.apply(expr));
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(tmech::almost_equal(as_tmech_diff<3, 4>(*result), numdiff, 1e-6));
}

// d(X*X)/dX — same variable in both tensor_mul factors (product rule)
// Build tensor_mul directly to bypass X*X → pow(X,2) simplification
TEST_F(TensorDifferentiationTest, TensorMulSameVariable) {
  auto expr = make_expression<tensor_mul>(dim, rank);
  expr.template get<tensor_mul>().push_back(X);
  expr.template get<tensor_mul>().push_back(X);

  auto d = diff(expr, X);
  ASSERT_TRUE(d.is_valid()) << "Expected valid derivative for X*X";

  tmech::tensor<double, 3, 2> X_t = tmech::randn<double, 3, 2>();
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);

  auto result = ev.apply(d);
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->rank(), 4u);

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = x;
        return as_tmech_diff<3, 2>(*ev.apply(expr));
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(tmech::almost_equal(as_tmech_diff<3, 4>(*result), numdiff, 1e-6));
}

// Regression: d(permute_indices(inner_product(C, {1}, E, {3}), {3,1,2}))/dC
// Tests basis_change_imp permutation composition in differentiation
TEST_F(TensorDifferentiationTest, PermuteInnerProductDiff) {
  auto C = make_expression<tensor>("C", 3, 2);
  auto E = make_expression<tensor>("E", 3, 3);

  auto ip = inner_product(C, sequence{1}, E, sequence{3});
  auto expr = permute_indices(std::move(ip), sequence{3, 1, 2});

  auto d = diff(expr, C);
  ASSERT_TRUE(d.is_valid());

  tmech::tensor<double, 3, 2> C_t = tmech::randn<double, 3, 2>();
  tmech::tensor<double, 3, 3> E_t = tmech::randn<double, 3, 3>();
  auto C_ptr = std::make_shared<tensor_data<double, 3, 2>>(C_t);
  auto E_ptr = std::make_shared<tensor_data<double, 3, 3>>(E_t);

  tensor_evaluator<double> ev;
  ev.set(C, C_ptr);
  ev.set(E, E_ptr);

  auto result = ev.apply(d);
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->rank(), 5u);

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4, 5>>(
      [&](auto const &x) {
        C_ptr->data() = x;
        return as_tmech_diff<3, 3>(*ev.apply(expr));
      },
      C_t);
  C_ptr->data() = C_t;

  EXPECT_TRUE(tmech::almost_equal(as_tmech_diff<3, 5>(*result), numdiff, 1e-6));
}

// d(dev(X))/dX — projector differentiation through generic inner_product path
TEST_F(TensorDifferentiationTest, DevProjectorDiff) {
  auto expr = dev(X);
  auto d = diff(expr, X);
  ASSERT_TRUE(d.is_valid()) << "Expected valid derivative for dev(X)";

  tmech::tensor<double, 3, 2> X_t = tmech::randn<double, 3, 2>();
  for (std::size_t i = 0; i < 3; ++i)
    X_t(i, i) += 5.0;
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);

  auto result = ev.apply(d);
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->rank(), 4u);

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = x;
        return as_tmech_diff<3, 2>(*ev.apply(expr));
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(tmech::almost_equal(as_tmech_diff<3, 4>(*result), numdiff, 1e-6))
      << "dev(X) derivative mismatch";
}

// d(inv(dev(X)))/dX — inv of projected tensor
TEST_F(TensorDifferentiationTest, InvDevProjectorDiff) {
  auto expr = inv(dev(X));
  auto d = diff(expr, X);
  ASSERT_TRUE(d.is_valid()) << "Expected valid derivative for inv(dev(X))";

  // Use symmetric, diagonally-dominant matrix for invertible dev
  tmech::tensor<double, 3, 2> X_t = tmech::randn<double, 3, 2>();
  for (std::size_t i = 0; i < 3; ++i)
    X_t(i, i) += 10.0;
  X_t = tmech::sym(X_t);
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);

  auto result = ev.apply(d);
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->rank(), 4u);

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = x;
        return as_tmech_diff<3, 2>(*ev.apply(expr));
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(tmech::almost_equal(as_tmech_diff<3, 4>(*result), numdiff, 1e-6))
      << "inv(dev(X)) derivative mismatch";
}

// d(trans(inv(dev(X))))/dX — combination of trans, inv, dev
TEST_F(TensorDifferentiationTest, TransInvDevProjectorDiff) {
  auto expr = trans(inv(dev(X)));
  auto d = diff(expr, X);
  ASSERT_TRUE(d.is_valid())
      << "Expected valid derivative for trans(inv(dev(X)))";

  tmech::tensor<double, 3, 2> X_t = tmech::randn<double, 3, 2>();
  for (std::size_t i = 0; i < 3; ++i)
    X_t(i, i) += 10.0;
  X_t = tmech::sym(X_t);
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);

  auto result = ev.apply(d);
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->rank(), 4u);

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = x;
        return as_tmech_diff<3, 2>(*ev.apply(expr));
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(tmech::almost_equal(as_tmech_diff<3, 4>(*result), numdiff, 1e-6))
      << "trans(inv(dev(X))) derivative mismatch";
}

} // namespace numsim::cas

#endif // TENSORDIFFERENTIATIONTEST_H
