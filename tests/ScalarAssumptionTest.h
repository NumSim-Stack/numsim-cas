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
  EXPECT_TRUE(numsim::cas::is_integer(x));
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

// ─── if_then_else branch-intersection assumptions (#209 review) ──────
//
// if_then_else(cond, A, B) returns either A or B, so any assumption
// tag they BOTH carry holds for the result. Concretely: both branches
// nonnegative ⇒ result nonnegative; both real ⇒ real; etc.

TEST_F(AssumptionFixture, InferIfThenElseBothNonnegResultNonneg) {
  numsim::cas::assume(x, numsim::cas::nonnegative{});
  numsim::cas::assume(y, numsim::cas::nonnegative{});
  auto e = numsim::cas::if_then_else(z, x, y);
  EXPECT_TRUE(numsim::cas::is_nonnegative(e));
}

TEST_F(AssumptionFixture, InferIfThenElseBothPositiveResultPositive) {
  numsim::cas::assume(x, numsim::cas::positive{});
  numsim::cas::assume(y, numsim::cas::positive{});
  auto e = numsim::cas::if_then_else(z, x, y);
  EXPECT_TRUE(numsim::cas::is_positive(e));
  EXPECT_TRUE(numsim::cas::is_nonzero(e));
}

TEST_F(AssumptionFixture, InferIfThenElseBothNegativeResultNegative) {
  numsim::cas::assume(x, numsim::cas::negative{});
  numsim::cas::assume(y, numsim::cas::negative{});
  auto e = numsim::cas::if_then_else(z, x, y);
  EXPECT_TRUE(numsim::cas::is_negative(e));
}

TEST_F(AssumptionFixture, InferIfThenElseMixedBranchesDropsSign) {
  // One positive, one negative — intersection is empty.
  numsim::cas::assume(x, numsim::cas::positive{});
  numsim::cas::assume(y, numsim::cas::negative{});
  auto e = numsim::cas::if_then_else(z, x, y);
  EXPECT_FALSE(numsim::cas::is_positive(e));
  EXPECT_FALSE(numsim::cas::is_negative(e));
}

TEST_F(AssumptionFixture, InferMaxDiffResultIsNonnegative) {
  // d/dx max(a, b) emits if_then_else(gt(a, b), 1, 0). Both branches
  // (scalar_one, scalar_zero) are nonnegative, so the diff result
  // should be inferable as nonnegative. Cross-validates that #209's
  // intersection rule combines with #207's max-diff path.
  auto e = numsim::cas::diff(numsim::cas::max(x, y), x);
  EXPECT_TRUE(numsim::cas::is_nonnegative(e));
}

// ═══════════════════════════════════════════════════════════════════════
//  Foundational lock-ins: base-ctor m_assumption propagation
// ═══════════════════════════════════════════════════════════════════════
//
// The expression base copy/move ctors propagate m_assumption (the numeric
// assumption manager). Without this, deep-copy of any leaf node silently
// drops user assertions. These tests pin the invariant — a future refactor
// that reverts the propagation would be caught here rather than via
// downstream behavioral regressions.
//
// scalar's copy ctor is `= delete`, but its move ctor chains through
// symbol_base → expression, so move-construction exercises the propagation
// path. Tensor moves were broken pre-fix (chained through by-name ctor
// instead of base move) — the test would have caught that.

TEST_F(AssumptionFixture, ScalarSymbolMovePreservesAssumptions) {
  // Build a scalar leaf node directly, assert on it via the manager
  // API (assume_* helpers go through the propagator and are tested above;
  // here we want the bare m_assumption mechanics), move-construct a new
  // leaf from it, and confirm the assertion survives.
  numsim::cas::scalar src{"x_src"};
  src.assumptions().insert(numsim::cas::positive{});
  ASSERT_TRUE(src.assumptions().contains(numsim::cas::positive{}));

  numsim::cas::scalar moved{std::move(src)};
  EXPECT_TRUE(moved.assumptions().contains(numsim::cas::positive{}))
      << "scalar move ctor must propagate m_assumption via the "
         "symbol_base → expression chain";
}

