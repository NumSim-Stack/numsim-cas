#ifndef SCALARASSUMPTIONTEST_H
#define SCALARASSUMPTIONTEST_H

#include "cas_test_helpers.h"
#include "numsim_cas/numsim_cas.h"
#include "numsim_cas/scalar/scalar_assume.h"
#include "numsim_cas/scalar/visitors/scalar_assumption_propagator.h"
#include "gtest/gtest.h"

#include <cmath>

// ─── Fixture ─────────────────────────────────────────────────────────

struct AssumptionFixture : ::testing::Test {
  using scalar_expr =
      numsim::cas::expression_holder<numsim::cas::scalar_expression>;

  scalar_expr x, y, z;

  AssumptionFixture() {
    std::tie(x, y, z) = numsim::cas::make_scalar_variable("x", "y", "z");
  }
};

using std::abs;
using std::exp;
using std::pow;
using std::sqrt;

// ═══════════════════════════════════════════════════════════════════════
//  Step 1: API tests
// ═══════════════════════════════════════════════════════════════════════

TEST_F(AssumptionFixture, AssumePositiveSetsImplied) {
  numsim::cas::assume(x, numsim::cas::positive{});
  EXPECT_TRUE(numsim::cas::is_positive(x));
  EXPECT_TRUE(numsim::cas::is_nonnegative(x));
  EXPECT_TRUE(numsim::cas::is_nonzero(x));
  EXPECT_TRUE(numsim::cas::is_real(x));
  EXPECT_FALSE(numsim::cas::is_negative(x));
}

TEST_F(AssumptionFixture, AssumeNegativeSetsImplied) {
  numsim::cas::assume(x, numsim::cas::negative{});
  EXPECT_TRUE(numsim::cas::is_negative(x));
  EXPECT_TRUE(numsim::cas::is_nonpositive(x));
  EXPECT_TRUE(numsim::cas::is_nonzero(x));
  EXPECT_TRUE(numsim::cas::is_real(x));
  EXPECT_FALSE(numsim::cas::is_positive(x));
}

TEST_F(AssumptionFixture, RemoveAssumptionRemovesOnly) {
  numsim::cas::assume(x, numsim::cas::positive{});
  EXPECT_TRUE(numsim::cas::is_positive(x));

  numsim::cas::remove_assumption(x, numsim::cas::positive{});
  EXPECT_FALSE(numsim::cas::is_positive(x));
  // implied assumptions remain
  EXPECT_TRUE(numsim::cas::is_nonnegative(x));
  EXPECT_TRUE(numsim::cas::is_nonzero(x));
}

TEST_F(AssumptionFixture, QueryHelpers) {
  numsim::cas::assume(x, numsim::cas::even{});
  EXPECT_TRUE(numsim::cas::is_even(x));
  EXPECT_TRUE(numsim::cas::is_integer_assumed(x));
  EXPECT_TRUE(numsim::cas::is_real(x));
}

// ═══════════════════════════════════════════════════════════════════════
//  Step 2: Propagation tests
// ═══════════════════════════════════════════════════════════════════════

TEST_F(AssumptionFixture, PropagateExp) {
  auto e = exp(x);
  auto a = numsim::cas::propagate_assumptions(e);
  EXPECT_TRUE(a.contains(numsim::cas::positive{}));
  EXPECT_TRUE(a.contains(numsim::cas::nonzero{}));
  EXPECT_TRUE(a.contains(numsim::cas::real_tag{}));
}

TEST_F(AssumptionFixture, PropagateAbs) {
  auto e = abs(x);
  auto a = numsim::cas::propagate_assumptions(e);
  EXPECT_TRUE(a.contains(numsim::cas::nonnegative{}));
  EXPECT_TRUE(a.contains(numsim::cas::real_tag{}));
}

TEST_F(AssumptionFixture, PropagateAbsNonzero) {
  numsim::cas::assume(x, numsim::cas::nonzero{});
  auto e = abs(x);
  auto a = numsim::cas::propagate_assumptions(e);
  EXPECT_TRUE(a.contains(numsim::cas::positive{}));
}

TEST_F(AssumptionFixture, PropagateAddBothPositive) {
  numsim::cas::assume(x, numsim::cas::positive{});
  numsim::cas::assume(y, numsim::cas::positive{});
  auto e = x + y;
  auto a = numsim::cas::propagate_assumptions(e);
  EXPECT_TRUE(a.contains(numsim::cas::positive{}));
}

TEST_F(AssumptionFixture, PropagateMulPosPos) {
  numsim::cas::assume(x, numsim::cas::positive{});
  numsim::cas::assume(y, numsim::cas::positive{});
  auto e = x * y;
  auto a = numsim::cas::propagate_assumptions(e);
  EXPECT_TRUE(a.contains(numsim::cas::positive{}));
}

