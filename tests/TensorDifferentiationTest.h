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
    I = make_expression<identity_tensor>(dim, std::size_t{2});
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

// d(trans(X))/dX — permute_indices differentiation
TEST_F(TensorDifferentiationTest, PermuteIndicesDiff) {
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
// Tests permute_indices_wrapper permutation composition in differentiation
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

  tmech::tensor<double, 3, 2> X_t =
      tmech::eval(tmech::sym(tmech::randn<double, 3, 2>()));
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
        X_ptr->data() = tmech::eval(tmech::sym(x));
        return as_tmech_diff<3, 2>(*ev.apply(expr));
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(tmech::almost_equal(as_tmech_diff<3, 4>(*result), numdiff, 1e-6))
      << "dev(X) derivative mismatch";
}

// d(inv(dev(X)))/dX — inv of projected tensor.
TEST_F(TensorDifferentiationTest, InvDevProjectorDiff) {
  auto expr = inv(dev(X));
  auto d = diff(expr, X);
  ASSERT_TRUE(d.is_valid()) << "Expected valid derivative for inv(dev(X))";

  std::mt19937 rng1(123);
  std::normal_distribution<double> dist1(0.0, 1.0);
  tmech::tensor<double, 3, 2> X_t;
  for (std::size_t i = 0; i < 9; ++i)
    X_t.raw_data()[i] = dist1(rng1);
  X_t = tmech::eval(tmech::sym(X_t));
  for (std::size_t i = 0; i < 3; ++i)
    X_t(i, i) += 10.0;
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);

  auto result = ev.apply(d);
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->rank(), 4u);

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = tmech::eval(tmech::sym(x));
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

  std::mt19937 rng2(456);
  std::normal_distribution<double> dist2(0.0, 1.0);
  tmech::tensor<double, 3, 2> X_t;
  for (std::size_t i = 0; i < 9; ++i)
    X_t.raw_data()[i] = dist2(rng2);
  X_t = tmech::eval(tmech::sym(X_t));
  for (std::size_t i = 0; i < 3; ++i)
    X_t(i, i) += 10.0;
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);

  auto result = ev.apply(d);
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->rank(), 4u);

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = tmech::eval(tmech::sym(x));
        return as_tmech_diff<3, 2>(*ev.apply(expr));
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(tmech::almost_equal(as_tmech_diff<3, 4>(*result), numdiff, 1e-6))
      << "trans(inv(dev(X))) derivative mismatch";
}

// d(F*trans(F))/dF — product rule; zero terms must simplify away
TEST_F(TensorDifferentiationTest, ProductRuleNoZeroArtifacts) {
  auto C = X * trans(X);
  auto d = diff(C, X);
  auto s = to_string(d);
  // Must not contain "0*" or "*0" or "permute_indices(0" patterns
  EXPECT_EQ(s.find("0*"), std::string::npos) << "Found '0*' artifact in: " << s;
  EXPECT_EQ(s.find("*0"), std::string::npos) << "Found '*0' artifact in: " << s;
  EXPECT_EQ(s.find("permute_indices(0"), std::string::npos)
      << "Found 'permute_indices(0' artifact in: " << s;
}

// ---------------------------------------------------------------------------
// Audit #42: lock-in coverage tests for tensor_differentiation.
// All 16 node types in NUMSIM_CAS_TENSOR_NODE_LIST have explicit operator()
// overrides via the virtual visitor base (missing override = compile error,
// not silent fallback). Existing TEST_F entries above cover 13/16 nodes;
// these add lock-ins for the remaining 3: identity_tensor, tensor_projector,
// and tensor_to_scalar_with_tensor_mul.
// ---------------------------------------------------------------------------

TEST_F(TensorDifferentiationTest, AuditIdentityTensorIsConstant) {
  // identity_tensor is constant w.r.t. any tensor variable.
  auto I_rank2 = make_expression<identity_tensor>(dim, rank);
  auto d = diff(I_rank2, X);
  EXPECT_TRUE(is_same<tensor_zero>(d))
      << "Expected tensor_zero, got: " << to_string(d);
}

