#ifndef NUMERICALDIFFTEST_H
#define NUMERICALDIFFTEST_H

// Curated numerical-diff golden tests (#85).
//
// The fuzz tests (FuzzyScalarDiffTest, FuzzyTensorDiffTest) cover
// random smooth expressions and reproduce well under random seeds.
// What they DON'T give:
//   1. Named regression coverage per operator family.
//   2. Coverage of non-smooth operators (max/min, macauley, abs,
//      if_then_else, comparisons) at smooth points, where the
//      symbolic differentiation rule must still produce a result
//      that matches central difference.
//
// This file fills both gaps with deterministic, named tests.

#include "NumericalDiffHelpers.h"
#include "ScalarExpressionTest.h"

#include <gtest/gtest.h>

namespace numsim::cas::numerical_diff_test {

// Inherit the well-stocked fixture from ScalarExpressionTest.h:
// gives us x, y, z, a, b, c, d, _1, _2, _3 etc.
struct NumericalDiffFixture : ScalarFixture {};

// ─── Polynomial ────────────────────────────────────────────────────
// d/dx (x^3 - 2x + 1) = 3 x^2 - 2.  Exact polynomial derivative;
// central difference is also exact up to round-off, so this acts as
// a sanity probe on the harness itself.
TEST_F(NumericalDiffFixture, Polynomial) {
  auto e = x * x * x - _2 * x + _1;
  EXPECT_DIFF_MATCHES(e, x, 0.7);
  EXPECT_DIFF_MATCHES(e, x, -1.3);
  EXPECT_DIFF_MATCHES(e, x, 2.5);
  EXPECT_DIFF_MATCHES(e, x, 0.0);
}

// ─── Trigonometric ─────────────────────────────────────────────────
TEST_F(NumericalDiffFixture, Sin) {
  auto e = sin(x);
  EXPECT_DIFF_MATCHES(e, x, 0.0);
  EXPECT_DIFF_MATCHES(e, x, 0.7);
  EXPECT_DIFF_MATCHES(e, x, 1.5);
  EXPECT_DIFF_MATCHES(e, x, -2.3);
}

TEST_F(NumericalDiffFixture, Cos) {
  auto e = cos(x);
  EXPECT_DIFF_MATCHES(e, x, 0.0);
  EXPECT_DIFF_MATCHES(e, x, 0.7);
  EXPECT_DIFF_MATCHES(e, x, 1.5);
  EXPECT_DIFF_MATCHES(e, x, -2.3);
}

// Tan is checked away from its singularities at ±π/2.
TEST_F(NumericalDiffFixture, Tan) {
  auto e = tan(x);
  EXPECT_DIFF_MATCHES(e, x, 0.0);
  EXPECT_DIFF_MATCHES(e, x, 0.4);
  EXPECT_DIFF_MATCHES(e, x, -0.6);
  EXPECT_DIFF_MATCHES(e, x, 1.0);
}

// ─── Inverse trigonometric ─────────────────────────────────────────
// Inverse trig at points within their domain — well-behaved.
// Use 5-point near the domain edge because the 2nd derivative blows
// up there, eating the 2-point truncation budget.
TEST_F(NumericalDiffFixture, Asin) {
  auto e = asin(x);
  EXPECT_DIFF_MATCHES(e, x, 0.0);
  EXPECT_DIFF_MATCHES(e, x, 0.5);
  EXPECT_DIFF_MATCHES(e, x, -0.7);
  EXPECT_DIFF_MATCHES_5PT(e, x, 0.95);
}

TEST_F(NumericalDiffFixture, Acos) {
  auto e = acos(x);
  EXPECT_DIFF_MATCHES(e, x, 0.0);
  EXPECT_DIFF_MATCHES(e, x, 0.5);
  EXPECT_DIFF_MATCHES(e, x, -0.7);
  EXPECT_DIFF_MATCHES_5PT(e, x, 0.95);
}

TEST_F(NumericalDiffFixture, Atan) {
  auto e = atan(x);
  EXPECT_DIFF_MATCHES(e, x, 0.0);
  EXPECT_DIFF_MATCHES(e, x, 1.7);
  EXPECT_DIFF_MATCHES(e, x, -3.5);
  EXPECT_DIFF_MATCHES(e, x, 12.0);
}

// ─── Exp / log / sqrt ──────────────────────────────────────────────
TEST_F(NumericalDiffFixture, Exp) {
  auto e = exp(x);
  EXPECT_DIFF_MATCHES(e, x, 0.0);
  EXPECT_DIFF_MATCHES(e, x, 1.5);
  EXPECT_DIFF_MATCHES(e, x, -2.0);
}

TEST_F(NumericalDiffFixture, Log) {
  // Domain x > 0; pick well clear of 0.
  auto e = log(x);
  EXPECT_DIFF_MATCHES(e, x, 0.5);
  EXPECT_DIFF_MATCHES(e, x, 1.0);
  EXPECT_DIFF_MATCHES(e, x, 4.7);
}

TEST_F(NumericalDiffFixture, Sqrt) {
  // Pick well clear of 0, where d/dx sqrt is unbounded.
  auto e = sqrt(x);
  EXPECT_DIFF_MATCHES(e, x, 0.5);
  EXPECT_DIFF_MATCHES(e, x, 2.0);
  EXPECT_DIFF_MATCHES(e, x, 9.0);
}

// ─── Power ─────────────────────────────────────────────────────────
TEST_F(NumericalDiffFixture, PowIntegerExponent) {
  auto e = pow(x, _3); // d/dx = 3 x^2
  EXPECT_DIFF_MATCHES(e, x, 0.5);
  EXPECT_DIFF_MATCHES(e, x, -1.7);
  EXPECT_DIFF_MATCHES(e, x, 2.0);
}

TEST_F(NumericalDiffFixture, PowFractionalExponent) {
  // x^1.5 — derivative is 1.5 x^0.5 — domain x ≥ 0.
  auto half3 = make_scalar_constant(1.5);
  auto e = pow(x, half3);
  EXPECT_DIFF_MATCHES(e, x, 0.5);
  EXPECT_DIFF_MATCHES(e, x, 2.0);
  EXPECT_DIFF_MATCHES(e, x, 9.0);
}

// ─── Compositions ──────────────────────────────────────────────────
TEST_F(NumericalDiffFixture, SinOfXSquared) {
  // d/dx sin(x^2) = 2x cos(x^2). Composition of two rules.
  auto e = sin(x * x);
  EXPECT_DIFF_MATCHES(e, x, 0.0);
  EXPECT_DIFF_MATCHES(e, x, 0.6);
  EXPECT_DIFF_MATCHES(e, x, -1.1);
}

TEST_F(NumericalDiffFixture, ExpOfSin) {
  auto e = exp(sin(x));
  EXPECT_DIFF_MATCHES(e, x, 0.0);
  EXPECT_DIFF_MATCHES(e, x, 0.8);
  EXPECT_DIFF_MATCHES(e, x, -2.4);
}

TEST_F(NumericalDiffFixture, LogOfXSquaredPlusOne) {
  // Always positive — safe at 0.
  auto e = log(x * x + _1);
  EXPECT_DIFF_MATCHES(e, x, 0.0);
  EXPECT_DIFF_MATCHES(e, x, 1.2);
  EXPECT_DIFF_MATCHES(e, x, -3.5);
}

// ─── abs / sign on the smooth interior ─────────────────────────────
// |x| is smooth at x ≠ 0; central difference there agrees with the
// signum-based derivative. The singularity at x = 0 is intentionally
// excluded: that's a non-differentiable junction by design.
TEST_F(NumericalDiffFixture, AbsOnSmoothInterior) {
  auto e = abs(x);
  EXPECT_DIFF_MATCHES(e, x, 0.7);
  EXPECT_DIFF_MATCHES(e, x, -1.3);
}

// ─── Recently-landed operators: max / min ──────────────────────────
// d/dx max(x, c) at x > c is 1 (slope of x branch). At x < c it
// is 0 (slope of constant branch). The symbolic rule produces
// if_then_else(x > c, 1, 0) — verify both branches numerically.
TEST_F(NumericalDiffFixture, MaxAtUpperBranch) {
  auto e = max(x, _1); // = x for x > 1
  EXPECT_DIFF_MATCHES(e, x, 2.0);
  EXPECT_DIFF_MATCHES(e, x, 5.7);
}

TEST_F(NumericalDiffFixture, MaxAtLowerBranch) {
  auto e = max(x, _1); // = 1 for x < 1
  EXPECT_DIFF_MATCHES(e, x, 0.5);
  EXPECT_DIFF_MATCHES(e, x, -2.3);
}

TEST_F(NumericalDiffFixture, MinAtUpperBranch) {
  auto e = min(x, _1); // = 1 for x > 1
  EXPECT_DIFF_MATCHES(e, x, 2.0);
  EXPECT_DIFF_MATCHES(e, x, 5.7);
}

TEST_F(NumericalDiffFixture, MinAtLowerBranch) {
  auto e = min(x, _1); // = x for x < 1
  EXPECT_DIFF_MATCHES(e, x, 0.5);
  EXPECT_DIFF_MATCHES(e, x, -2.3);
}

// ─── Recently-landed operators: Macauley bracket ───────────────────
// macauley_plus(x) = max(0, x), so d/dx = 1 for x > 0, 0 for x < 0.
TEST_F(NumericalDiffFixture, MacauleyPlusPositiveSide) {
  auto e = macauley_plus(x);
  EXPECT_DIFF_MATCHES(e, x, 0.5);
  EXPECT_DIFF_MATCHES(e, x, 4.2);
}

TEST_F(NumericalDiffFixture, MacauleyPlusNegativeSide) {
  auto e = macauley_plus(x);
  EXPECT_DIFF_MATCHES(e, x, -0.7);
  EXPECT_DIFF_MATCHES(e, x, -3.1);
}

// macauley_minus(x) = max(0, -x), so d/dx = -1 for x < 0, 0 for x > 0.
TEST_F(NumericalDiffFixture, MacauleyMinusNegativeSide) {
  auto e = macauley_minus(x);
  EXPECT_DIFF_MATCHES(e, x, -0.7);
  EXPECT_DIFF_MATCHES(e, x, -3.1);
}

TEST_F(NumericalDiffFixture, MacauleyMinusPositiveSide) {
  auto e = macauley_minus(x);
  EXPECT_DIFF_MATCHES(e, x, 0.5);
  EXPECT_DIFF_MATCHES(e, x, 4.2);
}

// smoothed_macauley is smooth everywhere — including x = 0.
TEST_F(NumericalDiffFixture, SmoothedMacauleySmoothEverywhere) {
  auto eps = make_scalar_constant(0.1);
  auto e = smoothed_macauley(x, eps);
  EXPECT_DIFF_MATCHES(e, x, 0.0);
  EXPECT_DIFF_MATCHES(e, x, 1.0);
  EXPECT_DIFF_MATCHES(e, x, -1.0);
  EXPECT_DIFF_MATCHES(e, x, 3.5);
}

// ─── Recently-landed operators: if_then_else ───────────────────────
// A condition built from two literal constants folds at construction
// (`gt(_2, _1) → 1`, then `if_then_else(1, then, else) → then`),
// so the if_then_else node never reaches the differentiator. Lock
// in that the folded expression still differentiates correctly —
// regression coverage for the cond-fold path interaction with diff.
TEST_F(NumericalDiffFixture, IfThenElseConstantCondFoldsAtConstruction) {
  auto cond = gt(_2, _1);                      // folds to scalar_one
  auto e = if_then_else(cond, sin(x), cos(x)); // folds to sin(x)
  // Sanity: the construction-time fold should have collapsed the
  // outer node. The result must NOT still be an if_then_else.
  EXPECT_FALSE(::numsim::cas::is_same<scalar_if_then_else>(e))
      << "construction-time fold for constant cond did not fire";
  EXPECT_DIFF_MATCHES(e, x, 0.0);
  EXPECT_DIFF_MATCHES(e, x, 0.7);
  EXPECT_DIFF_MATCHES(e, x, -1.4);
}

// True symbolic-cond test: condition depends on another variable
// (y) that is held constant during differentiation w.r.t. x. The
// if_then_else node survives construction (cond is non-constant)
// so diff really exercises the if_then_else differentiation rule.
TEST_F(NumericalDiffFixture, IfThenElseSymbolicCondPreservesNode) {
  auto cond = gt(y, _zero); // y is symbolic — does not fold
  auto e = if_then_else(cond, sin(x), cos(x));
  EXPECT_TRUE(::numsim::cas::is_same<scalar_if_then_else>(e))
      << "non-constant cond should keep if_then_else node alive";

  // Bind y to a strictly positive value and check that d/dx
  // matches cos(x) (the "then" branch). Use the manual setup
  // because the EXPECT_DIFF_MATCHES macro only binds one var.
  auto dx_expr = ::numsim::cas::diff(e, x);
  scalar_evaluator<double> ev;
  ev.set(y, 1.7); // cond true → e behaves as sin(x)
  for (double x0 : {0.0, 0.7, -1.4}) {
    ev.set(x, x0 + 1e-5);
    double f_plus = ev.apply(e);
    ev.set(x, x0 - 1e-5);
    double f_minus = ev.apply(e);
    ev.set(x, x0);
    double sym = ev.apply(dx_expr);
    double num = (f_plus - f_minus) / 2e-5;
    EXPECT_NEAR(sym, num, 1e-5)
        << "y=1.7 (cond true) at x=" << x0 << ": sym=" << sym << " num=" << num;
  }

  // Bind y negative — cond false, e behaves as cos(x).
  ev.set(y, -1.7);
  for (double x0 : {0.0, 0.7, -1.4}) {
    ev.set(x, x0 + 1e-5);
    double f_plus = ev.apply(e);
    ev.set(x, x0 - 1e-5);
    double f_minus = ev.apply(e);
    ev.set(x, x0);
    double sym = ev.apply(dx_expr);
    double num = (f_plus - f_minus) / 2e-5;
    EXPECT_NEAR(sym, num, 1e-5) << "y=-1.7 (cond false) at x=" << x0
                                << ": sym=" << sym << " num=" << num;
  }
}

// When the condition depends on x but we evaluate strictly on one
// side of the boundary, the derivative is just the derivative of
// the chosen branch.
TEST_F(NumericalDiffFixture, IfThenElseXDependentCondPositiveSide) {
  auto e = if_then_else(gt(x, _zero), x * x, x * x * x);
  EXPECT_DIFF_MATCHES(e, x, 0.5); // strictly positive → 2x branch
  EXPECT_DIFF_MATCHES(e, x, 3.0);
}

TEST_F(NumericalDiffFixture, IfThenElseXDependentCondNegativeSide) {
  auto e = if_then_else(gt(x, _zero), x * x, x * x * x);
  EXPECT_DIFF_MATCHES(e, x, -0.5); // strictly negative → 3x^2 branch
  EXPECT_DIFF_MATCHES(e, x, -2.4);
}

// ─── Cross-operator composition ────────────────────────────────────
// Realistic constitutive-modeling style expression: a smoothed ramp
// composed with trig. Locks in the chain rule across all of:
// smoothed_macauley, sin, multiplication.
TEST_F(NumericalDiffFixture, SmoothedRampTimesTrig) {
  auto eps = make_scalar_constant(0.05);
  auto e = smoothed_macauley(x, eps) * sin(x);
  EXPECT_DIFF_MATCHES(e, x, 0.5);
  EXPECT_DIFF_MATCHES(e, x, -0.5);
  EXPECT_DIFF_MATCHES(e, x, 2.0);
}

// Multi-variable: differentiate a 2-variable expression w.r.t. each
// variable independently. Catches accidental cross-coupling in the
// differentiation visitor.
TEST_F(NumericalDiffFixture, MultiVariableSeparability) {
  auto e = sin(x) * exp(y) + x * x * y;
  scalar_evaluator<double> ev;
  ev.set(y, 1.3);
  // d/dx at x=0.5, y=1.3
  auto dx_expr = diff(e, x);
  ev.set(x, 0.5 + 1e-5);
  double fpx = ev.apply(e);
  ev.set(x, 0.5 - 1e-5);
  double fmx = ev.apply(e);
  ev.set(x, 0.5);
  double sym_dx = ev.apply(dx_expr);
  double num_dx = (fpx - fmx) / 2e-5;
  EXPECT_NEAR(sym_dx, num_dx, 1e-5);
  // d/dy at x=0.5, y=1.3
  auto dy_expr = diff(e, y);
  ev.set(y, 1.3 + 1e-5);
  double fpy = ev.apply(e);
  ev.set(y, 1.3 - 1e-5);
  double fmy = ev.apply(e);
  ev.set(y, 1.3);
  double sym_dy = ev.apply(dy_expr);
  double num_dy = (fpy - fmy) / 2e-5;
  EXPECT_NEAR(sym_dy, num_dy, 1e-5);
}

} // namespace numsim::cas::numerical_diff_test

#endif // NUMERICALDIFFTEST_H