TEST_F(AssumptionFixture, PropagateMulPosNeg) {
  numsim::cas::assume(x, numsim::cas::positive{});
  numsim::cas::assume(y, numsim::cas::negative{});
  auto e = x * y;
  auto a = numsim::cas::propagate_assumptions(e);
  EXPECT_TRUE(a.contains(numsim::cas::negative{}));
}

TEST_F(AssumptionFixture, PropagateNegPositive) {
  numsim::cas::assume(x, numsim::cas::positive{});
  auto e = -x;
  auto a = numsim::cas::propagate_assumptions(e);
  EXPECT_TRUE(a.contains(numsim::cas::negative{}));
}

TEST_F(AssumptionFixture, PropagateConstant) {
  auto c5 = numsim::cas::make_expression<numsim::cas::scalar_constant>(5);
  auto a = numsim::cas::propagate_assumptions(c5);
  EXPECT_TRUE(a.contains(numsim::cas::positive{}));
  EXPECT_TRUE(a.contains(numsim::cas::integer{}));
  EXPECT_TRUE(a.contains(numsim::cas::real_tag{}));
}

TEST_F(AssumptionFixture, PropagatePowEvenExp) {
  auto e = pow(x, 2);
  auto a = numsim::cas::propagate_assumptions(e);
  EXPECT_TRUE(a.contains(numsim::cas::nonnegative{}));
}

TEST_F(AssumptionFixture, PropagateSqrt) {
  auto e = sqrt(x);
  auto a = numsim::cas::propagate_assumptions(e);
  EXPECT_TRUE(a.contains(numsim::cas::nonnegative{}));
}

// ═══════════════════════════════════════════════════════════════════════
//  Step 3: Construction-time simplification tests
// ═══════════════════════════════════════════════════════════════════════

TEST_F(AssumptionFixture, AbsPositiveReturnsExpr) {
  numsim::cas::assume(x, numsim::cas::positive{});
  auto e = abs(x);
  // Should NOT be a scalar_abs node
  EXPECT_FALSE(numsim::cas::is_same<numsim::cas::scalar_abs>(e));
  EXPECT_PRINT(e, "x");
}

TEST_F(AssumptionFixture, AbsNegativeReturnsNegExpr) {
  numsim::cas::assume(x, numsim::cas::negative{});
  auto e = abs(x);
  // Should be a scalar_negative node wrapping x
  EXPECT_TRUE(numsim::cas::is_same<numsim::cas::scalar_negative>(e));
  EXPECT_PRINT(e, "-x");
}

TEST_F(AssumptionFixture, SignPositiveReturnsOne) {
  numsim::cas::assume(x, numsim::cas::positive{});
  auto e = numsim::cas::sign(x);
  EXPECT_TRUE(numsim::cas::is_same<numsim::cas::scalar_one>(e));
}

TEST_F(AssumptionFixture, SignNegativeReturnsNegOne) {
  numsim::cas::assume(x, numsim::cas::negative{});
  auto e = numsim::cas::sign(x);
  EXPECT_PRINT(e, "-1");
}

TEST_F(AssumptionFixture, SqrtPow2NonnegReturnsBase) {
  numsim::cas::assume(x, numsim::cas::nonnegative{});
  auto e = sqrt(pow(x, 2));
  // sqrt(x^2) with x>=0 should give x directly
  EXPECT_PRINT(e, "x");
}

// ═══════════════════════════════════════════════════════════════════════
//  Step 4: Automatic inference at binary simplifier level
//  (no manual propagate_assumptions call needed)
// ═══════════════════════════════════════════════════════════════════════

TEST_F(AssumptionFixture, InferAddPositive) {
  numsim::cas::assume(x, numsim::cas::positive{});
  numsim::cas::assume(y, numsim::cas::positive{});
  auto e = x + y;
  // Assumptions inferred automatically at construction time
  EXPECT_TRUE(numsim::cas::is_positive(e));
  EXPECT_TRUE(numsim::cas::is_nonnegative(e));
}

TEST_F(AssumptionFixture, InferMulNegative) {
  numsim::cas::assume(x, numsim::cas::positive{});
  numsim::cas::assume(y, numsim::cas::negative{});
  auto e = x * y;
  EXPECT_TRUE(numsim::cas::is_negative(e));
}

TEST_F(AssumptionFixture, InferSubPositive) {
  numsim::cas::assume(x, numsim::cas::positive{});
  numsim::cas::assume(y, numsim::cas::negative{});
  // positive - negative = positive + positive
  auto e = x - y;
  EXPECT_TRUE(numsim::cas::is_positive(e));
}

TEST_F(AssumptionFixture, InferNegFlipsSign) {
  numsim::cas::assume(x, numsim::cas::positive{});
  auto e = -x;
  EXPECT_TRUE(numsim::cas::is_negative(e));
  EXPECT_TRUE(numsim::cas::is_nonpositive(e));
}

TEST_F(AssumptionFixture, InferPowEvenNonneg) {
  auto e = pow(x, 2);
  EXPECT_TRUE(numsim::cas::is_nonnegative(e));
}

