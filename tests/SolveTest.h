#ifndef SOLVETEST_H
#define SOLVETEST_H

#include "cas_test_helpers.h"
#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"

#include <numsim_cas/scalar/visitors/scalar_evaluator.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor/visitors/tensor_evaluator.h>

#include <cmath>
#include <tuple>

// ============================================================================
// Scalar Solve Tests
// ============================================================================

struct ScalarSolveFixture : ::testing::Test {
  using scalar_expr =
      numsim::cas::expression_holder<numsim::cas::scalar_expression>;

  scalar_expr x, y;
  scalar_expr a, b, c;
  scalar_expr _1, _2, _3, _4;

  ScalarSolveFixture() {
    std::tie(x, y) = numsim::cas::make_scalar_variable("x", "y");
    std::tie(a, b, c) = numsim::cas::make_scalar_variable("a", "b", "c");
    std::tie(_1, _2, _3) = numsim::cas::make_scalar_constant(1, 2, 3);
    _4 = numsim::cas::make_scalar_constant(4);
  }
};

using std::pow;
using std::sqrt;

// --- Coefficient extraction ---

TEST_F(ScalarSolveFixture, PolyCoeff_Constant) {
  auto coeffs = numsim::cas::polynomial_coefficients(_3, x);
  ASSERT_TRUE(coeffs.has_value());
  EXPECT_EQ(coeffs->size(), 1u);
  EXPECT_PRINT(coeffs->at(0), "3");
}

TEST_F(ScalarSolveFixture, PolyCoeff_Linear) {
  // 2*x + 3
  auto expr = _2 * x + _3;
  auto coeffs = numsim::cas::polynomial_coefficients(expr, x);
  ASSERT_TRUE(coeffs.has_value());
  EXPECT_PRINT(coeffs->at(0), "3");
  EXPECT_PRINT(coeffs->at(1), "2");
}

TEST_F(ScalarSolveFixture, PolyCoeff_Quadratic) {
  // x^2 - 4
  auto expr = pow(x, _2) - _4;
  auto coeffs = numsim::cas::polynomial_coefficients(expr, x);
  ASSERT_TRUE(coeffs.has_value());
  EXPECT_PRINT(coeffs->at(2), "1");
}

TEST_F(ScalarSolveFixture, PolyCoeff_SymbolicCoeffs) {
  // a*x + b
  auto expr = a * x + b;
  auto coeffs = numsim::cas::polynomial_coefficients(expr, x);
  ASSERT_TRUE(coeffs.has_value());
  EXPECT_PRINT(coeffs->at(0), "b");
  EXPECT_PRINT(coeffs->at(1), "a");
}

TEST_F(ScalarSolveFixture, PolyCoeff_NonPolynomial) {
  // sin(x) is not polynomial
  auto expr = numsim::cas::sin(x);
  auto coeffs = numsim::cas::polynomial_coefficients(expr, x);
  EXPECT_FALSE(coeffs.has_value());
}

TEST_F(ScalarSolveFixture, PolyCoeff_Undistributed) {
  // 2*(x+1) stays as mul(2, add(x,1)) — not distributed
  auto expr = _2 * (x + _1);
  auto coeffs = numsim::cas::polynomial_coefficients(expr, x);
  ASSERT_TRUE(coeffs.has_value());
  // Should have degree 0 and degree 1 terms
  ASSERT_TRUE(coeffs->count(0) > 0 || coeffs->count(1) > 0);
  EXPECT_PRINT(coeffs->at(1), "2");
  EXPECT_PRINT(coeffs->at(0), "2");
}

TEST_F(ScalarSolveFixture, PolyCoeff_JustX) {
  auto coeffs = numsim::cas::polynomial_coefficients(x, x);
  ASSERT_TRUE(coeffs.has_value());
  EXPECT_EQ(coeffs->size(), 1u);
  EXPECT_PRINT(coeffs->at(1), "1");
}

// --- Linear solve ---

TEST_F(ScalarSolveFixture, Solve_LinearSimple) {
  // x - 3 = 0  →  x = 3
  auto solutions = numsim::cas::solve(x - _3, x);
  ASSERT_EQ(solutions.size(), 1u);
  EXPECT_PRINT(solutions[0], "3");
}

TEST_F(ScalarSolveFixture, Solve_LinearWithCoeff) {
  // 2*x - 4 = 0  →  x = 2
  auto solutions = numsim::cas::solve(_2 * x - _4, x);
  ASSERT_EQ(solutions.size(), 1u);
  EXPECT_PRINT(solutions[0], "2");
}

TEST_F(ScalarSolveFixture, Solve_LinearSymbolic) {
  // a*x + b = 0  →  x = -b/a
  auto solutions = numsim::cas::solve(a * x + b, x);
  ASSERT_EQ(solutions.size(), 1u);
  EXPECT_PRINT(solutions[0], testcas::S(-b / a));
}

