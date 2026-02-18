#ifndef SCALAREVALUATORTEST_H
#define SCALAREVALUATORTEST_H

#include <cmath>
#include <gtest/gtest.h>
#include <numbers>

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/scalar/visitors/scalar_evaluator.h>

namespace numsim::cas {

namespace {
using expr_t = expression_holder<scalar_expression>;
} // namespace

// --- Individual operator() tests ---

TEST(ScalarEval, EvalScalarSymbol) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, 3.0);
  EXPECT_NEAR(ev.apply(x), 3.0, 1e-12);
}

TEST(ScalarEval, EvalScalarZero) {
  scalar_evaluator<double> ev;
  EXPECT_NEAR(ev.apply(get_scalar_zero()), 0.0, 1e-12);
}

TEST(ScalarEval, EvalScalarOne) {
  scalar_evaluator<double> ev;
  EXPECT_NEAR(ev.apply(get_scalar_one()), 1.0, 1e-12);
}

TEST(ScalarEval, EvalScalarConstant) {
  scalar_evaluator<double> ev;
  auto c = make_expression<scalar_constant>(42.0);
  EXPECT_NEAR(ev.apply(c), 42.0, 1e-12);
}

TEST(ScalarEval, EvalScalarAdd) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, 3.0);
  auto result = ev.apply(x + make_scalar_constant(2));
  EXPECT_NEAR(result, 5.0, 1e-12);
}

TEST(ScalarEval, EvalScalarMul) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, 3.0);
  auto result = ev.apply(x * make_scalar_constant(4));
  EXPECT_NEAR(result, 12.0, 1e-12);
}

TEST(ScalarEval, EvalScalarNegative) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, 3.0);
  EXPECT_NEAR(ev.apply(-x), -3.0, 1e-12);
}

TEST(ScalarEval, EvalScalarPow) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, 2.0);
  auto result = ev.apply(pow(x, make_scalar_constant(3)));
  EXPECT_NEAR(result, 8.0, 1e-12);
}

TEST(ScalarEval, EvalScalarRational) {
  scalar_evaluator<double> ev;
  auto num = make_scalar_constant(1);
  auto den = make_scalar_constant(3);
  auto r = make_expression<scalar_rational>(num, den);
  EXPECT_NEAR(ev.apply(r), 1.0 / 3.0, 1e-12);
}

TEST(ScalarEval, EvalScalarSin) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, std::numbers::pi / 6.0);
  EXPECT_NEAR(ev.apply(sin(x)), 0.5, 1e-12);
}

TEST(ScalarEval, EvalScalarCos) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, std::numbers::pi / 3.0);
  EXPECT_NEAR(ev.apply(cos(x)), 0.5, 1e-12);
}

TEST(ScalarEval, EvalScalarTan) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, std::numbers::pi / 4.0);
  EXPECT_NEAR(ev.apply(tan(x)), 1.0, 1e-12);
}

TEST(ScalarEval, EvalScalarAsin) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, 0.5);
  EXPECT_NEAR(ev.apply(asin(x)), std::asin(0.5), 1e-12);
}

TEST(ScalarEval, EvalScalarAcos) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, 0.5);
  EXPECT_NEAR(ev.apply(acos(x)), std::acos(0.5), 1e-12);
}

TEST(ScalarEval, EvalScalarAtan) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, 1.0);
  EXPECT_NEAR(ev.apply(atan(x)), std::numbers::pi / 4.0, 1e-12);
}

TEST(ScalarEval, EvalScalarSqrt) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, 4.0);
  EXPECT_NEAR(ev.apply(sqrt(x)), 2.0, 1e-12);
}

TEST(ScalarEval, EvalScalarLog) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, std::numbers::e);
  EXPECT_NEAR(ev.apply(log(x)), 1.0, 1e-12);
}

TEST(ScalarEval, EvalScalarExp) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, 1.0);
  EXPECT_NEAR(ev.apply(exp(x)), std::numbers::e, 1e-12);
}

TEST(ScalarEval, EvalScalarSign) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, 5.0);
  EXPECT_NEAR(ev.apply(sign(x)), 1.0, 1e-12);
  EXPECT_NEAR(ev.apply(sign(-x)), -1.0, 1e-12);

  scalar_evaluator<double> ev2;
  auto z = make_expression<scalar>("z");
  ev2.set(z, 0.0);
  EXPECT_NEAR(ev2.apply(sign(z)), 0.0, 1e-12);
}

