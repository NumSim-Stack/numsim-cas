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
  EXPECT_TRUE(numsim::cas::is_integer_assumed(c));
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
  EXPECT_TRUE(numsim::cas::is_integer_assumed(c));
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
  // Zero is integer regardless of spelling.
  EXPECT_TRUE(numsim::cas::is_integer_assumed(c_int));
  EXPECT_TRUE(numsim::cas::is_integer_assumed(c_dbl));
}

TEST(ScalarConstantValueAssumptions, NonzeroDoubleDoesNotClaimInteger) {
  // Counterpart to the zero case: a non-zero double like 5.0 must NOT
  // claim integer, because IEEE 754 doubles in general represent
  // approximations. SymPy follows the same convention.
  auto c = numsim::cas::make_expression<numsim::cas::scalar_constant>(5.0);
  EXPECT_TRUE(numsim::cas::is_positive(c));
  EXPECT_TRUE(numsim::cas::is_real(c));
  EXPECT_FALSE(numsim::cas::is_integer_assumed(c))
      << "non-zero doubles must not auto-claim integer";
}

TEST(ScalarConstantValueAssumptions,
     RationalNontrivialDenomIsRationalNotInteger) {
  // QA Q3c: rational with non-unit denominator. Pre-fix path had a
  // den == 1 → integer branch; verify the negation (den != 1 → not
  // integer but still rational/real).
  auto c = numsim::cas::make_expression<numsim::cas::scalar_constant>(
      numsim::cas::rational_t{1, 3});
  EXPECT_TRUE(numsim::cas::is_positive(c));
  EXPECT_TRUE(numsim::cas::is_real(c));
  EXPECT_FALSE(numsim::cas::is_integer_assumed(c));
}

TEST(ScalarConstantValueAssumptions, ComplexCarriesNoSignOrRealPredicates) {
  // QA Q3d: complex values get NO sign predicates AND NO real_tag. This
  // branch is purely negative (asserts nothing inserted) — the riskiest
  // case because a future edit that adds real_tag for complex would be
  // silently invisible without this test.
  auto c = numsim::cas::make_expression<numsim::cas::scalar_constant>(
      std::complex<double>{1.0, 1.0});
  EXPECT_FALSE(numsim::cas::is_positive(c));
  EXPECT_FALSE(numsim::cas::is_negative(c));
  EXPECT_FALSE(numsim::cas::is_nonzero(c));
  EXPECT_FALSE(numsim::cas::is_real(c));
  EXPECT_FALSE(numsim::cas::is_integer_assumed(c));
}

#endif // SCALARASSUMPTIONTEST_H
