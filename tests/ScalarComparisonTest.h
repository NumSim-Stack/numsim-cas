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

// --- 1b. Cross-rank numeric folding: int vs double, rationals, etc.
// Locks in the use of `numeric_less` (which promotes ranks) rather
// than `scalar_number::operator<` (which is rank-lexicographic, a
// total order for sorting that gives wrong numeric answers).

TEST(ScalarComparison, ConstantFolding_CrossRank_IntVsDouble) {
  // int(3) and double(3.0) are numerically equal — not <, not >.
  EXPECT_TRUE(is_same<scalar_zero>(
      lt(make_scalar_constant(3), make_scalar_constant(3.0))));
  EXPECT_TRUE(is_same<scalar_zero>(
      gt(make_scalar_constant(3), make_scalar_constant(3.0))));
  EXPECT_TRUE(is_same<scalar_one>(
      eq(make_scalar_constant(3), make_scalar_constant(3.0))));
  EXPECT_TRUE(is_same<scalar_one>(
      le(make_scalar_constant(3), make_scalar_constant(3.0))));
  EXPECT_TRUE(is_same<scalar_one>(
      ge(make_scalar_constant(3), make_scalar_constant(3.0))));
  // int(2) < double(2.5) numerically.
  EXPECT_TRUE(is_same<scalar_one>(
      lt(make_scalar_constant(2), make_scalar_constant(2.5))));
}

TEST(ScalarComparison, ConstantFolding_Rationals) {
  auto half = make_scalar_constant(rational_t{1, 2});
  auto third = make_scalar_constant(rational_t{1, 3});
  EXPECT_TRUE(is_same<scalar_one>(gt(half, third)));
  EXPECT_TRUE(is_same<scalar_zero>(lt(half, third)));
  // rational vs int.
  EXPECT_TRUE(is_same<scalar_zero>(lt(half, make_scalar_constant(0))));
  EXPECT_TRUE(is_same<scalar_one>(gt(half, make_scalar_constant(0))));
}

// Locks in #143: NaN handling. The structural-identity fold treats
// hash-equal nodes as equal, so two scalar_constant(NaN) compare as
// equal — folding `eq(NaN, NaN)` to 1, `ne(NaN, NaN)` to 0,
// `lt(NaN, NaN)` to 0. IEEE-754 says NaN compares not-equal to
// anything including itself, so this is a deliberate-but-non-IEEE
// contract. Tests pin the current behaviour; tightening would be
// an API change.
TEST(ScalarComparison, EdgeCase_NaN_StructuralIdentity) {
  auto nan_const =
      make_scalar_constant(std::numeric_limits<double>::quiet_NaN());
  // Both eq/le/ge fold to 1 via identity (lhs == rhs structurally).
  EXPECT_TRUE(is_same<scalar_one>(eq(nan_const, nan_const)));
  EXPECT_TRUE(is_same<scalar_one>(le(nan_const, nan_const)));
  EXPECT_TRUE(is_same<scalar_one>(ge(nan_const, nan_const)));
  // lt/gt/ne fold to 0.
  EXPECT_TRUE(is_same<scalar_zero>(lt(nan_const, nan_const)));
  EXPECT_TRUE(is_same<scalar_zero>(gt(nan_const, nan_const)));
  EXPECT_TRUE(is_same<scalar_zero>(ne(nan_const, nan_const)));
}

// Locks in #144: complex ordering. `numeric_less` uses real-then-imag
// tiebreak for complex values — a total order needed by sort
// containers, but not a mathematically meaningful ordering. Documents
// the choice so any future change is deliberate.
TEST(ScalarComparison, EdgeCase_Complex_RealThenImagOrdering) {
  using namespace std::complex_literals;
  auto c_1_5 =
      make_scalar_constant(scalar_number{std::complex<double>{1.0, 5.0}});
  auto c_1_3 =
      make_scalar_constant(scalar_number{std::complex<double>{1.0, 3.0}});
  auto c_2_0 =
      make_scalar_constant(scalar_number{std::complex<double>{2.0, 0.0}});
  // Same real component → imag breaks the tie: 5 > 3 so c_1_5 > c_1_3
  EXPECT_TRUE(is_same<scalar_one>(gt(c_1_5, c_1_3)));
  EXPECT_TRUE(is_same<scalar_zero>(lt(c_1_5, c_1_3)));
  // Different real components → real wins regardless of imag: 1 < 2
  EXPECT_TRUE(is_same<scalar_one>(lt(c_1_5, c_2_0)));
}