TEST_F(TensorDifferentiationTest, AuditTensorProjectorIsConstant) {
  // Projection-tensor leaves (P_sym, P_dev, etc.) are constant w.r.t. tensor
  // arguments. The projector itself has no dependence on X.
  auto P = P_sym(dim);
  auto d = diff(P, X);
  EXPECT_TRUE(is_same<tensor_zero>(d))
      << "Expected tensor_zero, got: " << to_string(d);
}

TEST_F(TensorDifferentiationTest, AuditTensorToScalarWithTensorMulCovered) {
  // tensor_to_scalar_with_tensor_mul is constructed by tensor_to_scalar
  // differentiation (see src/.../tensor_to_scalar_differentiation.cpp).
  // It appears in tensor-domain expressions as a result of differentiating
  // a t2s expression w.r.t. a tensor argument — e.g. d(det(X))/dX yields
  // a chain that contains tensor_to_scalar_with_tensor_mul nodes.
  //
  // This test exercises the differentiator dispatch: when a node of this
  // type is reached during a second differentiation, the visitor must have
  // an explicit override (it does — see tensor_differentiation.h:152) and
  // produce a valid result. Construct via det(X) → diff w.r.t. X.
  auto detX = det(X);
  auto first_diff = diff(detX, X);
  // first_diff is itself a tensor expression. Differentiate again to ensure
  // the tensor_to_scalar_with_tensor_mul path inside tensor_differentiation
  // is exercised without throwing.
  auto second_diff = diff(first_diff, X);
  EXPECT_TRUE(second_diff.is_valid())
      << "Second-order differentiation produced invalid result";
}

// #248: rank-4 inv() construction is now supported, but differentiation
// of a rank-4 inv is NOT yet wired. The visitor throws
// not_implemented_error with a clear message. Lock that contract in so
// users hit a definite error rather than getting silent garbage if they
// try to compose `diff(inv(rank4), ...)`.
TEST(TensorDiffRank4InvNotImplemented, ThrowsWithClearMessage) {
  auto C = make_expression<tensor>("C", 3, 4);
  assume_minor_major(C);
  auto X = make_expression<tensor>("X", 3, 2);
  auto invC = inv(C);
  try {
    [[maybe_unused]] auto r = diff(invC, X);
    FAIL() << "Expected diff(inv(rank-4), ...) to throw not_implemented_error";
  } catch (not_implemented_error const &e) {
    EXPECT_NE(std::string(e.what()).find("rank"), std::string::npos)
        << "error message should mention rank; got: " << e.what();
  }
}

// ─── diff(tensor, scalar) — issue #275 ────────────────────────────
//
// Companion to the tensor-arg differentiation above. Differentiating
// a tensor expression w.r.t. a scalar variable returns a same-rank
// tensor; the central piece is reinstating the product-rule term in
// `tensor_scalar_mul` that the tensor-arg visitor drops as "scalar is
// constant w.r.t. tensor arg".

class TensorDiffWrtScalarTest : public ::testing::Test {
protected:
  static constexpr std::size_t dim = 3;

  using tensor_t = expression_holder<tensor_expression>;
  using scalar_t = expression_holder<scalar_expression>;

  TensorDiffWrtScalarTest() {
    std::tie(A, B, n) =
        make_tensor_variable(std::tuple{"A", dim, std::size_t{2}},
                             std::tuple{"B", dim, std::size_t{2}},
                             std::tuple{"n", dim, std::size_t{2}});
    std::tie(s, t) = make_scalar_variable("s", "t");
    std::tie(mu, dgamma) = make_scalar_variable("mu", "dgamma");
    I = make_expression<identity_tensor>(dim, std::size_t{2});
    Zero = make_expression<tensor_zero>(dim, std::size_t{2});
  }

