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

    I = make_expression<identity_tensor>(dim, std::size_t{2});
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

// Regression: dot(B)-(det(B)*det(A)*tr(A)) w.r.t. symmetric B
// derivative must have rank 2, not rank 4
TEST_F(TensorToScalarDifferentiationTest, SymmetricDotDetDiff) {
  // Create symmetric variables
  tensor_space sym_space{Symmetric{}, AnyTraceTag{}};
  auto A = make_expression<tensor>("A", dim, rank);
  auto B = make_expression<tensor>("B", dim, rank);
  A.data()->set_space(sym_space);
  B.data()->set_space(sym_space);

  auto expr = dot(B) - (det(B) * det(A) * trace(A));
  auto d = diff(expr, B);
  ASSERT_TRUE(d.is_valid()) << "Expected valid derivative";
  EXPECT_EQ(d.get().rank(), 2u)
      << "derivative of t2s expr w.r.t. rank-2 var must be rank 2\n"
      << "  d: " << to_string(d);
}

// ---------------------------------------------------------------------------
// Audit #38: lock-in coverage tests for tensor_to_scalar_differentiation.
// All 15 node types in NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST have explicit
// operator() overrides via the pure-virtual visitor base (missing override
// = compile error, not silent fallback). Existing TEST_F entries above
// cover trace, det, dot, norm, log, add, negative, one, zero. These add
// lock-ins for the remaining six nodes.
// ---------------------------------------------------------------------------

TEST_F(TensorToScalarDifferentiationTest, AuditNode_ScalarWrapper) {
  // d(scalar_wrapper(c))/dY = 0 — scalar constant inside t2s is independent.
  auto c = make_expression<tensor_to_scalar_scalar_wrapper>(
      make_expression<scalar_constant>(2.0));
  auto d = diff(c, Y);
  EXPECT_TRUE(d.is_valid()) << "Expected valid derivative for scalar_wrapper";
  EXPECT_TRUE(is_same<tensor_zero>(d))
      << "Expected tensor_zero, got: " << to_string(d);
}

TEST_F(TensorToScalarDifferentiationTest, AuditNode_Mul) {
  // d(trace(Y) * det(Y))/dY — product rule applied to t2s factors.
  auto expr = trace(Y) * det(Y);
  auto d = diff(expr, Y);
  EXPECT_TRUE(d.is_valid()) << "Expected valid derivative for t2s mul";
  EXPECT_EQ(d.get().rank(), rank);
}

TEST_F(TensorToScalarDifferentiationTest, AuditNode_Pow) {
  // d(trace(Y)^3)/dY — exercise the t2s pow differentiation path.
  auto expr = pow(trace(Y), 3);
  auto d = diff(expr, Y);
  EXPECT_TRUE(d.is_valid()) << "Expected valid derivative for t2s pow";
  EXPECT_EQ(d.get().rank(), rank);
}

TEST_F(TensorToScalarDifferentiationTest, AuditNode_Exp) {
  // d(exp(trace(Y)))/dY — chain rule via t2s exp.
  auto expr = exp(trace(Y));
  auto d = diff(expr, Y);
  EXPECT_TRUE(d.is_valid()) << "Expected valid derivative for t2s exp";
  EXPECT_EQ(d.get().rank(), rank);
}

TEST_F(TensorToScalarDifferentiationTest, AuditNode_Sqrt) {
  // d(sqrt(trace(Y)))/dY — chain rule via t2s sqrt.
  auto expr = sqrt(trace(Y));
  auto d = diff(expr, Y);
  EXPECT_TRUE(d.is_valid()) << "Expected valid derivative for t2s sqrt";
  EXPECT_EQ(d.get().rank(), rank);
}

TEST_F(TensorToScalarDifferentiationTest, AuditNode_InnerProductToScalar) {
  // d(Y:Y)/dY via dot_product (constructs tensor_inner_product_to_scalar).
  auto expr = dot_product(Y, sequence{1, 2}, Y, sequence{1, 2});
  auto d = diff(expr, Y);
  EXPECT_TRUE(d.is_valid())
      << "Expected valid derivative for inner_product_to_scalar";
  EXPECT_EQ(d.get().rank(), rank);
}

// ─── diff(tensor_to_scalar, scalar) — issue #285 ─────────────────
//
// Companion to #275's tensor-arg diff w.r.t. scalar. Result type is
// tensor_to_scalar (a scalar-valued AST that may contain tensor
// invariants).

class T2SDiffWrtScalarTest : public ::testing::Test {
protected:
  static constexpr std::size_t dim = 3;

  using tensor_t = expression_holder<tensor_expression>;
  using t2s_t = expression_holder<tensor_to_scalar_expression>;
  using scalar_t = expression_holder<scalar_expression>;

  T2SDiffWrtScalarTest() {
    std::tie(eps, n, s_trial) =
        make_tensor_variable(std::tuple{"eps", dim, std::size_t{2}},
                             std::tuple{"n", dim, std::size_t{2}},
                             std::tuple{"s_trial", dim, std::size_t{2}});
    std::tie(sv, mu) = make_scalar_variable("sv", "mu");
  }