TEST_F(ScalarSolveFixture, Solve_JustX) {
  // x = 0  →  x = 0
  auto solutions = numsim::cas::solve(x, x);
  ASSERT_EQ(solutions.size(), 1u);
  EXPECT_PRINT(solutions[0], "0");
}

// --- Quadratic solve ---

TEST_F(ScalarSolveFixture, Solve_QuadraticSimple) {
  // x^2 - 1 = 0  →  x = 1 or x = -1
  auto solutions = numsim::cas::solve(pow(x, _2) - _1, x);
  ASSERT_EQ(solutions.size(), 2u);
  numsim::cas::scalar_evaluator<double> ev;
  EXPECT_NEAR(ev.apply(solutions[0]), 1.0, 1e-12);
  EXPECT_NEAR(ev.apply(solutions[1]), -1.0, 1e-12);
}

TEST_F(ScalarSolveFixture, Solve_QuadraticDoubleRoot) {
  // x^2 - 2x + 1 = 0  →  x = 1 (double root)
  auto solutions = numsim::cas::solve(pow(x, _2) - _2 * x + _1, x);
  ASSERT_EQ(solutions.size(), 1u);
  EXPECT_PRINT(solutions[0], "1");
}

TEST_F(ScalarSolveFixture, Solve_QuadraticSymbolic) {
  // a*x^2 + b*x + c = 0  →  quadratic formula
  auto solutions = numsim::cas::solve(a * pow(x, _2) + b * x + c, x);
  ASSERT_EQ(solutions.size(), 2u);
}

TEST_F(ScalarSolveFixture, Solve_QuadraticX2Minus4) {
  // x^2 - 4 = 0  →  x = 2 or x = -2
  auto solutions = numsim::cas::solve(pow(x, _2) - _4, x);
  ASSERT_EQ(solutions.size(), 2u);
  numsim::cas::scalar_evaluator<double> ev;
  EXPECT_NEAR(ev.apply(solutions[0]), 2.0, 1e-12);
  EXPECT_NEAR(ev.apply(solutions[1]), -2.0, 1e-12);
}

// --- Edge cases ---

TEST_F(ScalarSolveFixture, Solve_NoVariable) {
  // 3 + 2 = 0 — no variable x
  auto solutions = numsim::cas::solve(_3 + _2, x);
  EXPECT_TRUE(solutions.empty());
}

TEST_F(ScalarSolveFixture, Solve_NonPolynomial) {
  // sin(x) = 0 — not polynomial
  auto solutions = numsim::cas::solve(numsim::cas::sin(x), x);
  EXPECT_TRUE(solutions.empty());
}

TEST_F(ScalarSolveFixture, Solve_Cubic) {
  // x^3 - 1 = 0 — cubic not supported
  auto solutions =
      numsim::cas::solve(pow(x, numsim::cas::make_scalar_constant(3)) - _1, x);
  EXPECT_TRUE(solutions.empty());
}

TEST_F(ScalarSolveFixture, Solve_WrongVariable) {
  // x + 1 = 0 solved for y — y not in equation
  auto solutions = numsim::cas::solve(x + _1, y);
  EXPECT_TRUE(solutions.empty());
}

// ============================================================================
// Tensor Solve Tests
// ============================================================================

namespace {

using numsim::cas::expression_holder;
using numsim::cas::make_expression;
using numsim::cas::tensor;
using numsim::cas::tensor_data;
using numsim::cas::tensor_evaluator;
using numsim::cas::tensor_expression;
using numsim::cas::tensor_zero;

template <std::size_t Dim, std::size_t Rank>
auto make_solve_test_data(std::initializer_list<double> values) {
  auto ptr = std::make_shared<tensor_data<double, Dim, Rank>>();
  auto *raw = ptr->raw_data();
  std::size_t i = 0;
  for (auto v : values)
    raw[i++] = v;
  return ptr;
}

template <std::size_t Dim, std::size_t Rank>
tmech::tensor<double, Dim, Rank>
make_solve_tmech(std::initializer_list<double> values) {
  tmech::tensor<double, Dim, Rank> t;
  auto *raw = t.raw_data();
  std::size_t i = 0;
  for (auto v : values)
    raw[i++] = v;
  return t;
}

template <std::size_t Dim, std::size_t Rank>
auto const &solve_as_tmech(numsim::cas::tensor_data_base<double> const &data) {
  return static_cast<tensor_data<double, Dim, Rank> const &>(data).data();
}

constexpr double solve_tol = 1e-10;

} // namespace

TEST(TensorSolve, LinearXMinusA) {
  // solve(X - A, X)  →  A
  auto X = make_expression<tensor>("X", 2, 2);
  auto A = make_expression<tensor>("A", 2, 2);

  auto solutions = numsim::cas::solve(X - A, X);
  ASSERT_EQ(solutions.size(), 1u);

  // Evaluate numerically
  tensor_evaluator<double> ev;
  // clang-format off
  ev.set(A, make_solve_test_data<2, 2>({1.0, 2.0,
                                          3.0, 4.0}));
  // clang-format on
  auto result = ev.apply(solutions[0]);
  ASSERT_NE(result, nullptr);
  auto expected = make_solve_tmech<2, 2>({1.0, 2.0, 3.0, 4.0});
  EXPECT_TRUE(
      tmech::almost_equal(solve_as_tmech<2, 2>(*result), expected, solve_tol));
}