  tensor_t A, B, n;
  scalar_t s, t;
  scalar_t mu, dgamma;
  tensor_t I;
  tensor_t Zero;
};

// Acceptance #1: diff(T, s) compiles and returns a same-rank tensor.
TEST_F(TensorDiffWrtScalarTest, CompilesAndReturnsSameRankTensor) {
  auto d = diff(A, s);
  EXPECT_TRUE(d.is_valid());
  EXPECT_EQ(d.get().rank(), A.get().rank());
  EXPECT_EQ(d.get().dim(), A.get().dim());
}

// Acceptance #4: tensor variable independent of s → zero.
TEST_F(TensorDiffWrtScalarTest, IndependentTensorVariableYieldsZero) {
  auto d = diff(A, s);
  EXPECT_TRUE(is_same<tensor_zero>(d))
      << "Expected tensor_zero, got: " << to_string(d);
}

// Acceptance #2: diff(s * I, s) == I (product rule reinstated).
// The central piece of #275 — the tensor-arg visitor drops the
// scalar factor's derivative; we now reinstate it.
TEST_F(TensorDiffWrtScalarTest, ScalarTimesIdentityDerivativeIsIdentity) {
  auto d = diff(s * I, s);
  ASSERT_TRUE(d.is_valid());
  EXPECT_EQ(d.get().hash_value(), I.get().hash_value())
      << "Expected I, got: " << to_string(d);
}

// Acceptance #3: diff(f(s) * A, s) == f'(s) * A for a tensor variable
// A independent of s. Uses f(s) = s^2 so f'(s) = 2*s.
TEST_F(TensorDiffWrtScalarTest, ScalarFunctionTimesConstantTensor) {
  auto f = pow(s, 2); // f(s) = s^2 → f'(s) = 2*s
  auto d = diff(f * A, s);
  ASSERT_TRUE(d.is_valid());
  // Build the expected expression: (2*s) * A
  auto expected =
      (scalar_t{make_expression<scalar_constant>(scalar_number{2})} * s) * A;
  EXPECT_EQ(d.get().hash_value(), expected.get().hash_value())
      << "Expected (2*s)*A, got: " << to_string(d);
}

// Acceptance #5: full product rule on (u·A)·(v·B) with u=u(s), v=v(s).
// d/ds = (du·A) · (v·B) + (u·A) · (dv·B)
//
// Compare printed forms rather than hash values. The visitor's
// step-by-step build may yield a different STRUCTURAL representation
// than the hand-expanded expression (different canonicalization
// paths through tensor_mul flattening) even when the math is
// equivalent. The printer normalises both to the same canonical
// string, which is the contract we actually care about for #275.
TEST_F(TensorDiffWrtScalarTest, FullProductRuleTwoScalarFactors) {
  auto u = pow(s, 2); // u(s) = s^2 → u'(s) = 2*s
  auto v = pow(s, 3); // v(s) = s^3 → v'(s) = 3*s^2
  auto uA = u * A;
  auto vB = v * B;
  auto expr = uA * vB;
  auto d = diff(expr, s);
  ASSERT_TRUE(d.is_valid());

  // Validate against the hand-expanded result:
  //   d/ds[(u·A)·(v·B)] = (u'·A)·(v·B) + (u·A)·(v'·B)
  auto du = diff(u, s); // 2*s
  auto dv = diff(v, s); // 3*s^2
  auto expected = (du * A) * (v * B) + (u * A) * (dv * B);
  EXPECT_EQ(to_string(d), to_string(expected))
      << "Got:      " << to_string(d) << "\nExpected: " << to_string(expected);
}