  tensor_t eps, n, s_trial;
  scalar_t sv, mu;
};

// Acceptance #1: diff(norm(eps) - sv, sv) == -1 numerically.
TEST_F(T2SDiffWrtScalarTest, NormMinusScalarWrtScalar) {
  auto R = norm(eps) - sv;
  auto J = diff(R, sv);
  ASSERT_TRUE(J.is_valid());
  using tdata = tensor_data<double, 3, 2>;
  tmech::tensor<double, 3, 2> eps_t = tmech::randn<double, 3, 2>();
  auto eps_ptr = std::make_shared<tdata>(eps_t);
  tensor_to_scalar_evaluator<double> ev;
  ev.set(eps, eps_ptr);
  ev.set_scalar(sv, 1.5);
  EXPECT_NEAR(ev.apply(J), -1.0, 1e-12);
}

// Acceptance #2: diff(f(sv) * trace(eps), sv) == f'(sv) * trace(eps).
TEST_F(T2SDiffWrtScalarTest, ScalarFunctionTimesTrace) {
  auto f = pow(sv, 2); // f(sv) = sv^2 → f'(sv) = 2*sv
  auto R = f * trace(eps);
  auto J = diff(R, sv);
  ASSERT_TRUE(J.is_valid());
  using tdata = tensor_data<double, 3, 2>;
  tmech::tensor<double, 3, 2> eps_t = tmech::randn<double, 3, 2>();
  auto eps_ptr = std::make_shared<tdata>(eps_t);
  tensor_to_scalar_evaluator<double> ev;
  ev.set(eps, eps_ptr);
  ev.set_scalar(sv, 2.0);
  double trace_eps = ev.apply(trace(eps));
  double expected = 2.0 * 2.0 * trace_eps; // 2*sv at sv=2
  EXPECT_NEAR(ev.apply(J), expected, 1e-12);
}

// Acceptance #3: radial-return shape
// d/d(sv) [norm(s_trial - 2*mu*sv*n)] at sv=0 with mu, n independent.
// Hand value: -2*mu*(n : s_trial)/norm(s_trial).
TEST_F(T2SDiffWrtScalarTest, RadialReturnNormDerivative) {
  auto two = make_expression<scalar_constant>(scalar_number{2});
  auto inside = s_trial - two * mu * sv * n;
  auto R = norm(inside);
  auto J = diff(R, sv);
  ASSERT_TRUE(J.is_valid());

  using tdata = tensor_data<double, 3, 2>;
  tmech::tensor<double, 3, 2> s_t = tmech::randn<double, 3, 2>();
  tmech::tensor<double, 3, 2> n_t = tmech::randn<double, 3, 2>();
  double nn = std::sqrt(tmech::dcontract(n_t, n_t));
  n_t = n_t / nn;

  auto s_ptr = std::make_shared<tdata>(s_t);
  auto n_ptr = std::make_shared<tdata>(n_t);
  tensor_to_scalar_evaluator<double> ev;
  ev.set(s_trial, s_ptr);
  ev.set(n, n_ptr);
  ev.set_scalar(mu, 1.5);
  ev.set_scalar(sv, 0.0);

  double ns = tmech::dcontract(n_t, s_t);
  double norm_s = std::sqrt(tmech::dcontract(s_t, s_t));
  double expected = -2.0 * 1.5 * ns / norm_s;
  EXPECT_NEAR(ev.apply(J), expected, 1e-10);
}

// Constant t2s w.r.t. sv → 0.
TEST_F(T2SDiffWrtScalarTest, ConstantT2sYieldsZero) {
  auto J = diff(trace(eps), sv);
  ASSERT_TRUE(J.is_valid());
  using tdata = tensor_data<double, 3, 2>;
  tmech::tensor<double, 3, 2> eps_t = tmech::randn<double, 3, 2>();
  auto eps_ptr = std::make_shared<tdata>(eps_t);
  tensor_to_scalar_evaluator<double> ev;
  ev.set(eps, eps_ptr);
  ev.set_scalar(sv, 0.7);
  EXPECT_NEAR(ev.apply(J), 0.0, 1e-12);
}

// Product rule on scalar coefficient: diff(sv * trace(eps), sv) ==
// trace(eps).
TEST_F(T2SDiffWrtScalarTest, ScalarCoefficientProductRule) {
  auto R = sv * trace(eps);
  auto J = diff(R, sv);
  ASSERT_TRUE(J.is_valid());
  using tdata = tensor_data<double, 3, 2>;
  tmech::tensor<double, 3, 2> eps_t = tmech::randn<double, 3, 2>();
  auto eps_ptr = std::make_shared<tdata>(eps_t);
  tensor_to_scalar_evaluator<double> ev;
  ev.set(eps, eps_ptr);
  ev.set_scalar(sv, 1.0);
  EXPECT_NEAR(ev.apply(J), ev.apply(trace(eps)), 1e-12);
}

} // namespace numsim::cas

#endif // TENSORTOSCALARDIFFERENTIATIONTEST_H