TEST_F(AssumptionFixture, TensorSymbolMovePreservesAssumptions) {
  // Mirror for tensor — this is the bug the β-4 review surfaced. Before
  // the fix, tensor's move ctor routed `base(data.m_name, data.m_dim,
  // data.m_rank)` which goes through symbol_base's by-name ctor and
  // never invokes expression's move ctor, silently dropping m_assumption.
  numsim::cas::tensor src{"X_src", std::size_t{3}, std::size_t{2}};
  src.assumptions().insert(numsim::cas::positive{});
  ASSERT_TRUE(src.assumptions().contains(numsim::cas::positive{}));

  numsim::cas::tensor moved{std::move(src)};
  EXPECT_TRUE(moved.assumptions().contains(numsim::cas::positive{}))
      << "tensor move ctor must propagate m_assumption via the "
         "symbol_base → expression chain (regression test for the bug "
         "found in the step-1 critical review)";
}

// ─── Step 4: scalar assume() throws on non-Symbols ────────────────────
// Symmetric to the tensor side. Compounds like x + y can't carry
// user-asserted facts — the user must assert on the leaves and let the
// propagator derive facts for the compound.

TEST_F(AssumptionFixture, AssumePositiveOnCompoundThrows) {
  EXPECT_THROW(numsim::cas::assume(x + y, numsim::cas::positive{}),
               numsim::cas::invalid_assumption_error);
}

TEST_F(AssumptionFixture, AssumeNegativeOnCompoundThrows) {
  EXPECT_THROW(numsim::cas::assume(x * y, numsim::cas::negative{}),
               numsim::cas::invalid_assumption_error);
}

TEST_F(AssumptionFixture, AssumeNonzeroOnCompoundThrows) {
  EXPECT_THROW(numsim::cas::assume(x - y, numsim::cas::nonzero{}),
               numsim::cas::invalid_assumption_error);
}

TEST_F(AssumptionFixture, AssumePositiveOnSymbolSucceeds) {
  // Positive case: same call on a Symbol works. Guards against
  // over-aggressive guards rejecting valid usage.
  EXPECT_NO_THROW(numsim::cas::assume(x, numsim::cas::positive{}));
  EXPECT_TRUE(numsim::cas::is_positive(x));
}

TEST_F(AssumptionFixture, AssumeEvenOnCompoundThrows) {
  // QA Q2: assume(even{}) inserts both even AND integer — non-trivial
  // implication chain. Pin the throw separately from sign predicates.
  EXPECT_THROW(numsim::cas::assume(x + y, numsim::cas::even{}),
               numsim::cas::invalid_assumption_error);
}

// ─── Step 4: scalar_constant self-annotates from value ────────────────
// Direct unit lock-ins for the value-derived assumption logic. Without
// these, the only coverage is indirect via abs/sign simplifications.

TEST(ScalarConstantValueAssumptions, PositiveIntegerCarriesIntegerAndPositive) {
  auto c = numsim::cas::make_expression<numsim::cas::scalar_constant>(
      std::int64_t{5});
  EXPECT_TRUE(numsim::cas::is_positive(c));
  EXPECT_TRUE(numsim::cas::is_nonnegative(c));
  EXPECT_TRUE(numsim::cas::is_nonzero(c));
  EXPECT_TRUE(numsim::cas::is_integer(c));
  EXPECT_TRUE(numsim::cas::is_real(c));
  EXPECT_FALSE(numsim::cas::is_negative(c));
}

TEST(ScalarConstantValueAssumptions, NegativeIntegerCarriesNegative) {
  // QA Q3a: no existing test exercised the negative-int branch.
  auto c = numsim::cas::make_expression<numsim::cas::scalar_constant>(
      std::int64_t{-5});
  EXPECT_TRUE(numsim::cas::is_negative(c));
  EXPECT_TRUE(numsim::cas::is_nonpositive(c));
  EXPECT_TRUE(numsim::cas::is_nonzero(c));
  EXPECT_TRUE(numsim::cas::is_integer(c));
  EXPECT_FALSE(numsim::cas::is_positive(c));
}