// Acceptance #6 — the radial-return case from the issue body:
// d/d(dgamma) [sigma_trial - 2*mu*dgamma*n] == -2*mu*n
// with mu, n independent of dgamma.
TEST_F(TensorDiffWrtScalarTest, RadialReturnShape) {
  auto sigma_trial = A; // doesn't depend on dgamma
  auto two = scalar_t{make_expression<scalar_constant>(scalar_number{2})};
  auto expr = sigma_trial - two * mu * dgamma * n;
  auto d = diff(expr, dgamma);
  ASSERT_TRUE(d.is_valid());
  // Expected: -2*mu*n  (sigma_trial and n are dgamma-independent;
  // mu is dgamma-independent; only the dgamma factor contributes).
  auto expected = -(two * mu * n);
  EXPECT_EQ(d.get().hash_value(), expected.get().hash_value())
      << "Got:      " << to_string(d) << "\nExpected: " << to_string(expected);
}

// Sanity: zero-derivative leaf nodes (identity_tensor, levi_civita)
// w.r.t. a scalar are zero.
TEST_F(TensorDiffWrtScalarTest, IdentityIsConstant) {
  auto d = diff(I, s);
  EXPECT_TRUE(is_same<tensor_zero>(d))
      << "Expected tensor_zero, got: " << to_string(d);
}

TEST_F(TensorDiffWrtScalarTest, ZeroDerivative) {
  auto d = diff(Zero, s);
  EXPECT_TRUE(is_same<tensor_zero>(d))
      << "Expected tensor_zero, got: " << to_string(d);
}

// Linearity: diff(A + s*B, s) == B
TEST_F(TensorDiffWrtScalarTest, AdditiveLinearity) {
  auto expr = A + s * B;
  auto d = diff(expr, s);
  ASSERT_TRUE(d.is_valid());
  EXPECT_EQ(d.get().hash_value(), B.get().hash_value())
      << "Expected B, got: " << to_string(d);
}

// Pass-1 review L1 + pass-3 fix: cover the tensor_inv rule directly.
// The public `inv(s*A)` factory folds to `inv(A)/s` (the
// `inv(α·A) → inv(A)/α` rule from #71), which is a tensor_scalar_mul
// — NOT a tensor_inv node — so the visitor's tensor_inv rule never
// fires. Bypass the fold via make_expression to actually exercise the
// rule under test. Same pattern as PowOneSingletonHandledByVisitor.
TEST_F(TensorDiffWrtScalarTest, InvRuleExercisedDirectly) {
  auto expr = make_expression<tensor_inv>(A);
  ASSERT_TRUE(is_same<tensor_inv>(expr))
      << "Setup precondition: expression must be a tensor_inv node to "
         "exercise the rule under test; got: "
      << to_string(expr);
  // A is s-independent → recursive diff(A, s) returns a valid
  // tensor_zero singleton (per apply()'s shape-aware fallback). The
  // pass-5 singleton-aware guard in the tensor_inv rule then fires
  // and early-returns, leaving m_result invalid; apply() coerces to
  // tensor_zero. Without the singleton guard, the rule would build
  // `-(invA · 0 · invA)` and depend on the inner_product simplifier
  // to fold the zeros — fragile coupling.
  auto d = diff(expr, s);
  ASSERT_TRUE(d.is_valid());
  EXPECT_TRUE(is_same<tensor_zero>(d))
      << "Expected tensor_zero for derivative of inv(A) w.r.t. s "
         "where A is s-independent; got: "
      << to_string(d);
}

// Pass-1 review L1: cover the tensor_pow rule with non-trivial
// scalar coefficient. d/ds(pow(s*A, 2)) = 2*s*A*A (rank-2 matrix
// product semantics).
TEST_F(TensorDiffWrtScalarTest, PowRule) {
  auto expr = pow(s * A, 2);
  auto d = diff(expr, s);
  ASSERT_TRUE(d.is_valid());
  EXPECT_EQ(d.get().rank(), 2u);
}