// Locks in #142: rational cross-multiplication must not overflow.
// `10^18 / 3` and `10^18 / 2` have numerator * denominator products
// around 2*10^18 and 3*10^18 — both well past INT64_MAX (~9.2*10^18
// fits, but with such large numerators we'd hit overflow at the
// next size up). Without the __int128 / long-double-fallback path
// in `rat_less`, naive `num * den` wraps to negative and the
// comparison flips. Mathematically `10^18/3 < 10^18/2`.
TEST(ScalarComparison, ConstantFolding_Rationals_LargeNumerators) {
  constexpr std::int64_t big = 1'000'000'000'000'000'000LL; // 10^18
  auto big_over_3 = make_scalar_constant(rational_t{big, 3});
  auto big_over_2 = make_scalar_constant(rational_t{big, 2});
  // big/3 ≈ 3.33e17 < big/2 = 5e17
  EXPECT_TRUE(is_same<scalar_one>(lt(big_over_3, big_over_2)));
  EXPECT_TRUE(is_same<scalar_zero>(gt(big_over_3, big_over_2)));
  // Symmetric: bigger denominator → smaller value
  EXPECT_TRUE(is_same<scalar_one>(gt(big_over_2, big_over_3)));
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

// --- 3b. Assumption-based folding (uses is_positive/is_negative/...).

TEST(ScalarComparison, AssumptionFold_AbsAgainstZero) {
  auto [x] = make_scalar_variable("x");
  // |x| is nonnegative, so |x| < 0 must be false …
  EXPECT_TRUE(is_same<scalar_zero>(lt(abs(x), make_scalar_constant(0))));
  // … and 0 <= |x| must be true.
  EXPECT_TRUE(is_same<scalar_one>(le(make_scalar_constant(0), abs(x))));
  // |x| > -1 must be true (|x| nonneg, -1 negative).
  auto neg_one = -make_scalar_constant(1);
  EXPECT_TRUE(is_same<scalar_one>(gt(abs(x), neg_one)));
}

TEST(ScalarComparison, AssumptionFold_SqrtAgainstNegative) {
  auto [x] = make_scalar_variable("x");
  // sqrt(x) is nonnegative, -1 is negative ⇒ sqrt(x) >= -1 is true.
  auto neg_one = -make_scalar_constant(1);
  EXPECT_TRUE(is_same<scalar_one>(ge(sqrt(x), neg_one)));
  // -1 != sqrt(x) is true because their sign cones don't overlap.
  EXPECT_TRUE(is_same<scalar_one>(ne(neg_one, sqrt(x))));
}

TEST(ScalarComparison, AssumptionFold_ExpIsStrictlyPositive) {
  // exp(x) > 0 for all real x ⇒ lt(exp(x), 0) → 0, le(exp(x), 0) → 0,
  // gt(exp(x), 0) → 1, ne(exp(x), 0) → 1. Locks in the *strict* positive
  // cone, which is different from the nonneg cone covered by abs/sqrt.
  auto [x] = make_scalar_variable("x");
  auto zero = make_scalar_constant(0);
  EXPECT_TRUE(is_same<scalar_zero>(lt(exp(x), zero)));
  EXPECT_TRUE(is_same<scalar_zero>(le(exp(x), zero)));
  EXPECT_TRUE(is_same<scalar_one>(gt(exp(x), zero)));
  EXPECT_TRUE(is_same<scalar_one>(ne(exp(x), zero)));
}

// --- 3c. Cross-rank equality and structural commutativity.

TEST(ScalarComparison, ConstantFolding_CrossRank_Equal) {
  // int(1) == double(1.0) numerically ⇒ eq folds to 1, ne to 0,
  // le/ge to 1, lt/gt to 0. Inverse of the cross-rank lt test above.
  auto i1 = make_scalar_constant(1);
  auto d1 = make_scalar_constant(1.0);
  EXPECT_TRUE(is_same<scalar_one>(eq(i1, d1)));
  EXPECT_TRUE(is_same<scalar_zero>(ne(i1, d1)));
  EXPECT_TRUE(is_same<scalar_one>(le(i1, d1)));
  EXPECT_TRUE(is_same<scalar_one>(ge(i1, d1)));
  EXPECT_TRUE(is_same<scalar_zero>(lt(i1, d1)));
  EXPECT_TRUE(is_same<scalar_zero>(gt(i1, d1)));
}

TEST(ScalarComparison, IdentityFolding_CommutativeMul) {
  // The n_ary_tree stores mul children unordered (hash-keyed), so x*y
  // and y*x are the same expression_holder structurally. The identity
  // fold should fire on rearranged-but-equivalent expressions.
  auto [x, y] = make_scalar_variable("x", "y");
  EXPECT_EQ(x * y, y * x);
  EXPECT_TRUE(is_same<scalar_one>(eq(x * y, y * x)));
  EXPECT_TRUE(is_same<scalar_zero>(ne(x * y, y * x)));
  EXPECT_TRUE(is_same<scalar_one>(le(x * y, y * x)));
  EXPECT_TRUE(is_same<scalar_one>(ge(x * y, y * x)));
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

// --- 6b. Indicator survives composition: printer + evaluator on a
//        compound expression that mixes a comparison with arithmetic.

TEST(ScalarComparison, IndicatorInsideExpression_PrintAndEval) {
  auto [x, y] = make_scalar_variable("x", "y");
  auto expr = lt(x, y) * make_scalar_constant(2);
  // The printer wraps the comparison in parens so the product reads
  // unambiguously regardless of operator precedence in the host
  // language.
  std::stringstream s;
  s << expr;
  EXPECT_EQ(s.str(), "2*(x < y)");

  scalar_evaluator<double> ev;
  ev.set(x, 1.0);
  ev.set(y, 2.0);
  EXPECT_NEAR(ev.apply(expr), 2.0, 1e-12);
  ev.set(x, 3.0);
  EXPECT_NEAR(ev.apply(expr), 0.0, 1e-12);
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