TEST(ScalarConstantValueAssumptions, IntegerZeroAndDoubleZeroCarrySameFactSet) {
  // Architect b.1: zero-spelling consistency. Pre-fix, scalar_constant(0)
  // got {integer, rational} but scalar_constant(0.0) didn't — divergent
  // contract by storage. SymPy treats S(0), S(0.0), Rational(0)
  // identically (zero is integer regardless of spelling).
  auto c_int = numsim::cas::make_expression<numsim::cas::scalar_constant>(
      std::int64_t{0});
  auto c_dbl = numsim::cas::make_expression<numsim::cas::scalar_constant>(0.0);
  // Both must be nonneg AND nonpos (zero satisfies both vacuously).
  EXPECT_TRUE(numsim::cas::is_nonnegative(c_int));
  EXPECT_TRUE(numsim::cas::is_nonpositive(c_int));
  EXPECT_TRUE(numsim::cas::is_nonnegative(c_dbl));
  EXPECT_TRUE(numsim::cas::is_nonpositive(c_dbl));
  // Both must be NOT nonzero (zero is the additive identity).
  EXPECT_FALSE(numsim::cas::is_nonzero(c_int));
  EXPECT_FALSE(numsim::cas::is_nonzero(c_dbl));
  // Zero is integer + rational + real regardless of spelling. The
  // is_rational query helper was added alongside this test —
  // closes the architect's final gap so a regression stripping the
  // rational tag from either branch surfaces.
  EXPECT_TRUE(numsim::cas::is_integer(c_int));
  EXPECT_TRUE(numsim::cas::is_integer(c_dbl));
  EXPECT_TRUE(numsim::cas::is_rational(c_int));
  EXPECT_TRUE(numsim::cas::is_rational(c_dbl));
  EXPECT_TRUE(numsim::cas::is_real(c_int));
  EXPECT_TRUE(numsim::cas::is_real(c_dbl));
}

TEST(ScalarConstantValueAssumptions, NegativeDoubleCarriesNegative) {
  // QA: the double < 0 branch had no coverage. Structurally symmetric
  // with negative-int but a distinct code path. Negative assertions
  // close the NOT-side (architect/QA: pins against a regression that
  // accidentally inserts nonneg/positive for all non-positive values).
  auto c = numsim::cas::make_expression<numsim::cas::scalar_constant>(-5.0);
  EXPECT_TRUE(numsim::cas::is_negative(c));
  EXPECT_TRUE(numsim::cas::is_nonpositive(c));
  EXPECT_TRUE(numsim::cas::is_nonzero(c));
  EXPECT_TRUE(numsim::cas::is_real(c));
  EXPECT_FALSE(numsim::cas::is_positive(c));
  EXPECT_FALSE(numsim::cas::is_nonnegative(c));
  EXPECT_FALSE(numsim::cas::is_integer(c))
      << "non-zero doubles do NOT auto-claim integer";
  EXPECT_FALSE(numsim::cas::is_rational(c))
      << "non-zero doubles do NOT auto-claim rational either";
}

TEST(ScalarConstantValueAssumptions, NonzeroDoubleDoesNotClaimInteger) {
  // Counterpart to the zero case: a non-zero double like 5.0 must NOT
  // claim integer or rational. QA: symmetric with negative-double test.
  auto c = numsim::cas::make_expression<numsim::cas::scalar_constant>(5.0);
  EXPECT_TRUE(numsim::cas::is_positive(c));
  EXPECT_TRUE(numsim::cas::is_real(c));
  EXPECT_FALSE(numsim::cas::is_integer(c))
      << "non-zero doubles must not auto-claim integer";
  EXPECT_FALSE(numsim::cas::is_rational(c))
      << "non-zero doubles must not auto-claim rational either";
}

TEST(ScalarConstantValueAssumptions, RationalDenomOneNormalizesToInteger) {
  // Architect dead-code-removal lock-in: scalar_number's normalize_rational
  // is documented to collapse rational_t{N, 1} to int64{N} in the variant.
  // The annotate_from_value rational_t branch trusts this invariant — if
  // normalization ever regressed, integer-claiming on a rational_t with
  // unit denominator would silently fail. This test pins the invariant
  // via the constant's observable behavior.
  auto c = numsim::cas::make_expression<numsim::cas::scalar_constant>(
      numsim::cas::rational_t{7, 1});
  EXPECT_TRUE(numsim::cas::is_integer(c))
      << "rational_t{N, 1} must normalize to int64{N} and claim integer";
  EXPECT_TRUE(numsim::cas::is_rational(c));
  EXPECT_TRUE(numsim::cas::is_positive(c));
}