// Pass-1 review / pass-2 fix: lock that the tensor_pow rule handles
// the scalar_one singleton (#284 trap). The public `pow(x, 1)` factory
// folds to `x` at construction (tensor_std.h), so we bypass the fold
// by constructing a tensor_pow node directly via make_expression.
// A bare is_same<scalar_constant>(scalar_one) check would return
// false and throw not_implemented; try_int_constant correctly returns
// 1 and the rule produces a valid result.
TEST_F(TensorDiffWrtScalarTest, PowOneSingletonHandledByVisitor) {
  auto one_singleton = get_scalar_one();
  // make_expression bypasses the pow() factory fold that would
  // collapse pow(s*A, 1) → s*A; we need a real tensor_pow node with
  // scalar_one as the exponent so the visitor's tensor_pow rule
  // runs.
  auto expr = make_expression<tensor_pow>(s * A, one_singleton);
  ASSERT_TRUE(is_same<tensor_pow>(expr))
      << "Setup precondition: expression must be a tensor_pow node "
         "to exercise the rule under test; got: "
      << to_string(expr);
  EXPECT_NO_THROW({ [[maybe_unused]] auto d = diff(expr, s); });
}

// Negation: diff(-(s*A), s) == -A
TEST_F(TensorDiffWrtScalarTest, NegationCommutesWithDiff) {
  auto expr = -(s * A);
  auto d = diff(expr, s);
  ASSERT_TRUE(d.is_valid());
  auto expected = -A;
  EXPECT_EQ(d.get().hash_value(), expected.get().hash_value())
      << "Got:      " << to_string(d) << "\nExpected: " << to_string(expected);
}

// Pass-1 review (architect M6) + pass-2 M3 (numerical lock-in):
// tensor_to_scalar_with_tensor_mul rule exercises BOTH
// diff(tensor, scalar) AND diff(t2s, scalar). Without #285 the t2s
// side would throw not_implemented. Verified numerically:
//   d/ds(A * trace(s*B)) = (dA/ds)*trace(s*B) + A*d/ds(trace(s*B))
//                       = 0*trace(s*B) + A*trace(B)  (A is s-indep)
//                       = A * trace(B)
// At any s value the derivative's components are A_t * trace(B_t).
TEST_F(TensorDiffWrtScalarTest, CrossCPOTensorTimesT2s) {
  auto expr =
      make_expression<tensor_to_scalar_with_tensor_mul>(A, trace(s * B));
  auto d = diff(expr, s);
  ASSERT_TRUE(d.is_valid());
  EXPECT_EQ(d.get().rank(), A.get().rank());
  EXPECT_FALSE(is_same<tensor_zero>(d))
      << "Expected non-zero derivative; got: " << to_string(d);

  // Numerical lock-in.
  tmech::tensor<double, 3, 2> A_t = tmech::randn<double, 3, 2>();
  tmech::tensor<double, 3, 2> B_t = tmech::randn<double, 3, 2>();
  auto A_ptr = std::make_shared<tensor_data<double, 3, 2>>(A_t);
  auto B_ptr = std::make_shared<tensor_data<double, 3, 2>>(B_t);
  tensor_evaluator<double> ev;
  ev.set(A, A_ptr);
  ev.set(B, B_ptr);
  ev.set_scalar(s, 2.0);
  auto d_val = ev.apply(d);
  ASSERT_NE(d_val, nullptr);
  double tr_B = tmech::trace(B_t);
  auto expected_t = A_t * tr_B;
  EXPECT_TRUE(
      tmech::almost_equal(as_tmech_diff<3, 2>(*d_val), expected_t, 1e-12))
      << "diff(A * trace(s*B), s) should evaluate to A * trace(B)";
}

// ─── Direct rule lock-ins for nodes the acceptance tests don't reach ──
// Without these, each rule is only exercised transitively through
// composition; a future refactor breaking a specific rule wouldn't be
// caught. Coverage debt found in the pass-5+ audit.

