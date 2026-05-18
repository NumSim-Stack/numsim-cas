#ifndef SCALARCOMPARISONTEST_H
#define SCALARCOMPARISONTEST_H

// Lock-in tests for the six scalar comparison operators (#136).
//
// Comparisons in numsim::cas are *scalar-valued indicator* expressions:
// `lt(a, b)` evaluates to 1.0 when a < b and 0.0 otherwise. This is the
// pattern that makes constructs like `(a < b) * x` work directly as the
// damage-activation idiom in constitutive modelling and gives the
// upcoming `if_then_else` a uniform Real-typed condition.
//
// These tests pin the contract so the eventual logical-combinator and
// `if_then_else` PRs cannot silently widen the indicator's behaviour.

#include "cas_test_helpers.h"
#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"
#include <numsim_cas/core/contains_expression.h>
#include <numsim_cas/core/substitute.h>
#include <numsim_cas/scalar/visitors/scalar_substitution.h>

namespace numsim::cas {

namespace {
using expr_t = expression_holder<scalar_expression>;
} // namespace

// --- 1. Constant folding: both sides numeric ⇒ resolves at construction.

TEST(ScalarComparison, ConstantFolding_Lt) {
  EXPECT_TRUE(is_same<scalar_one>(
      lt(make_scalar_constant(1), make_scalar_constant(2))));
  EXPECT_TRUE(is_same<scalar_zero>(
      lt(make_scalar_constant(2), make_scalar_constant(1))));
  EXPECT_TRUE(is_same<scalar_zero>(
      lt(make_scalar_constant(1), make_scalar_constant(1))));
}

TEST(ScalarComparison, ConstantFolding_Gt) {
  EXPECT_TRUE(is_same<scalar_one>(
      gt(make_scalar_constant(2), make_scalar_constant(1))));
  EXPECT_TRUE(is_same<scalar_zero>(
      gt(make_scalar_constant(1), make_scalar_constant(2))));
  EXPECT_TRUE(is_same<scalar_zero>(
      gt(make_scalar_constant(1), make_scalar_constant(1))));
}

TEST(ScalarComparison, ConstantFolding_Le) {
  EXPECT_TRUE(is_same<scalar_one>(
      le(make_scalar_constant(1), make_scalar_constant(2))));
  EXPECT_TRUE(is_same<scalar_one>(
      le(make_scalar_constant(1), make_scalar_constant(1))));
  EXPECT_TRUE(is_same<scalar_zero>(
      le(make_scalar_constant(2), make_scalar_constant(1))));
}

TEST(ScalarComparison, ConstantFolding_Ge) {
  EXPECT_TRUE(is_same<scalar_one>(
      ge(make_scalar_constant(2), make_scalar_constant(1))));
  EXPECT_TRUE(is_same<scalar_one>(
      ge(make_scalar_constant(1), make_scalar_constant(1))));
  EXPECT_TRUE(is_same<scalar_zero>(
      ge(make_scalar_constant(1), make_scalar_constant(2))));
}

TEST(ScalarComparison, ConstantFolding_Eq) {
  EXPECT_TRUE(is_same<scalar_one>(
      eq(make_scalar_constant(3), make_scalar_constant(3))));
  EXPECT_TRUE(is_same<scalar_zero>(
      eq(make_scalar_constant(3), make_scalar_constant(4))));
}

TEST(ScalarComparison, ConstantFolding_Ne) {
  EXPECT_TRUE(is_same<scalar_one>(
      ne(make_scalar_constant(3), make_scalar_constant(4))));
  EXPECT_TRUE(is_same<scalar_zero>(
      ne(make_scalar_constant(3), make_scalar_constant(3))));
}

// --- 2. Structural identity: same expression on both sides folds.

TEST(ScalarComparison, IdentityFolding_EqLeGe_ToOne) {
  auto [x] = make_scalar_variable("x");
  EXPECT_TRUE(is_same<scalar_one>(eq(x, x)));
  EXPECT_TRUE(is_same<scalar_one>(le(x, x)));
  EXPECT_TRUE(is_same<scalar_one>(ge(x, x)));
}

TEST(ScalarComparison, IdentityFolding_LtGtNe_ToZero) {
  auto [x] = make_scalar_variable("x");
  EXPECT_TRUE(is_same<scalar_zero>(lt(x, x)));
  EXPECT_TRUE(is_same<scalar_zero>(gt(x, x)));
  EXPECT_TRUE(is_same<scalar_zero>(ne(x, x)));
}

// --- 3. Non-foldable: stays as the corresponding node type.

TEST(ScalarComparison, NoFold_Lt) {
  auto [x, y] = make_scalar_variable("x", "y");
  EXPECT_TRUE(is_same<scalar_lt>(lt(x, y)));
}
TEST(ScalarComparison, NoFold_Gt) {
  auto [x, y] = make_scalar_variable("x", "y");
  EXPECT_TRUE(is_same<scalar_gt>(gt(x, y)));
}
TEST(ScalarComparison, NoFold_Le) {
  auto [x, y] = make_scalar_variable("x", "y");
  EXPECT_TRUE(is_same<scalar_le>(le(x, y)));
}
TEST(ScalarComparison, NoFold_Ge) {
  auto [x, y] = make_scalar_variable("x", "y");
  EXPECT_TRUE(is_same<scalar_ge>(ge(x, y)));
}
TEST(ScalarComparison, NoFold_Eq) {
  auto [x, y] = make_scalar_variable("x", "y");
  EXPECT_TRUE(is_same<scalar_eq>(eq(x, y)));
}
TEST(ScalarComparison, NoFold_Ne) {
  auto [x, y] = make_scalar_variable("x", "y");
  EXPECT_TRUE(is_same<scalar_ne>(ne(x, y)));
}

// --- 4. Evaluator: indicator semantics — true ⇒ 1.0, false ⇒ 0.0.

TEST(ScalarComparison, EvaluatorIndicator_Lt) {
  scalar_evaluator<double> ev;
  auto [x, y] = make_scalar_variable("x", "y");
  ev.set(x, 1.0);
  ev.set(y, 2.0);
  EXPECT_NEAR(ev.apply(lt(x, y)), 1.0, 1e-12);
  ev.set(x, 3.0);
  EXPECT_NEAR(ev.apply(lt(x, y)), 0.0, 1e-12);
  ev.set(x, 2.0);
  EXPECT_NEAR(ev.apply(lt(x, y)), 0.0, 1e-12); // strict
}

TEST(ScalarComparison, EvaluatorIndicator_Gt) {
  scalar_evaluator<double> ev;
  auto [x, y] = make_scalar_variable("x", "y");
  ev.set(x, 3.0);
  ev.set(y, 2.0);
  EXPECT_NEAR(ev.apply(gt(x, y)), 1.0, 1e-12);
  ev.set(x, 1.0);
  EXPECT_NEAR(ev.apply(gt(x, y)), 0.0, 1e-12);
  ev.set(x, 2.0);
  EXPECT_NEAR(ev.apply(gt(x, y)), 0.0, 1e-12); // strict
}

TEST(ScalarComparison, EvaluatorIndicator_Le) {
  scalar_evaluator<double> ev;
  auto [x, y] = make_scalar_variable("x", "y");
  ev.set(x, 1.0);
  ev.set(y, 2.0);
  EXPECT_NEAR(ev.apply(le(x, y)), 1.0, 1e-12);
  ev.set(x, 2.0);
  EXPECT_NEAR(ev.apply(le(x, y)), 1.0, 1e-12); // inclusive
  ev.set(x, 3.0);
  EXPECT_NEAR(ev.apply(le(x, y)), 0.0, 1e-12);
}

TEST(ScalarComparison, EvaluatorIndicator_Ge) {
  scalar_evaluator<double> ev;
  auto [x, y] = make_scalar_variable("x", "y");
  ev.set(x, 3.0);
  ev.set(y, 2.0);
  EXPECT_NEAR(ev.apply(ge(x, y)), 1.0, 1e-12);
  ev.set(x, 2.0);
  EXPECT_NEAR(ev.apply(ge(x, y)), 1.0, 1e-12); // inclusive
  ev.set(x, 1.0);
  EXPECT_NEAR(ev.apply(ge(x, y)), 0.0, 1e-12);
}

TEST(ScalarComparison, EvaluatorIndicator_Eq) {
  scalar_evaluator<double> ev;
  auto [x, y] = make_scalar_variable("x", "y");
  ev.set(x, 2.0);
  ev.set(y, 2.0);
  EXPECT_NEAR(ev.apply(eq(x, y)), 1.0, 1e-12);
  ev.set(x, 3.0);
  EXPECT_NEAR(ev.apply(eq(x, y)), 0.0, 1e-12);
}

TEST(ScalarComparison, EvaluatorIndicator_Ne) {
  scalar_evaluator<double> ev;
  auto [x, y] = make_scalar_variable("x", "y");
  ev.set(x, 3.0);
  ev.set(y, 2.0);
  EXPECT_NEAR(ev.apply(ne(x, y)), 1.0, 1e-12);
  ev.set(x, 2.0);
  EXPECT_NEAR(ev.apply(ne(x, y)), 0.0, 1e-12);
}

// --- 5. The damage-activation idiom: indicator * payload.

TEST(ScalarComparison, IndicatorProductActivates) {
  scalar_evaluator<double> ev;
  auto [eps, eps0, sigma] = make_scalar_variable("eps", "eps0", "sigma");
  auto activated = gt(eps, eps0) * sigma; // = 0 below threshold, σ above
  ev.set(eps0, 1.0);
  ev.set(sigma, 100.0);

  ev.set(eps, 0.5);
  EXPECT_NEAR(ev.apply(activated), 0.0, 1e-12);
  ev.set(eps, 2.0);
  EXPECT_NEAR(ev.apply(activated), 100.0, 1e-12);
}

// --- 6. Differentiation: indicator's derivative is zero (sub-gradient).

TEST(ScalarComparison, DerivativeIsZero) {
  auto [x, y] = make_scalar_variable("x", "y");
  EXPECT_TRUE(is_same<scalar_zero>(diff(lt(x, y), x)));
  EXPECT_TRUE(is_same<scalar_zero>(diff(gt(x, y), x)));
  EXPECT_TRUE(is_same<scalar_zero>(diff(le(x, y), x)));
  EXPECT_TRUE(is_same<scalar_zero>(diff(ge(x, y), x)));
  EXPECT_TRUE(is_same<scalar_zero>(diff(eq(x, y), x)));
  EXPECT_TRUE(is_same<scalar_zero>(diff(ne(x, y), x)));
}

// --- 7. Printer: infix with parentheses (lossless re-parsing aid).

TEST(ScalarComparison, PrinterEmitsInfix) {
  auto [x, y] = make_scalar_variable("x", "y");
  std::stringstream s_lt;
  s_lt << lt(x, y);
  EXPECT_EQ(s_lt.str(), "(x < y)");
  std::stringstream s_gt;
  s_gt << gt(x, y);
  EXPECT_EQ(s_gt.str(), "(x > y)");
  std::stringstream s_le;
  s_le << le(x, y);
  EXPECT_EQ(s_le.str(), "(x <= y)");
  std::stringstream s_ge;
  s_ge << ge(x, y);
  EXPECT_EQ(s_ge.str(), "(x >= y)");
  std::stringstream s_eq;
  s_eq << eq(x, y);
  EXPECT_EQ(s_eq.str(), "(x == y)");
  std::stringstream s_ne;
  s_ne << ne(x, y);
  EXPECT_EQ(s_ne.str(), "(x != y)");
}

// --- 8. Substitution recurses through both operands.

TEST(ScalarComparison, SubstitutesIntoBothSides) {
  auto [x, y, z] = make_scalar_variable("x", "y", "z");
  auto expr = lt(x, y);
  auto sub_lhs = substitute(expr, x, z);
  EXPECT_EQ(sub_lhs, lt(z, y));
  auto sub_rhs = substitute(expr, y, z);
  EXPECT_EQ(sub_rhs, lt(x, z));
}

// --- 9. `contains_expression` finds operands.

TEST(ScalarComparison, ContainsFindsOperands) {
  auto [x, y, z] = make_scalar_variable("x", "y", "z");
  EXPECT_TRUE(contains_expression(lt(x, y), x));
  EXPECT_TRUE(contains_expression(lt(x, y), y));
  EXPECT_FALSE(contains_expression(lt(x, y), z));
}

} // namespace numsim::cas

#endif // SCALARCOMPARISONTEST_H