TEST(ScalarConstantValueAssumptions,
     RationalNontrivialDenomIsRationalNotInteger) {
  // QA Q3c: rational with non-unit denominator — the only live rational_t
  // branch (scalar_number normalizes den==1 to int64). Verify the
  // negation (den != 1 → not integer but still rational + real).
  auto c = numsim::cas::make_expression<numsim::cas::scalar_constant>(
      numsim::cas::rational_t{1, 3});
  EXPECT_TRUE(numsim::cas::is_positive(c));
  EXPECT_TRUE(numsim::cas::is_real(c));
  EXPECT_TRUE(numsim::cas::is_rational(c));
  EXPECT_FALSE(numsim::cas::is_integer(c));
}

TEST(ScalarConstantValueAssumptions, ComplexCarriesNoSignOrRealPredicates) {
  // QA Q3d: complex values get NO sign predicates AND NO real_tag. This
  // branch is purely negative (asserts nothing inserted) — the riskiest
  // case because a future edit that adds real_tag for complex would be
  // silently invisible without this test. Includes is_nonnegative /
  // is_even as the most-plausible accidental insertions (e.g. if someone
  // misused magnitude or even-bit logic on a complex value). The
  // !is_rational assertion is the false-path lock-in for the
  // is_rational query helper (QA fifth-round gap).
  auto c = numsim::cas::make_expression<numsim::cas::scalar_constant>(
      std::complex<double>{1.0, 1.0});
  EXPECT_FALSE(numsim::cas::is_positive(c));
  EXPECT_FALSE(numsim::cas::is_negative(c));
  EXPECT_FALSE(numsim::cas::is_nonzero(c));
  EXPECT_FALSE(numsim::cas::is_nonnegative(c));
  EXPECT_FALSE(numsim::cas::is_real(c));
  EXPECT_FALSE(numsim::cas::is_integer(c));
  EXPECT_FALSE(numsim::cas::is_rational(c));
  EXPECT_FALSE(numsim::cas::is_even(c));
}

// ─── Step 5: expression_holder::assumption() variadic API ─────────────
// SymPy-style fluent assertion. Single-fact, multi-fact, chained,
// 0-fact, and on-non-Symbol error paths all covered.

TEST_F(AssumptionFixture, AssumptionSingleFactSucceeds) {
  // Equivalent to assume(x, positive{}) — same implication chain runs.
  x.assumption(numsim::cas::positive{});
  EXPECT_TRUE(numsim::cas::is_positive(x));
  EXPECT_TRUE(numsim::cas::is_nonnegative(x));
  EXPECT_TRUE(numsim::cas::is_nonzero(x));
  EXPECT_TRUE(numsim::cas::is_real(x));
}

TEST_F(AssumptionFixture, AssumptionMultiFactRunsAllChains) {
  // Both facts' implication chains run, in left-to-right order. positive
  // implies {nonneg, nonzero, real}; integer implies {rational, real}.
  // Union of all derived facts must be present.
  x.assumption(numsim::cas::positive{}, numsim::cas::integer{});
  EXPECT_TRUE(numsim::cas::is_positive(x));
  EXPECT_TRUE(numsim::cas::is_integer(x));
  EXPECT_TRUE(numsim::cas::is_rational(x));
  EXPECT_TRUE(numsim::cas::is_real(x));
  EXPECT_TRUE(numsim::cas::is_nonzero(x));
}

TEST_F(AssumptionFixture, AssumptionChainableReturnsSelf) {
  // Fluent chaining: A.assumption(f1).assumption(f2).
  x.assumption(numsim::cas::positive{}).assumption(numsim::cas::integer{});
  EXPECT_TRUE(numsim::cas::is_positive(x));
  EXPECT_TRUE(numsim::cas::is_integer(x));
}

TEST_F(AssumptionFixture, AssumptionZeroFactsIsNoOp) {
  // 0-fact case must not mutate state and must not throw.
  EXPECT_NO_THROW(x.assumption());
  EXPECT_FALSE(numsim::cas::is_positive(x))
      << "0-fact assumption() must not assert anything";
}