// levi_civita_tensor leaf: constant w.r.t. any scalar → derivative is
// the canonical tensor_zero of matching rank/dim.
TEST_F(TensorDiffWrtScalarTest, LeviCivitaTensorRuleIsConstant) {
  auto eps3 = numsim::cas::levi_civita(std::size_t{3});
  auto d = diff(eps3, s);
  EXPECT_TRUE(is_same<tensor_zero>(d))
      << "Expected tensor_zero; got: " << to_string(d);
}

// tensor_projector leaf: same — constant w.r.t. any scalar.
TEST_F(TensorDiffWrtScalarTest, TensorProjectorRuleIsConstant) {
  auto P = P_sym(std::size_t{3});
  auto d = diff(P, s);
  EXPECT_TRUE(is_same<tensor_zero>(d))
      << "Expected tensor_zero; got: " << to_string(d);
}

// tensor_if_then_else rule: derivative passes through both branches.
// d/ds if_then_else(cond, s*A, B) where cond is sv-independent →
// if_then_else(cond, A, 0).
TEST_F(TensorDiffWrtScalarTest, TensorIfThenElseRuleAppliesToBothBranches) {
  auto cond = t; // scalar variable, sv-independent
  auto then_branch = s * A;
  auto else_branch = B;
  auto expr =
      make_expression<tensor_if_then_else>(cond, then_branch, else_branch);
  ASSERT_TRUE(is_same<tensor_if_then_else>(expr));
  auto d = diff(expr, s);
  ASSERT_TRUE(d.is_valid());
  EXPECT_TRUE(is_same<tensor_if_then_else>(d))
      << "Expected tensor_if_then_else; got: " << to_string(d);
}

// permute_indices_wrapper rule: chain rule applies to the child.
// d/ds permute_indices(s*A, [2,1]) = permute_indices(A, [2,1])
TEST_F(TensorDiffWrtScalarTest, PermuteIndicesRuleAppliesToChild) {
  auto sA = s * A;
  auto expr = permute_indices(sA, sequence{2, 1});
  auto d = diff(expr, s);
  ASSERT_TRUE(d.is_valid());
  auto expected = permute_indices(A, sequence{2, 1});
  EXPECT_EQ(d.get().hash_value(), expected.get().hash_value())
      << "Got:      " << to_string(d) << "\nExpected: " << to_string(expected);
}

// simple_outer_product rule: product rule across factors.
TEST_F(TensorDiffWrtScalarTest, SimpleOuterProductRuleAppliesProductRule) {
  auto sop = make_expression<simple_outer_product>(dim, std::size_t{4});
  sop.template get<simple_outer_product>().push_back(s * A);
  sop.template get<simple_outer_product>().push_back(B);
  auto d = diff(sop, s);
  ASSERT_TRUE(d.is_valid());
  EXPECT_FALSE(is_same<tensor_zero>(d))
      << "Expected non-zero derivative; got: " << to_string(d);
}

// inner_product_wrapper rule: product rule, tensor-typed result.
TEST_F(TensorDiffWrtScalarTest, InnerProductWrapperRuleAppliesProductRule) {
  // Contract on a single index pair → rank-2 tensor result.
  auto expr = inner_product(s * A, sequence{2}, B, sequence{1});
  auto d = diff(expr, s);
  ASSERT_TRUE(d.is_valid());
  EXPECT_EQ(d.get().rank(), 2u);
  EXPECT_FALSE(is_same<tensor_zero>(d))
      << "Expected non-zero derivative; got: " << to_string(d);
}

// outer_product_wrapper rule: product rule.
TEST_F(TensorDiffWrtScalarTest, OuterProductWrapperRuleAppliesProductRule) {
  auto expr = otimes(s * A, B);
  auto d = diff(expr, s);
  ASSERT_TRUE(d.is_valid());
  EXPECT_EQ(d.get().rank(), 4u);
  EXPECT_FALSE(is_same<tensor_zero>(d))
      << "Expected non-zero derivative; got: " << to_string(d);
}

} // namespace numsim::cas

#endif // TENSORDIFFERENTIATIONTEST_H