TEST_F(AssumptionFixture, InferExpPositive) {
  auto e = exp(x);
  EXPECT_TRUE(numsim::cas::is_positive(e));
  EXPECT_TRUE(numsim::cas::is_nonzero(e));
}

TEST_F(AssumptionFixture, InferSqrtNonneg) {
  auto e = sqrt(x);
  EXPECT_TRUE(numsim::cas::is_nonnegative(e));
}

TEST_F(AssumptionFixture, InferChainedOps) {
  numsim::cas::assume(x, numsim::cas::positive{});
  numsim::cas::assume(y, numsim::cas::positive{});
  // exp(x) is positive, x+y is positive, their product should be positive
  auto e = exp(x) * (x + y);
  EXPECT_TRUE(numsim::cas::is_positive(e));
}

// ─── max / min sign-monotonicity (#207 review) ───────────────────────
//
// max(a, b) ≥ both, so any positive lower bound propagates upward.
// min(a, b) ≤ both, so any negative upper bound propagates downward.
// Lock in the inference for each predicate so the Macauley-bracket
// downstream (#138) can rely on `max(x, 0)` being known nonnegative
// without the user having to assume() it manually.

TEST_F(AssumptionFixture, InferMaxNonnegativeWhenEitherOperandNonneg) {
  numsim::cas::assume(x, numsim::cas::nonnegative{});
  auto e = numsim::cas::max(x, y); // y unknown but max ≥ x ≥ 0
  EXPECT_TRUE(numsim::cas::is_nonnegative(e));
}

TEST_F(AssumptionFixture, InferMaxPositiveWhenEitherOperandPositive) {
  numsim::cas::assume(x, numsim::cas::positive{});
  auto e = numsim::cas::max(x, y);
  EXPECT_TRUE(numsim::cas::is_positive(e));
  EXPECT_TRUE(numsim::cas::is_nonzero(e));
}

TEST_F(AssumptionFixture, InferMaxNegativeWhenBothNegative) {
  numsim::cas::assume(x, numsim::cas::negative{});
  numsim::cas::assume(y, numsim::cas::negative{});
  auto e = numsim::cas::max(x, y);
  EXPECT_TRUE(numsim::cas::is_negative(e));
}

TEST_F(AssumptionFixture, InferMinNonpositiveWhenEitherOperandNonpos) {
  numsim::cas::assume(x, numsim::cas::nonpositive{});
  auto e = numsim::cas::min(x, y);
  EXPECT_TRUE(numsim::cas::is_nonpositive(e));
}

TEST_F(AssumptionFixture, InferMinNegativeWhenEitherOperandNegative) {
  numsim::cas::assume(x, numsim::cas::negative{});
  auto e = numsim::cas::min(x, y);
  EXPECT_TRUE(numsim::cas::is_negative(e));
  EXPECT_TRUE(numsim::cas::is_nonzero(e));
}

TEST_F(AssumptionFixture, InferMinPositiveWhenBothPositive) {
  numsim::cas::assume(x, numsim::cas::positive{});
  numsim::cas::assume(y, numsim::cas::positive{});
  auto e = numsim::cas::min(x, y);
  EXPECT_TRUE(numsim::cas::is_positive(e));
}

TEST_F(AssumptionFixture, InferMaxOfXAndZeroIsNonnegative) {
  // The canonical Macauley positive part: <x>+ = max(x, 0).
  // Must be inferable as nonnegative without user-side `assume`.
  auto zero = numsim::cas::get_scalar_zero();
  auto e = numsim::cas::max(x, zero);
  EXPECT_TRUE(numsim::cas::is_nonnegative(e));
}

TEST_F(AssumptionFixture, InferMinOfXAndZeroIsNonpositive) {
  // Counterpart to the macauley negative part.
  auto zero = numsim::cas::get_scalar_zero();
  auto e = numsim::cas::min(x, zero);
  EXPECT_TRUE(numsim::cas::is_nonpositive(e));
}

// ─── Macauley brackets sign-propagation (#208 review) ────────────────
//
// macauley_plus(x) = max(x, 0)   should infer nonnegative.
// macauley_minus(x) = -min(x, 0) should infer nonnegative too (because
// min(x, 0) is nonpositive, and -nonpositive = nonnegative). These
// tests cross-validate that the #207 sign-monotonicity fix flows all
// the way through the constitutive-modelling primitives — the user
// shouldn't need to assume() these manually.

TEST_F(AssumptionFixture, InferMacauleyPlusIsNonnegative) {
  auto e = numsim::cas::macauley_plus(x);
  EXPECT_TRUE(numsim::cas::is_nonnegative(e));
}

TEST_F(AssumptionFixture, InferMacauleyMinusIsNonnegative) {
  // <x>- = -min(x, 0). min(x, 0) is nonpositive (one operand is 0),
  // so the negation is nonnegative.
  auto e = numsim::cas::macauley_minus(x);
  EXPECT_TRUE(numsim::cas::is_nonnegative(e));
}

#endif // SCALARASSUMPTIONTEST_H