TEST_F(AssumptionFixture, AssumptionOnCompoundThrows) {
  // Compound is not a Symbol — variadic API throws at the holder-level
  // guard, before any per-fact dispatch fires.
  auto sum = x + y;
  EXPECT_THROW(sum.assumption(numsim::cas::positive{}),
               numsim::cas::invalid_assumption_error);
}

TEST_F(AssumptionFixture, AssumptionOnSymbolStateUnchangedOnThrow) {
  // The throw path on a compound must not mutate x's or y's state
  // (require_symbol fires BEFORE the per-fact dispatch loop).
  auto compound = x + y;
  try {
    compound.assumption(numsim::cas::positive{}, numsim::cas::integer{});
  } catch (numsim::cas::invalid_assumption_error const &) {
    // expected
  }
  EXPECT_FALSE(numsim::cas::is_positive(x));
  EXPECT_FALSE(numsim::cas::is_integer(x));
}

TEST_F(AssumptionFixture, AssumptionThreeFactsAllApplied) {
  // QA: fold expansion with arity > 2. Pins all three implication chains
  // fire in left-to-right order; even though C++17+ fold expressions are
  // language-guaranteed to do this, the test catches accidental refactors
  // (e.g. someone writes a manual loop that short-circuits early).
  x.assumption(numsim::cas::positive{}, numsim::cas::integer{},
               numsim::cas::nonzero{});
  EXPECT_TRUE(numsim::cas::is_positive(x));
  EXPECT_TRUE(numsim::cas::is_integer(x));
  EXPECT_TRUE(numsim::cas::is_nonzero(x));
  EXPECT_TRUE(numsim::cas::is_real(x));
}

TEST_F(AssumptionFixture, AssumptionAccumulatesAcrossSeparateCalls) {
  // QA: pin that a later assumption() call doesn't clear or overwrite
  // earlier facts. The ChainableReturnsSelf test only queries final state;
  // this checks the intermediate state survives.
  x.assumption(numsim::cas::positive{});
  EXPECT_TRUE(numsim::cas::is_positive(x));
  x.assumption(numsim::cas::integer{});
  EXPECT_TRUE(numsim::cas::is_positive(x)) << "first call must not be cleared";
  EXPECT_TRUE(numsim::cas::is_integer(x));
}

TEST_F(AssumptionFixture, AssumptionChainableReturnsSelfByIdentity) {
  // QA: ChainableReturnsSelf passed even if assumption() returned a copy
  // (shared_ptr semantics make all copies see the same node). This test
  // pins the actual contract — returns *this by reference, not a copy.
  auto &ref = x.assumption(numsim::cas::positive{});
  EXPECT_EQ(&ref, &x)
      << "assumption() must return *this by reference, not a copy";
}

TEST_F(AssumptionFixture, AssumptionZeroFactsChainable) {
  // QA: x.assumption().assumption() must compile and preserve identity
  // (the if constexpr early-return path also returns *this). Also pins
  // the address identity for the 0-fact branch — a regression that
  // default-constructed the return holder would fail the EXPECT_EQ.
  auto &ref0 = x.assumption();
  EXPECT_EQ(&ref0, &x) << "0-fact assumption() must return *this by reference";
  EXPECT_NO_THROW(ref0.assumption());
  EXPECT_FALSE(numsim::cas::is_positive(x));
}

TEST_F(AssumptionFixture, AssumptionOnT2sWrappedScalarSymbol) {
  // Architect Q2 step-5 review gap: a holder<t2s_expression> over a
  // wrapped scalar Symbol must accept assumption(fact). is_symbol()
  // forwards (step 1); apply_assumption now forwards too (this commit).
  // Without the t2s overload this call was a compile error despite the
  // require_symbol guard claiming the holder was a Symbol.
  auto wrapped = numsim::cas::make_expression<
      numsim::cas::tensor_to_scalar_scalar_wrapper>(x);
  wrapped.assumption(numsim::cas::positive{});
  // The fact lands on the inner scalar x — both holders see it via the
  // shared underlying node.
  EXPECT_TRUE(numsim::cas::is_positive(x));
}