TEST(ScalarEval, EvalScalarAbs) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, 3.0);
  EXPECT_NEAR(ev.apply(abs(-x)), 3.0, 1e-12);
}

TEST(ScalarEval, EvalScalarFunction) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, 3.0);
  auto f = make_scalar_function("f", x * x);
  EXPECT_NEAR(ev.apply(f), 9.0, 1e-12);
}

// --- Combination tests ---

TEST(ScalarEval, EvalPolynomial) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, 3.0);
  auto expr = x * x + 2 * x + 1;
  EXPECT_NEAR(ev.apply(expr), 16.0, 1e-12);
}

TEST(ScalarEval, EvalMultiVariable) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  ev.set(x, 2.0);
  ev.set(y, 3.0);
  auto expr = x * y + y;
  EXPECT_NEAR(ev.apply(expr), 9.0, 1e-12);
}

TEST(ScalarEval, EvalNestedTrig) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, 0.0);
  EXPECT_NEAR(ev.apply(sin(cos(x))), std::sin(1.0), 1e-12);
}

TEST(ScalarEval, EvalChainPowExp) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, 1.0);
  auto expr = exp(pow(x, make_scalar_constant(2)));
  EXPECT_NEAR(ev.apply(expr), std::numbers::e, 1e-12);
}

TEST(ScalarEval, EvalComplexArithmetic) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  ev.set(x, 5.0);
  ev.set(y, 3.0);
  auto expr = (x + y) * (x - y);
  EXPECT_NEAR(ev.apply(expr), 16.0, 1e-12);
}

TEST(ScalarEval, EvalNegativeChain) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  ev.set(x, 2.0);
  ev.set(y, 3.0);
  auto expr = -(x * -y);
  EXPECT_NEAR(ev.apply(expr), 6.0, 1e-12);
}

TEST(ScalarEval, EvalAddWithCoeff) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  ev.set(x, 1.0);
  ev.set(y, 2.0);
  auto expr = 3 * x + 2 * y + 1;
  EXPECT_NEAR(ev.apply(expr), 8.0, 1e-12);
}

TEST(ScalarEval, EvalMulWithCoeff) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  ev.set(x, 3.0);
  ev.set(y, 4.0);
  auto expr = 2 * (x * y);
  EXPECT_NEAR(ev.apply(expr), 24.0, 1e-12);
}

TEST(ScalarEval, EvalDivisionViaReciprocal) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  ev.set(x, 6.0);
  ev.set(y, 3.0);
  auto expr = x / y;
  EXPECT_NEAR(ev.apply(expr), 2.0, 1e-12);
}

TEST(ScalarEval, EvalTrigIdentity) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, 1.23);
  auto expr = sin(x) * sin(x) + cos(x) * cos(x);
  EXPECT_NEAR(ev.apply(expr), 1.0, 1e-12);
}

TEST(ScalarEval, EvalLogExpInverse) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, 2.5);
  EXPECT_NEAR(ev.apply(log(exp(x))), 2.5, 1e-12);
}

TEST(ScalarEval, EvalSqrtOfSquare) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  ev.set(x, 7.0);
  EXPECT_NEAR(ev.apply(sqrt(x * x)), 7.0, 1e-12);
}

// --- Error tests ---

TEST(ScalarEval, EvalMissingSymbolThrows) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  EXPECT_THROW(ev.apply(x), evaluation_error);
}

TEST(ScalarEval, EvalMissingSymbolInCompoundExpr) {
  // Only x is set, y is missing — fails inside compound expression
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  ev.set(x, 3.0);
  EXPECT_THROW(ev.apply(x + y), evaluation_error);
}

TEST(ScalarEval, EvalMissingSymbolInNestedExpr) {
  // Missing symbol buried inside sin(x + y)
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  ev.set(x, 1.0);
  EXPECT_THROW(ev.apply(sin(x + y)), evaluation_error);
}

TEST(ScalarEval, EvaluationErrorIsCatchableAsCasError) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  EXPECT_THROW(ev.apply(x), cas_error);
}

TEST(ScalarEval, EvaluationErrorIsCatchableAsRuntimeError) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  EXPECT_THROW(ev.apply(x), std::runtime_error);
}

TEST(ScalarEval, EvaluationErrorCarriesMessage) {
  scalar_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  try {
    ev.apply(x);
    FAIL() << "Expected evaluation_error";
  } catch (evaluation_error const &e) {
    EXPECT_TRUE(std::string(e.what()).find("symbol not found") !=
                std::string::npos);
  }
}

} // namespace numsim::cas

#endif // SCALAREVALUATORTEST_H