TEST(TensorSolve, Linear2XMinusA) {
  // solve(2*X - A, X)  →  A/2
  auto X = make_expression<tensor>("X", 2, 2);
  auto A = make_expression<tensor>("A", 2, 2);

  auto two = numsim::cas::make_scalar_constant(2);
  auto solutions = numsim::cas::solve(two * X - A, X);
  ASSERT_EQ(solutions.size(), 1u);

  tensor_evaluator<double> ev;
  // clang-format off
  ev.set(A, make_solve_test_data<2, 2>({2.0, 4.0,
                                          6.0, 8.0}));
  // clang-format on
  auto result = ev.apply(solutions[0]);
  ASSERT_NE(result, nullptr);
  auto expected = make_solve_tmech<2, 2>({1.0, 2.0, 3.0, 4.0});
  EXPECT_TRUE(
      tmech::almost_equal(solve_as_tmech<2, 2>(*result), expected, solve_tol));
}

TEST(TensorSolve, LinearXPlusA) {
  // solve(X + A, X)  →  -A
  auto X = make_expression<tensor>("X", 2, 2);
  auto A = make_expression<tensor>("A", 2, 2);

  auto solutions = numsim::cas::solve(X + A, X);
  ASSERT_EQ(solutions.size(), 1u);

  tensor_evaluator<double> ev;
  // clang-format off
  ev.set(A, make_solve_test_data<2, 2>({1.0, 2.0,
                                          3.0, 4.0}));
  // clang-format on
  auto result = ev.apply(solutions[0]);
  ASSERT_NE(result, nullptr);
  auto expected = make_solve_tmech<2, 2>({-1.0, -2.0, -3.0, -4.0});
  EXPECT_TRUE(
      tmech::almost_equal(solve_as_tmech<2, 2>(*result), expected, solve_tol));
}

TEST(TensorSolve, NonlinearXSquared) {
  // solve(X*X - A, X)  →  {} (nonlinear, X appears in derivative)
  auto X = make_expression<tensor>("X", 2, 2);
  auto A = make_expression<tensor>("A", 2, 2);

  auto XX = numsim::cas::inner_product(X, numsim::cas::sequence{2}, X,
                                       numsim::cas::sequence{1});
  auto solutions = numsim::cas::solve(XX - A, X);
  EXPECT_TRUE(solutions.empty());
}

TEST(TensorSolve, LinearAMinusX) {
  // solve(A - X, X)  →  A  (D = -I4, tests tensor_negative(identity) path)
  auto X = make_expression<tensor>("X", 2, 2);
  auto A = make_expression<tensor>("A", 2, 2);

  auto solutions = numsim::cas::solve(A - X, X);
  ASSERT_EQ(solutions.size(), 1u);

  tensor_evaluator<double> ev;
  // clang-format off
  ev.set(A, make_solve_test_data<2, 2>({5.0, 6.0,
                                          7.0, 8.0}));
  // clang-format on
  auto result = ev.apply(solutions[0]);
  ASSERT_NE(result, nullptr);
  auto expected = make_solve_tmech<2, 2>({5.0, 6.0, 7.0, 8.0});
  EXPECT_TRUE(
      tmech::almost_equal(solve_as_tmech<2, 2>(*result), expected, solve_tol));
}

TEST(TensorSolve, LinearXMinusA_Dim3) {
  // solve(X - A, X)  →  A  (dim=3 to verify generality)
  auto X = make_expression<tensor>("X", 3, 2);
  auto A = make_expression<tensor>("A", 3, 2);

  auto solutions = numsim::cas::solve(X - A, X);
  ASSERT_EQ(solutions.size(), 1u);

  tensor_evaluator<double> ev;
  // clang-format off
  ev.set(A, make_solve_test_data<3, 2>({1.0, 2.0, 3.0,
                                          4.0, 5.0, 6.0,
                                          7.0, 8.0, 9.0}));
  // clang-format on
  auto result = ev.apply(solutions[0]);
  ASSERT_NE(result, nullptr);
  auto expected =
      make_solve_tmech<3, 2>({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0});
  EXPECT_TRUE(
      tmech::almost_equal(solve_as_tmech<3, 2>(*result), expected, solve_tol));
}

TEST(TensorSolve, VariableNotInEquation) {
  // solve(A + B, X)  →  {} (X not in equation)
  auto X = make_expression<tensor>("X", 2, 2);
  auto A = make_expression<tensor>("A", 2, 2);
  auto B = make_expression<tensor>("B", 2, 2);

  auto solutions = numsim::cas::solve(A + B, X);
  EXPECT_TRUE(solutions.empty());
}

#endif // SOLVETEST_H
