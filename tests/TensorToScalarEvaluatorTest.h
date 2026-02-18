#ifndef TENSORTOSCALAREVALUATORTEST_H
#define TENSORTOSCALAREVALUATORTEST_H

#include <cmath>
#include <gtest/gtest.h>
#include <memory>

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_functions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_std.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_evaluator.h>

namespace numsim::cas {

namespace {
constexpr double t2s_tol = 1e-12;
} // namespace

// ─── Individual node tests ────────────────────────────────────────────

TEST(T2sEval, EvalT2sZero) {
  tensor_to_scalar_evaluator<double> ev;
  auto expr = make_expression<tensor_to_scalar_zero>();
  EXPECT_NEAR(ev.apply(expr), 0.0, t2s_tol);
}

TEST(T2sEval, EvalT2sOne) {
  tensor_to_scalar_evaluator<double> ev;
  auto expr = make_expression<tensor_to_scalar_one>();
  EXPECT_NEAR(ev.apply(expr), 1.0, t2s_tol);
}

TEST(T2sEval, EvalT2sScalarWrapper) {
  tensor_to_scalar_evaluator<double> ev;
  auto c = make_expression<scalar_constant>(42.0);
  auto expr = make_expression<tensor_to_scalar_scalar_wrapper>(c);
  EXPECT_NEAR(ev.apply(expr), 42.0, t2s_tol);
}

TEST(T2sEval, EvalTrace2x2) {
  tensor_to_scalar_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  // A = [1 2; 3 4], trace = 5
  ev.set(A, make_test_data<2, 2>({1.0, 2.0, 3.0, 4.0}));
  auto expr = trace(A);
  EXPECT_NEAR(ev.apply(expr), 5.0, t2s_tol);
}

TEST(T2sEval, EvalTrace3x3) {
  tensor_to_scalar_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  // clang-format off
  ev.set(A, make_test_data<3, 2>({1.0, 0.0, 0.0,
                                   0.0, 5.0, 0.0,
                                   0.0, 0.0, 9.0}));
  // clang-format on
  auto expr = trace(A);
  EXPECT_NEAR(ev.apply(expr), 15.0, t2s_tol);
}

TEST(T2sEval, EvalDet2x2) {
  tensor_to_scalar_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  // A = [1 2; 3 4], det = 1*4 - 2*3 = -2
  ev.set(A, make_test_data<2, 2>({1.0, 2.0, 3.0, 4.0}));
  auto expr = det(A);
  EXPECT_NEAR(ev.apply(expr), -2.0, t2s_tol);
}

TEST(T2sEval, EvalDet3x3) {
  tensor_to_scalar_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  // clang-format off
  ev.set(A, make_test_data<3, 2>({2.0, 1.0, 0.0,
                                   1.0, 3.0, 1.0,
                                   0.0, 1.0, 2.0}));
  // clang-format on
  auto A_val = make_tmech<3, 2>(
      {2.0, 1.0, 0.0, 1.0, 3.0, 1.0, 0.0, 1.0, 2.0});
  auto expr = det(A);
  EXPECT_NEAR(ev.apply(expr), tmech::det(A_val), t2s_tol);
}

TEST(T2sEval, EvalNorm) {
  tensor_to_scalar_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  // A = [1 2; 3 4], Frobenius norm = sqrt(1+4+9+16) = sqrt(30)
  ev.set(A, make_test_data<2, 2>({1.0, 2.0, 3.0, 4.0}));
  auto expr = norm(A);
  EXPECT_NEAR(ev.apply(expr), std::sqrt(30.0), t2s_tol);
}

TEST(T2sEval, EvalDot) {
  tensor_to_scalar_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  // dot(A) = A:A = sum of squares = 1+4+9+16 = 30
  ev.set(A, make_test_data<2, 2>({1.0, 2.0, 3.0, 4.0}));
  auto expr = dot(A);
  EXPECT_NEAR(ev.apply(expr), 30.0, t2s_tol);
}

TEST(T2sEval, EvalInnerProductToScalar) {
  tensor_to_scalar_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto B = make_expression<tensor>("B", 2, 2);
  // clang-format off
  ev.set(A, make_test_data<2, 2>({1.0, 2.0,
                                   3.0, 4.0}));
  ev.set(B, make_test_data<2, 2>({5.0, 6.0,
                                   7.0, 8.0}));
  // clang-format on
  // A:B = 1*5 + 2*6 + 3*7 + 4*8 = 5+12+21+32 = 70
  auto expr = dot_product(A, sequence{1, 2}, B, sequence{1, 2});
  EXPECT_NEAR(ev.apply(expr), 70.0, t2s_tol);
}

TEST(T2sEval, EvalT2sNegative) {
  tensor_to_scalar_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  ev.set(A, make_test_data<2, 2>({1.0, 2.0, 3.0, 4.0}));
  auto expr = -trace(A);
  EXPECT_NEAR(ev.apply(expr), -5.0, t2s_tol);
}

TEST(T2sEval, EvalT2sLog) {
  tensor_to_scalar_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  // trace = 5, log(5)
  ev.set(A, make_test_data<2, 2>({1.0, 2.0, 3.0, 4.0}));
  auto expr = log(trace(A));
  EXPECT_NEAR(ev.apply(expr), std::log(5.0), t2s_tol);
}

TEST(T2sEval, EvalT2sAdd) {
  tensor_to_scalar_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto B = make_expression<tensor>("B", 2, 2);
  ev.set(A, make_test_data<2, 2>({1.0, 2.0, 3.0, 4.0}));
  ev.set(B, make_test_data<2, 2>({5.0, 6.0, 7.0, 8.0}));
  // trace(A) + trace(B) = 5 + 13 = 18
  auto expr = trace(A) + trace(B);
  EXPECT_NEAR(ev.apply(expr), 18.0, t2s_tol);
}

TEST(T2sEval, EvalT2sMul) {
  tensor_to_scalar_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto B = make_expression<tensor>("B", 2, 2);
  ev.set(A, make_test_data<2, 2>({1.0, 2.0, 3.0, 4.0}));
  ev.set(B, make_test_data<2, 2>({5.0, 6.0, 7.0, 8.0}));
  // trace(A) * trace(B) = 5 * 13 = 65
  auto expr = trace(A) * trace(B);
  EXPECT_NEAR(ev.apply(expr), 65.0, t2s_tol);
}

TEST(T2sEval, EvalT2sPow) {
  tensor_to_scalar_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  ev.set(A, make_test_data<2, 2>({1.0, 2.0, 3.0, 4.0}));
  // trace(A)^3 = 5^3 = 125
  auto expr = pow(trace(A), 3);
  EXPECT_NEAR(ev.apply(expr), 125.0, t2s_tol);
}

// ─── Combination tests ────────────────────────────────────────────────

TEST(T2sEval, EvalDetPlusTrace) {
  tensor_to_scalar_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  ev.set(A, make_test_data<2, 2>({1.0, 2.0, 3.0, 4.0}));
  // det(A) + trace(A) = -2 + 5 = 3
  auto expr = det(A) + trace(A);
  EXPECT_NEAR(ev.apply(expr), 3.0, t2s_tol);
}

TEST(T2sEval, EvalLogDet) {
  tensor_to_scalar_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  // clang-format off
  ev.set(A, make_test_data<3, 2>({2.0, 0.0, 0.0,
                                   0.0, 3.0, 0.0,
                                   0.0, 0.0, 5.0}));
  // clang-format on
  // det = 30, log(30)
  auto expr = log(det(A));
  EXPECT_NEAR(ev.apply(expr), std::log(30.0), t2s_tol);
}

TEST(T2sEval, EvalNormSquaredVsDot) {
  tensor_to_scalar_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  ev.set(A, make_test_data<2, 2>({1.0, 2.0, 3.0, 4.0}));
  // norm(A)^2 should equal dot(A) = A:A
  auto norm_sq = pow(norm(A), 2);
  auto dot_val = dot(A);
  EXPECT_NEAR(ev.apply(norm_sq), ev.apply(dot_val), t2s_tol);
}

TEST(T2sEval, EvalTraceOfSum) {
  tensor_to_scalar_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto B = make_expression<tensor>("B", 2, 2);
  // clang-format off
  ev.set(A, make_test_data<2, 2>({1.0, 2.0,
                                   3.0, 4.0}));
  ev.set(B, make_test_data<2, 2>({5.0, 6.0,
                                   7.0, 8.0}));
  // clang-format on
  // trace(A+B) = trace(A) + trace(B) = 5 + 13 = 18
  auto lhs = trace(A + B);
  auto rhs = trace(A) + trace(B);
  EXPECT_NEAR(ev.apply(lhs), ev.apply(rhs), t2s_tol);
}

TEST(T2sEval, EvalScalarTimesTrace) {
  tensor_to_scalar_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto x = make_expression<scalar>("x");
  ev.set(A, make_test_data<2, 2>({1.0, 2.0, 3.0, 4.0}));
  ev.set_scalar(x, 3.0);
  // x * trace(A) = 3 * 5 = 15
  auto x_wrapped = make_expression<tensor_to_scalar_scalar_wrapper>(x);
  auto expr = x_wrapped * trace(A);
  EXPECT_NEAR(ev.apply(expr), 15.0, t2s_tol);
}

// ─── Error tests ──────────────────────────────────────────────────────

TEST(T2sEval, EvalMissingTensorSymbol) {
  tensor_to_scalar_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  EXPECT_THROW(ev.apply(trace(A)), evaluation_error);
}

TEST(T2sEval, EvalMissingScalarSymbol) {
  tensor_to_scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  auto expr = make_expression<tensor_to_scalar_scalar_wrapper>(x);
  EXPECT_THROW(ev.apply(expr), evaluation_error);
}

TEST(T2sEval, EvaluationErrorIsCatchableAsCasError) {
  tensor_to_scalar_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  EXPECT_THROW(ev.apply(trace(A)), cas_error);
}

TEST(T2sEval, EvaluationErrorIsCatchableAsRuntimeError) {
  tensor_to_scalar_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  EXPECT_THROW(ev.apply(trace(A)), std::runtime_error);
}

} // namespace numsim::cas

#endif // TENSORTOSCALAREVALUATORTEST_H