TEST(ScalarAssumptionConcept, RejectsBogusFactTypes) {
  // Architect Q1: the assumption_fact_for concept gates the variadic
  // pack. Bogus types like int produce a "constraints not satisfied"
  // diagnostic rather than a deep ADL-failure spew.
  static_assert(
      numsim::cas::assumption_fact_for<numsim::cas::scalar_expression,
                                       numsim::cas::positive>,
      "positive must satisfy assumption_fact_for<scalar_expression, ...>");
  static_assert(
      !numsim::cas::assumption_fact_for<numsim::cas::scalar_expression, int>,
      "bogus int must NOT satisfy assumption_fact_for<scalar_expression, ...>");
}

TEST(AssumptionConceptCrossDomain, RejectsWrongDomainFactTypes) {
  // QA Q4 / architect: cross-domain rejection. Scalar tags must NOT
  // satisfy the concept on tensor holders, and vice versa. If someone
  // accidentally widens an overload (e.g. catch-all template forwarding)
  // these static_asserts surface the leak.
  using TE = numsim::cas::tensor_expression;
  using SE = numsim::cas::scalar_expression;

  // Tensor holder must reject scalar-only tags (no apply_assumption
  // overload exists for that pair).
  static_assert(!numsim::cas::assumption_fact_for<TE, numsim::cas::positive>,
                "scalar tag 'positive' must NOT be valid on tensor holders");

  // Tensor holder must accept its own structural tags.
  static_assert(numsim::cas::assumption_fact_for<TE, numsim::cas::Symmetric>,
                "Symmetric must be valid on tensor holders");

  // Scalar holder must reject tensor structural tags.
  static_assert(!numsim::cas::assumption_fact_for<SE, numsim::cas::Symmetric>,
                "tensor tag 'Symmetric' must NOT be valid on scalar holders");

  // Cross-domain algebraic facts (orthogonal lives on tensor only).
  static_assert(numsim::cas::assumption_fact_for<TE, numsim::cas::orthogonal>,
                "orthogonal must be valid on tensor holders");
  static_assert(!numsim::cas::assumption_fact_for<SE, numsim::cas::orthogonal>,
                "orthogonal (tensor-only) must NOT be valid on scalar holders");
}

TEST(ScalarAssumptionConcept, IrrationalAndComplexAreUnsupported) {
  // cpp-pro F1: irrational and complex_tag are part of numeric_assumption
  // but have NO assume() overloads. The trait deliberately omits them so
  // the concept rejects them up-front instead of admitting them and
  // failing inside the template body. If support is later added (assume()
  // overload + trait specialization), this test should flip.
  using SE = numsim::cas::scalar_expression;
  static_assert(!numsim::cas::assumption_fact_for<SE, numsim::cas::irrational>,
                "irrational has no assume() overload; concept must reject");
  static_assert(!numsim::cas::assumption_fact_for<SE, numsim::cas::complex_tag>,
                "complex_tag has no assume() overload; concept must reject");
}

// ─── Step 6: cross-domain consistency sweep ─────────────────────────
// Verification (not new feature) that the step-2/3/4/5 work landed
// consistently across the scalar / tensor / t2s domains. Audits the
// three step-6 invariants spelled out in
// docs/sympy-assumption-redesign.md.

TEST_F(AssumptionFixture, Step6_AssumeOnT2sWrapperRoutesToInnerScalar) {
  // Cross-domain integration: the variadic API on a t2s holder over a
  // wrapped scalar Symbol must apply the fact to the inner scalar's
  // assumption set, observable via the original scalar holder.
  auto wrapped = numsim::cas::make_expression<
      numsim::cas::tensor_to_scalar_scalar_wrapper>(x);
  wrapped.assumption(numsim::cas::positive{}, numsim::cas::integer{});
  // The shared underlying scalar node has both facts.
  EXPECT_TRUE(numsim::cas::is_positive(x));
  EXPECT_TRUE(numsim::cas::is_integer(x));
}

