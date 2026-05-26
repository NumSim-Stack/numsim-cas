#ifndef NUMSIM_CAS_TESTS_NUMERICAL_DIFF_HELPERS_H
#define NUMSIM_CAS_TESTS_NUMERICAL_DIFF_HELPERS_H

// Curated numerical-diff verification helpers (#85).
//
// These complement the random fuzz tests (FuzzyScalarDiffTest /
// FuzzyTensorDiffTest) by giving deterministic, named coverage:
// pin one symbolic expression at one evaluation point, check that
// d(expr)/d(var) — computed symbolically by `diff()` and then
// evaluated — matches a central-difference estimate of the same
// derivative.
//
// Tolerance budget mirrors the fuzz tester's published numbers
// (FuzzyScalarDiffTest.h:240-241): abs 1e-6, rel 1e-4. The
// magnitude threshold for stiff-region skipping is intentionally
// dropped — curated tests should only pick non-stiff evaluation
// points to begin with.

#include "cas_test_helpers.h"

#include <numsim_cas/core/diff.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_diff.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/scalar/visitors/scalar_evaluator.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <sstream>

namespace numsim::cas::numerical_diff_test {

using scalar_expr = expression_holder<scalar_expression>;

// Evaluate a scalar expression at a single variable bound to `x0`.
// Convenience over inlining the 3-line evaluator-setup pattern at
// every call site.
inline double eval_at(scalar_expr const &expr, scalar_expr const &var,
                      double x0) {
  scalar_evaluator<double> ev;
  ev.set(var, x0);
  return ev.apply(expr);
}

// Central-difference estimate of d(expr)/d(var) at `var = x0`.
// Default `h = 1e-5` chosen to match the established fuzz pattern
// — small enough to keep truncation error O(h^2) ~ 1e-10 yet large
// enough that double-precision round-off (~ 1e-16 / h ~ 1e-11)
// stays well below the 1e-6 acceptance band.
inline double central_diff_scalar(scalar_expr const &expr,
                                  scalar_expr const &var, double x0,
                                  double h = 1e-5) {
  scalar_evaluator<double> ev;
  ev.set(var, x0 + h);
  double f_plus = ev.apply(expr);
  ev.set(var, x0 - h);
  double f_minus = ev.apply(expr);
  return (f_plus - f_minus) / (2.0 * h);
}

// Five-point central difference: 4th-order accurate.
// Used for stiff-but-smooth checks (e.g. inverse trig near the
// edge of their natural domain) where the 2-point estimate's O(h^2)
// truncation error eats into the tolerance budget.
inline double central_diff_scalar_5pt(scalar_expr const &expr,
                                      scalar_expr const &var, double x0,
                                      double h = 1e-3) {
  scalar_evaluator<double> ev;
  ev.set(var, x0 + 2 * h);
  double f_pp = ev.apply(expr);
  ev.set(var, x0 + h);
  double f_p = ev.apply(expr);
  ev.set(var, x0 - h);
  double f_m = ev.apply(expr);
  ev.set(var, x0 - 2 * h);
  double f_mm = ev.apply(expr);
  // (-f(x+2h) + 8 f(x+h) - 8 f(x-h) + f(x-2h)) / (12 h)
  return (-f_pp + 8.0 * f_p - 8.0 * f_m + f_mm) / (12.0 * h);
}

// AssertionResult-returning core: compares the symbolic derivative
// evaluated at `x0` against a central-difference estimate, mixing
// absolute and relative tolerance.
//
// Failure messages include both numerical values and the symbolic
// derivative's printed form so a regression points immediately at
// which differentiation rule is misbehaving.
inline ::testing::AssertionResult
check_symbolic_diff_matches(scalar_expr const &expr, scalar_expr const &var,
                            double x0, double abs_tol = 1e-6,
                            double rel_tol = 1e-4, double h = 1e-5) {
  auto d = diff(expr, var);
  if (!d.is_valid()) {
    return ::testing::AssertionFailure()
           << "diff(expr, var) returned an invalid expression";
  }
  double sym = eval_at(d, var, x0);
  double num = central_diff_scalar(expr, var, x0, h);
  if (!std::isfinite(sym) || !std::isfinite(num)) {
    return ::testing::AssertionFailure()
           << "non-finite value at " << var << " = " << x0
           << "\n  symbolic   = " << sym << "\n  numerical  = " << num;
  }
  double err = std::abs(sym - num);
  double mag = std::max(std::abs(sym), std::abs(num));
  if (err <= abs_tol || err <= rel_tol * mag) {
    return ::testing::AssertionSuccess();
  }
  std::ostringstream oss;
  oss << "symbolic vs numerical mismatch at " << var << " = " << x0
      << "\n  symbolic   = " << sym << "\n  numerical  = " << num
      << "\n  abs error  = " << err << "\n  rel error  = "
      << (mag > 0 ? err / mag : std::numeric_limits<double>::infinity())
      << "\n  symbolic d/dx = " << d;
  return ::testing::AssertionFailure() << oss.str();
}

// 5-point variant for stiff-but-smooth cases.
inline ::testing::AssertionResult
check_symbolic_diff_matches_5pt(scalar_expr const &expr, scalar_expr const &var,
                                double x0, double abs_tol = 1e-6,
                                double rel_tol = 1e-4, double h = 1e-3) {
  auto d = diff(expr, var);
  if (!d.is_valid()) {
    return ::testing::AssertionFailure()
           << "diff(expr, var) returned an invalid expression";
  }
  double sym = eval_at(d, var, x0);
  double num = central_diff_scalar_5pt(expr, var, x0, h);
  if (!std::isfinite(sym) || !std::isfinite(num)) {
    return ::testing::AssertionFailure()
           << "non-finite value at " << var << " = " << x0
           << "\n  symbolic   = " << sym << "\n  numerical  = " << num;
  }
  double err = std::abs(sym - num);
  double mag = std::max(std::abs(sym), std::abs(num));
  if (err <= abs_tol || err <= rel_tol * mag) {
    return ::testing::AssertionSuccess();
  }
  std::ostringstream oss;
  oss << "symbolic vs 5pt-numerical mismatch at " << var << " = " << x0
      << "\n  symbolic   = " << sym << "\n  numerical  = " << num
      << "\n  abs error  = " << err << "\n  rel error  = "
      << (mag > 0 ? err / mag : std::numeric_limits<double>::infinity())
      << "\n  symbolic d/dx = " << d;
  return ::testing::AssertionFailure() << oss.str();
}

} // namespace numsim::cas::numerical_diff_test

// EXPECT_DIFF_MATCHES(expr, var, x0)
//   Symbolic d(expr)/d(var) evaluated at x0 must match the central-
//   difference numerical estimate within (abs 1e-6, rel 1e-4).
#define EXPECT_DIFF_MATCHES(expr, var, x0)                                     \
  EXPECT_TRUE(::numsim::cas::numerical_diff_test::check_symbolic_diff_matches( \
      (expr), (var), (x0)))

// EXPECT_DIFF_MATCHES_5PT(expr, var, x0)
//   Same contract, 5-point central difference — for stiff-but-smooth
//   expressions where 2-point O(h^2) truncation error would eat the
//   tolerance budget (e.g. inverse trig near domain edges).
#define EXPECT_DIFF_MATCHES_5PT(expr, var, x0)                                 \
  EXPECT_TRUE(                                                                 \
      ::numsim::cas::numerical_diff_test::check_symbolic_diff_matches_5pt(     \
          (expr), (var), (x0)))

#endif // NUMSIM_CAS_TESTS_NUMERICAL_DIFF_HELPERS_H