TEST_F(AssumptionFixture, Step6_ConceptDispatchT2sParityWithScalar) {
  // The t2s wrapper's apply_assumption overload is constrained by the
  // same is_numeric_assumption_tag trait as the scalar template. Pin
  // that the concept produces the same answer through both routes for
  // the supported tags AND the deliberately-unsupported tags.
  using SE = numsim::cas::scalar_expression;
  using TS = numsim::cas::tensor_to_scalar_expression;

  // Supported scalar tag: accepted on both scalar and t2s holders.
  static_assert(numsim::cas::assumption_fact_for<SE, numsim::cas::positive>);
  static_assert(numsim::cas::assumption_fact_for<TS, numsim::cas::positive>);

  // Tensor structural tag: rejected on both scalar and t2s holders.
  static_assert(!numsim::cas::assumption_fact_for<SE, numsim::cas::Symmetric>);
  static_assert(!numsim::cas::assumption_fact_for<TS, numsim::cas::Symmetric>);

  // Deliberately unsupported tag: rejected on both.
  static_assert(!numsim::cas::assumption_fact_for<SE, numsim::cas::irrational>);
  static_assert(!numsim::cas::assumption_fact_for<TS, numsim::cas::irrational>);
}

TEST_F(AssumptionFixture, Step6_T2sWrapperQueryRequiresInnerUnwrap) {
  // Mixed-domain sentinel: the propagator and is_* helpers are scalar-only.
  // A t2s wrapper around a positive scalar Symbol does NOT have its own
  // m_assumption populated — the wrapper's assumption set is independent
  // of the inner scalar's. Querying the wrapper directly is impossible
  // (no is_* overload for t2s holders); the supported workaround is to
  // unwrap and query the inner scalar.
  //
  // This test documents the current limitation. A future step (1.1 or
  // later) could add t2s query helpers that forward through the wrapper;
  // when that lands, the workaround portion stays valid as a parallel
  // path, and this sentinel can be replaced with the forwarded query.
  numsim::cas::assume(x, numsim::cas::positive{});
  auto wrapped = numsim::cas::make_expression<
      numsim::cas::tensor_to_scalar_scalar_wrapper>(x);

  // Sentinel: the wrapper's OWN assumption set is empty — the inner
  // scalar's assumption set is what got populated.
  EXPECT_FALSE(wrapped.get().assumptions().contains(numsim::cas::positive{}))
      << "wrapper's m_assumption is independent of the inner scalar's";

  // Workaround: unwrap and query the inner scalar.
  auto inner =
      wrapped.get<numsim::cas::tensor_to_scalar_scalar_wrapper>().expr();
  EXPECT_TRUE(numsim::cas::is_positive(inner))
      << "unwrap-and-query is the supported path until t2s inference lands";
}

TEST_F(AssumptionFixture, Step6_ScalarAssumeUniformGuardSampling) {
  // Step-4 made every scalar assume() overload guard on require_symbol.
  // This sweep samples the 11 overloads (positive, negative, nonnegative,
  // nonpositive, nonzero, integer, even, odd, prime, rational, real_tag)
  // on the same compound expression and asserts every one throws — pins
  // the uniformity invariant against any future overload that forgets
  // the guard.
  auto compound = x + y;
  EXPECT_THROW(numsim::cas::assume(compound, numsim::cas::positive{}),
               numsim::cas::invalid_assumption_error);
  EXPECT_THROW(numsim::cas::assume(compound, numsim::cas::negative{}),
               numsim::cas::invalid_assumption_error);
  EXPECT_THROW(numsim::cas::assume(compound, numsim::cas::nonnegative{}),
               numsim::cas::invalid_assumption_error);
  EXPECT_THROW(numsim::cas::assume(compound, numsim::cas::nonpositive{}),
               numsim::cas::invalid_assumption_error);
  EXPECT_THROW(numsim::cas::assume(compound, numsim::cas::nonzero{}),
               numsim::cas::invalid_assumption_error);
  EXPECT_THROW(numsim::cas::assume(compound, numsim::cas::integer{}),
               numsim::cas::invalid_assumption_error);
  EXPECT_THROW(numsim::cas::assume(compound, numsim::cas::even{}),
               numsim::cas::invalid_assumption_error);
  EXPECT_THROW(numsim::cas::assume(compound, numsim::cas::odd{}),
               numsim::cas::invalid_assumption_error);
  EXPECT_THROW(numsim::cas::assume(compound, numsim::cas::prime{}),
               numsim::cas::invalid_assumption_error);
  EXPECT_THROW(numsim::cas::assume(compound, numsim::cas::rational{}),
               numsim::cas::invalid_assumption_error);
  EXPECT_THROW(numsim::cas::assume(compound, numsim::cas::real_tag{}),
               numsim::cas::invalid_assumption_error);
}

#endif // SCALARASSUMPTIONTEST_H
