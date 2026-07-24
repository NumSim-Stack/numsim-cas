#ifndef COREBUGFIXTEST_H
#define COREBUGFIXTEST_H

#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"
#include <cmath>
#include <numsim_cas/core/substitute.h>
#include <numsim_cas/tensor/visitors/tensor_substitution.h>

namespace numsim::cas {

// ---------------------------------------------------------------------------
// Bug 1.1: symbol_base equality now compares names, not hashes
// ---------------------------------------------------------------------------

TEST(CoreBugFix, SymbolEqualitySameName) {
  auto [x1] = make_scalar_variable("x");
  auto [x2] = make_scalar_variable("x");
  // Two symbols with the same name must compare equal
  EXPECT_EQ(x1, x2);
}

TEST(CoreBugFix, SymbolEqualityDifferentName) {
  auto [x] = make_scalar_variable("x");
  auto [y] = make_scalar_variable("y");
  EXPECT_NE(x, y);
}

TEST(CoreBugFix, SymbolOrderingByName) {
  auto [a] = make_scalar_variable("a");
  auto [b] = make_scalar_variable("b");
  // Two different symbols must be distinguishable via expression_holder
  // ordering
  EXPECT_TRUE(a < b || b < a);
}

// ---------------------------------------------------------------------------
// Bug 1.2: n_ary_tree::insert_hash throws on duplicate
// ---------------------------------------------------------------------------

TEST(CoreBugFix, NaryTreeDuplicateInsertThrows) {
  auto [x] = make_scalar_variable("x");
  // Build an add node manually to trigger duplicate insertion
  auto add_node = std::make_shared<scalar_add>();
  add_node->push_back(x);
  EXPECT_THROW(add_node->push_back(x), internal_error);
}

TEST(CoreBugFix, NaryTreeDuplicateInsertIsCatchableAsCasError) {
  auto [x] = make_scalar_variable("x");
  auto add_node = std::make_shared<scalar_add>();
  add_node->push_back(x);
  EXPECT_THROW(add_node->push_back(x), cas_error);
}

TEST(CoreBugFix, NaryTreeDuplicateInsertCarriesMessage) {
  auto [x] = make_scalar_variable("x");
  auto add_node = std::make_shared<scalar_add>();
  add_node->push_back(x);
  try {
    add_node->push_back(x);
    FAIL() << "Expected internal_error";
  } catch (internal_error const &e) {
    EXPECT_TRUE(std::string(e.what()).find("duplicate") != std::string::npos);
  }
}

// ---------------------------------------------------------------------------
// Bug 2.1: coeff() const returns const ref
// ---------------------------------------------------------------------------

TEST(CoreBugFix, CoeffConstReturnsConstRef) {
  // Verify at compile time that the const overload returns a const reference.
  // If this test compiles, the fix is correct.
  auto [x, y] = make_scalar_variable("x", "y");
  auto expr = x + y;
  const auto &add_node = expr.get<scalar_add>();
  // The const overload of coeff() must return a const reference
  using coeff_ref_type = decltype(add_node.coeff());
  static_assert(std::is_const_v<std::remove_reference_t<coeff_ref_type>>,
                "coeff() const must return const reference");
  (void)add_node.coeff(); // suppress unused warning
}

// ---------------------------------------------------------------------------
// Bug 3.1: expression_holder null access throws
// ---------------------------------------------------------------------------

TEST(CoreBugFix, ExpressionHolderNullDerefThrows) {
  expression_holder<scalar_expression> null_expr;
  EXPECT_FALSE(null_expr.is_valid());
  EXPECT_THROW(*null_expr, invalid_expression_error);
}

TEST(CoreBugFix, ExpressionHolderNullArrowThrows) {
  expression_holder<scalar_expression> null_expr;
  EXPECT_THROW(null_expr.operator->(), invalid_expression_error);
}

TEST(CoreBugFix, ExpressionHolderNullGetThrows) {
  expression_holder<scalar_expression> null_expr;
  EXPECT_THROW(null_expr.get(), invalid_expression_error);
}

TEST(CoreBugFix, ExpressionHolderNullCompareThrows) {
  expression_holder<scalar_expression> null_a;
  expression_holder<scalar_expression> null_b;
  EXPECT_THROW((void)(null_a == null_b), invalid_expression_error);
  EXPECT_THROW((void)(null_a < null_b), invalid_expression_error);
}

TEST(CoreBugFix, ExpressionHolderNullLhsCompareThrows) {
  expression_holder<scalar_expression> null_expr;
  auto [x] = make_scalar_variable("x");
  EXPECT_THROW((void)(null_expr == x), invalid_expression_error);
  EXPECT_THROW((void)(null_expr < x), invalid_expression_error);
}

TEST(CoreBugFix, InvalidExpressionErrorIsCatchableAsCasError) {
  expression_holder<scalar_expression> null_expr;
  EXPECT_THROW(*null_expr, cas_error);
}

TEST(CoreBugFix, InvalidExpressionErrorIsCatchableAsRuntimeError) {
  expression_holder<scalar_expression> null_expr;
  EXPECT_THROW(*null_expr, std::runtime_error);
}

TEST(CoreBugFix, InvalidExpressionErrorCarriesMessage) {
  expression_holder<scalar_expression> null_expr;
  try {
    (void)*null_expr;
    FAIL() << "Expected invalid_expression_error";
  } catch (invalid_expression_error const &e) {
    EXPECT_TRUE(std::string(e.what()).find("null") != std::string::npos);
  }
}

// ---------------------------------------------------------------------------
// Bug 1.3: expression_holder::operator< consistent with operator==
// ---------------------------------------------------------------------------

TEST(CoreBugFix, StrictWeakOrderingConsistency) {
  auto [x] = make_scalar_variable("x");
  auto [y] = make_scalar_variable("y");
  // Same expression: !(a<a) and a==a
  EXPECT_FALSE(x < x);
  EXPECT_TRUE(x == x);
  // Different expressions: !(a<b) && !(b<a) implies a==b
  if (!(x < y) && !(y < x)) {
    EXPECT_EQ(x, y);
  }
  // Antisymmetry: if a<b then !(b<a)
  if (x < y) {
    EXPECT_FALSE(y < x);
  }
}

TEST(CoreBugFix, StrictWeakOrderingCompoundExpressions) {
  auto [a, b] = make_scalar_variable("a", "b");
  auto expr1 = a + b;
  auto expr2 = a + b;
  // Equivalent compound expressions
  EXPECT_EQ(expr1, expr2);
  EXPECT_FALSE(expr1 < expr2);
  EXPECT_FALSE(expr2 < expr1);
}

// ---------------------------------------------------------------------------
// Sanity: valid expressions still work as before
// ---------------------------------------------------------------------------

TEST(CoreBugFix, ValidExpressionHolderAccessWorks) {
  auto [x] = make_scalar_variable("x");
  EXPECT_TRUE(x.is_valid());
  EXPECT_NO_THROW(*x);
  EXPECT_NO_THROW(x.get());
  EXPECT_NE(x.operator->(), nullptr);
}

TEST(CoreBugFix, ValidExpressionComparisonWorks) {
  auto [x, y] = make_scalar_variable("x", "y");
  EXPECT_NO_THROW((void)(x == x));
  EXPECT_NO_THROW((void)(x < y));
  EXPECT_EQ(x, x);
}

TEST(CoreBugFix, NaryTreeNonDuplicateInsertWorks) {
  auto [x, y] = make_scalar_variable("x", "y");
  auto add_node = std::make_shared<scalar_add>();
  EXPECT_NO_THROW(add_node->push_back(x));
  EXPECT_NO_THROW(add_node->push_back(y));
  EXPECT_EQ(add_node->size(), 2u);
}

// ---------------------------------------------------------------------------
// is_same on an invalid expression returns false (used to assert)
// ---------------------------------------------------------------------------

TEST(CoreBugFix, IsSameOnInvalidExpressionReturnsFalse) {
  expression_holder<scalar_expression> null_expr;
  EXPECT_FALSE(is_same<scalar_zero>(null_expr));
}

// ---------------------------------------------------------------------------
// scalar_pow differentiation: constant base, non-constant exponent
//   d/dx(2^x) = 2^x * log(2)
// Previously took the wrong branch (treated dg as the only contributor) and
// dropped the log(g)*dh term.
// ---------------------------------------------------------------------------

TEST(CoreBugFix, ScalarPowDiffConstBaseVariableExponent) {
  auto [x] = make_scalar_variable("x");
  auto two = make_scalar_constant(2);
  auto expr = pow(two, x);
  auto d = diff(expr, x);
  // Evaluate at x=0: d/dx(2^x)|_{x=0} = 2^0 * log(2) = log(2) ~= 0.6931...
  scalar_evaluator<double> ev;
  ev.set(x, 0.0);
  EXPECT_NEAR(ev.apply(d), std::log(2.0), 1e-12);
}

TEST(CoreBugFix, ScalarPowDiffVariableBaseConstExponent) {
  // d/dx(x^3) = 3 x^2 — confirms the long-standing branch still works.
  auto [x] = make_scalar_variable("x");
  auto expr = pow(x, 3);
  auto d = diff(expr, x);
  scalar_evaluator<double> ev;
  ev.set(x, 2.0);
  EXPECT_NEAR(ev.apply(d), 12.0, 1e-12);
}

TEST(CoreBugFix, ScalarPowDiffBothConstWrtArg) {
  // pow(y, z) differentiated by an unrelated x must yield 0, not crash.
  auto [x, y, z] = make_scalar_variable("x", "y", "z");
  auto expr = pow(y, z);
  auto d = diff(expr, x);
  EXPECT_TRUE(is_same<scalar_zero>(d));
}

// ---------------------------------------------------------------------------
// merge_or_insert: smoke test that compound add constructions succeed.
// The deterministic transitive-collision case requires a specific algebraic
// simplification path that this codebase does not currently expose as a fixed
// rule (e.g. there is no sin^2+cos^2 -> 1 reduction); the real exercise of
// merge_or_insert's loop happens in the fuzz suite, which previously skipped
// the seeds that hit it. Keep this as a smoke check that construction does
// not regress to the duplicate-child internal_error.
// ---------------------------------------------------------------------------

TEST(CoreBugFix, AddCompoundConstructionValue) {
  // (cos²(x)+sin²(x)+y) + (1+1+y) should construct without throwing and
  // evaluate correctly. The Pythagorean rule in scalar_simplifier_add
  // reduces cos²+sin² to 1, so the sum is 3+2y. At y=5 → 13.
  // Upgraded from no-throw-only smoke test per issue #113 / PR #114-style
  // value-assertion principle.
  auto [x, y] = make_scalar_variable("x", "y");
  auto one = make_scalar_constant(1);
  auto lhs = pow(cos(x), 2) + pow(sin(x), 2) + y;
  auto rhs = one + one + y;
  auto sum = lhs + rhs;
  scalar_evaluator<double> ev;
  ev.set(x, 0.5); // any x: cos²+sin² = 1
  ev.set(y, 5.0);
  EXPECT_NEAR(ev.apply(sum), 13.0, 1e-12);
}

// NOTE: SubSymbolDispatchSmoke was superseded by
// NArySubSymbolDispatchCancelsCleanly in PR #100 (the value-asserting
// version of the same path). Deleted per issue #113.

TEST(CoreBugFix, NegativeSubAddDispatchValue) {
  // -x - (y+z) at (x=2, y=3, z=4) = -2 - 7 = -9.
  // Exercises merge_or_insert on negative_sub_dispatch::dispatch(add).
  // Upgraded from no-throw-only smoke test per issue #113.
  auto [x, y, z] = make_scalar_variable("x", "y", "z");
  auto diff = -x - (y + z);
  scalar_evaluator<double> ev;
  ev.set(x, 2.0);
  ev.set(y, 3.0);
  ev.set(z, 4.0);
  EXPECT_NEAR(ev.apply(diff), -9.0, 1e-12);
}

TEST(CoreBugFix, NArySubAddDispatchScalar) {
  // Regression for issue #91. Previously n_ary_sub_dispatch::dispatch(add, add)
  // combined coefficients with `+` (instead of `-`) and was unguarded against
  // invalid coefficients — the latter making (x+y+z) - (x+y) throw on the
  // unguarded `+` of two invalid holders.
  auto [x, y, z] = make_scalar_variable("x", "y", "z");
  auto [a, b] = make_scalar_variable("a", "b");
  auto two = make_scalar_constant(2);
  auto one = make_scalar_constant(1);

  // (x+y+z) - (x+y) -> z
  EXPECT_EQ((x + y + z) - (x + y), z);
  // (2+x) - (1+x) -> 1
  EXPECT_EQ((two + x) - (one + x), one);
  // (a+b) - (a+b) -> 0
  EXPECT_TRUE(is_same<scalar_zero>((a + b) - (a + b)));
}

// ---------------------------------------------------------------------------
// finalize_add<Traits> direct unit tests — the trivial-result collapse helper
// extracted from n_ary_sub_dispatch in #99.
// ---------------------------------------------------------------------------

TEST(CoreBugFix, FinalizeAddEmptyAndCoeffReturnsCoeff) {
  using Traits = domain_traits<scalar_expression>;
  auto two = make_scalar_constant(2);
  auto node = std::make_shared<scalar_add>();
  node->set_coeff(two);
  expression_holder<scalar_expression> expr{node};
  auto result = detail::finalize_add<Traits>(expr);
  EXPECT_EQ(result, two);
}

TEST(CoreBugFix, FinalizeAddEmptyNoCoeffReturnsZero) {
  using Traits = domain_traits<scalar_expression>;
  auto node = std::make_shared<scalar_add>();
  expression_holder<scalar_expression> expr{node};
  auto result = detail::finalize_add<Traits>(expr);
  EXPECT_TRUE(is_same<scalar_zero>(result));
}

TEST(CoreBugFix, FinalizeAddSingleChildNoCoeffReturnsChild) {
  using Traits = domain_traits<scalar_expression>;
  auto [x] = make_scalar_variable("x");
  auto node = std::make_shared<scalar_add>();
  node->push_back(x);
  expression_holder<scalar_expression> expr{node};
  auto result = detail::finalize_add<Traits>(expr);
  EXPECT_EQ(result, x);
}

TEST(CoreBugFix, FinalizeAddNonTrivialReturnsUnchanged) {
  using Traits = domain_traits<scalar_expression>;
  auto [x, y] = make_scalar_variable("x", "y");
  auto node = std::make_shared<scalar_add>();
  node->push_back(x);
  node->push_back(y);
  expression_holder<scalar_expression> expr{node};
  auto result = detail::finalize_add<Traits>(expr);
  EXPECT_EQ(result, expr);
}

TEST(CoreBugFix, NArySubSymbolDispatchCancelsCleanly) {
  // Regression: dispatch(SymbolType) used to leak a stray zero child when
  // the symbol matched a child and the cancellation x-x=0 was pushed back
  // via merge_or_insert without filtering. (2+x)-x produced "2+0" instead
  // of "2"; (x+y+z)-x produced a 3-child add (the stray 0 plus y, z)
  // instead of the 2-child y+z.
  auto [x, y, z] = make_scalar_variable("x", "y", "z");
  auto two = make_scalar_constant(2);

  // (2+x) - x -> 2 (empty children + valid coeff: finalize_add returns coeff)
  EXPECT_EQ((two + x) - x, two);
  // (x+y+z) - x -> y+z (two children survive; finalize_add returns the add)
  EXPECT_EQ((x + y + z) - x, y + z);
  // (x+y) - x -> y (one child + no coeff: finalize_add returns the child)
  EXPECT_EQ((x + y) - x, y);
}

TEST(CoreBugFix, NArySubSymbolDispatchNotFoundCombinesWithExisting) {
  // Regression: when m_rhs is not in lhs's symbol_map but -m_rhs is, the
  // dispatch used push_back(-m_rhs) which hit the duplicate-child guard.
  // Switched to merge_or_insert so the negation combines with the existing
  // entry instead.
  auto [x, y] = make_scalar_variable("x", "y");
  // (-x + y) - x: lhs has -x and y, neither key matches x. Without the fix
  // push_back(-x) collides with the existing -x. With merge_or_insert,
  // combine to (-2*x) + y. Lock in the value (not just no-throw) so a
  // future regression that drops the term silently can't pass.
  auto r = (-x + y) - x;
  EXPECT_EQ(r, -2 * x + y);
}

TEST(CoreBugFix, FinalizeAddSingleChildWithCoeffReturnsUnchanged) {
  // One child + valid coeff is a meaningful add (e.g. 1+x); not trivial.
  using Traits = domain_traits<scalar_expression>;
  auto [x] = make_scalar_variable("x");
  auto one = make_scalar_constant(1);
  auto node = std::make_shared<scalar_add>();
  node->set_coeff(one);
  node->push_back(x);
  expression_holder<scalar_expression> expr{node};
  auto result = detail::finalize_add<Traits>(expr);
  EXPECT_EQ(result, expr);
}

TEST(CoreBugFix, NArySubAddDispatchT2s) {
  // The #91 fix lives in a generic dispatcher template instantiated by both
  // scalar_traits and tensor_to_scalar_traits. This test locks in the t2s
  // path; tensor doesn't instantiate it because tensor_traits::mul_type is
  // void.
  auto [X, Y, Z] =
      make_tensor_variable(std::tuple{"X", std::size_t{3}, std::size_t{2}},
                           std::tuple{"Y", std::size_t{3}, std::size_t{2}},
                           std::tuple{"Z", std::size_t{3}, std::size_t{2}});

  // (trace(X) + trace(Y) + trace(Z)) - (trace(X) + trace(Y)) -> trace(Z)
  EXPECT_EQ((trace(X) + trace(Y) + trace(Z)) - (trace(X) + trace(Y)), trace(Z));
  // (trace(X) + trace(Y)) - (trace(X) + trace(Y)) -> 0
  EXPECT_TRUE(is_same<tensor_to_scalar_zero>((trace(X) + trace(Y)) -
                                             (trace(X) + trace(Y))));
}

// ---------------------------------------------------------------------------
// merge_or_insert public-state contract (issue #92 remains open for the
// multi-iteration deterministic case).
// ---------------------------------------------------------------------------

TEST(CoreBugFix, MergeOrInsertNoCollisionAddsChild) {
  // No collision: both children remain present as distinct entries.
  auto [x, y] = make_scalar_variable("x", "y");
  auto add_node = std::make_shared<scalar_add>();
  add_node->push_back(x);
  add_node->merge_or_insert(y);
  EXPECT_EQ(add_node->size(), 2u);
}

TEST(CoreBugFix, MergeOrInsertCollisionMergesIntoSingleEntry) {
  // Collision case: pushing x when x is already there triggers the
  // merge path and the two entries collapse into one. n_ary_tree's
  // hash excludes the coefficient, so x and (x + x) hash to the same
  // bucket — but the *stored key* is the combined expression itself
  // (i.e. 2*x), not the original x.
  auto [x] = make_scalar_variable("x");
  auto add_node = std::make_shared<scalar_add>();
  add_node->push_back(x);
  add_node->merge_or_insert(x);
  ASSERT_EQ(add_node->size(), 1u);
  EXPECT_EQ(add_node->symbol_map().begin()->first, make_scalar_constant(2) * x);
}

TEST(CoreBugFix, MergeOrInsertSequentialCallsAreIndependent) {
  // A collision followed by a non-collision must leave the tree in the
  // expected final shape: the collided entry stored as 2*x, and y added
  // alongside it. Asserting both keys (not just size()) is what makes
  // this a real lock-in for "calls don't bleed state" — a bug where
  // the second call corrupted the first call's stored entry would pass
  // a size-only check but fail the key checks.
  auto [x, y] = make_scalar_variable("x", "y");
  auto add_node = std::make_shared<scalar_add>();
  add_node->push_back(x);
  add_node->merge_or_insert(x); // collision: x -> 2*x
  add_node->merge_or_insert(y); // no collision: y added
  ASSERT_EQ(add_node->size(), 2u);
  bool found_2x = false;
  bool found_y = false;
  auto const expected_2x = make_scalar_constant(2) * x;
  for (auto const &[key, _] : add_node->symbol_map()) {
    if (key == expected_2x)
      found_2x = true;
    else if (key == y)
      found_y = true;
  }
  EXPECT_TRUE(found_2x) << "missing merged 2*x entry";
  EXPECT_TRUE(found_y) << "missing standalone y entry";
}

// NOTE: a deterministic multi-iteration (>1) test would require the
// codebase to expose an algebraic simplification that transitions the
// combined entry's hash key to one matching another existing entry.
// No such chain is reachable via construction-time operators as far as
// the simplifier dispatchers were checked during PR #100 — but the
// audit was not exhaustive across every per-domain wrapper, so the
// "no path exists" claim is best read as "no obvious path found." The
// loop's multi-iteration safety is forward-protection against future
// simplifier additions; the fuzz suite remains the witness if a chain
// is produced. See issue #92 for the deterministic-coverage follow-up.
// t2s_eval rebuild correctness lock-in (issue #94) — the per-visit rebuild
// path in tensor_evaluator::operator()(tensor_to_scalar_with_tensor_mul)
// is functionally correct but pays construction + symbol-table copy cost
// on every visit. Optimization is non-trivial because of the include
// cycle between tensor_evaluator.h and tensor_to_scalar_evaluator.h
// (see #94 for the architectural constraint). This test verifies the
// rebuild path produces correct results across many visits — any future
// optimization that caches or shares state must preserve the contract.
// ---------------------------------------------------------------------------

TEST(CoreBugFix, TensorToScalarWithTensorMulCorrectness) {
  // Construct a tensor_to_scalar_with_tensor_mul node directly (mirroring
  // the existing TensorEvaluatorTest pattern) and verify evaluation
  // produces correct results. The dispatcher for this node rebuilds a
  // fresh tensor_to_scalar_evaluator on every visit (issue #94); any
  // future optimization that caches the inner evaluator must preserve
  // the value contract this test locks in.
  //
  // Result access uses raw_data() rather than a static_cast back to the
  // concrete tensor_data<T,Dim,Rank> type — the raw buffer view doesn't
  // depend on the underlying representation, so a future optimization
  // that changed how the result is stored still satisfies the contract.
  auto A = make_expression<tensor>("A", std::size_t{3}, std::size_t{2});
  auto t2s_expr = trace(A);
  auto expr = make_expression<tensor_to_scalar_with_tensor_mul>(A, t2s_expr);

  // A = diag(1, 2, 3); trace(A) = 6; result = 6 * A.
  auto A_data = std::make_shared<tensor_data<double, 3, 2>>();
  auto *Araw = A_data->raw_data();
  for (std::size_t i = 0; i < 9; ++i)
    Araw[i] = 0.0;
  Araw[0] = 1.0;
  Araw[4] = 2.0;
  Araw[8] = 3.0;

  // Row-major indices 0,4,8 are the diagonal in a 3x3.
  auto check_diag = [](tensor_data_base<double> const &result) {
    auto const *raw = result.raw_data();
    EXPECT_NEAR(raw[0], 6.0, 1e-12);
    EXPECT_NEAR(raw[4], 12.0, 1e-12);
    EXPECT_NEAR(raw[8], 18.0, 1e-12);
    // off-diagonals
    EXPECT_NEAR(raw[1], 0.0, 1e-12);
    EXPECT_NEAR(raw[2], 0.0, 1e-12);
    EXPECT_NEAR(raw[5], 0.0, 1e-12);
  };

  tensor_evaluator<double> ev;
  ev.set(A, A_data);
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  check_diag(*result);

  // Re-apply with the same evaluator: the rebuild fires again, result
  // must be identical. Locks in idempotence across visits — a future
  // optimization that caches the inner t2s_eval must not leak state
  // from the first call into the second.
  auto result2 = ev.apply(expr);
  ASSERT_NE(result2, nullptr);
  check_diag(*result2);
}

// constant_sub_dispatch(add) bug fix (issue #102) — same pattern as #91.
// constant - (coeff + x) was computing m_lhs - rhs.coeff() unguarded against
// invalid rhs.coeff() (the common case when rhs has no constant term);
// `2 - (x + y)` threw on the unguarded operator-. Children were also pushed
// via push_back (collides with existing entries) and the result wasn't
// collapsed when trivial.
// ---------------------------------------------------------------------------

// Helper: evaluate scalar expression at a fixed point. The result of
// constant_sub_dispatch's rewrite produces `scalar_add{coeff, neg-children}`
// which can equivalent algebraically to `-(1+x+y)` (a scalar_negative
// wrapping the add) but differs structurally — comparison by numerical
// evaluation avoids the canonical-form ambiguity.
namespace {
double eval2(expression_holder<scalar_expression> const &expr,
             expression_holder<scalar_expression> const &x, double x_val,
             expression_holder<scalar_expression> const &y, double y_val) {
  scalar_evaluator<double> ev;
  ev.set(x, x_val);
  ev.set(y, y_val);
  return ev.apply(expr);
}
} // namespace

TEST(CoreBugFix, ConstantSubAddNoRhsCoeff) {
  // 2 - (x + y) used to throw because m_lhs - rhs.coeff() called operator-
  // on an invalid holder. Verify no-throw and that the result evaluates
  // correctly at a fixed point: 2 - (3 + 4) = -5.
  auto [x, y] = make_scalar_variable("x", "y");
  auto two = make_scalar_constant(2);
  expression_holder<scalar_expression> result;
  ASSERT_NO_THROW({ result = two - (x + y); });
  EXPECT_NEAR(eval2(result, x, 3.0, y, 4.0), -5.0, 1e-12);
}

TEST(CoreBugFix, ConstantSubAddWithRhsCoeff) {
  // 2 - (3 + x + y) at (x=2, y=3): 2 - (3+2+3) = -6.
  auto [x, y] = make_scalar_variable("x", "y");
  auto two = make_scalar_constant(2);
  auto three = make_scalar_constant(3);
  auto result = two - (three + x + y);
  EXPECT_NEAR(eval2(result, x, 2.0, y, 3.0), -6.0, 1e-12);
}

TEST(CoreBugFix, ConstantSubAddCoeffCancels) {
  // 2 - (2 + x) -> -x  (coeff cancels via the `c_l - c_r == 0` filter,
  // single child + no coeff: finalize_add collapses to the bare child).
  // Verify the structural collapse explicitly.
  auto [x] = make_scalar_variable("x");
  auto two = make_scalar_constant(2);
  auto result = two - (two + x);
  EXPECT_TRUE(is_same<scalar_negative>(result));
  EXPECT_EQ(result.get<scalar_negative>().expr(), x);
}

// ---------------------------------------------------------------------------
// Scalar-times-tensor squaring (issue #75).
// (X_tr * x) * (X_tr * x) should canonicalize to pow(x * X_tr, 2). The
// existing mul_dispatch::get_default's `if (m_lhs == m_rhs) return pow(.,2)`
// rule already covers this, but the issue body's testcase wasn't locked in
// by a named test — adding one here to prevent future regressions.
// ---------------------------------------------------------------------------

TEST(CoreBugFix, ScalarTensorMulSquaring) {
  auto [X] =
      make_tensor_variable(std::tuple{"X", std::size_t{3}, std::size_t{2}});
  auto [x] = make_scalar_variable("x");
  auto X_tr = trans(X);
  // (X_tr * x) * (X_tr * x) should simplify to pow(x * X_tr, 2). Both sides
  // are pow nodes wrapping the same tensor_scalar_mul; operator== verifies
  // structural equality.
  auto sq = (X_tr * x) * (X_tr * x);
  auto expected = pow(x * X_tr, 2);
  EXPECT_TRUE(is_same<tensor_pow>(sq));
  EXPECT_EQ(sq, expected);
}

// ---------------------------------------------------------------------------
// Skew annotation propagation lock-in (issue #93).
// On the build platform that motivated commit 7e962e5, the Skew space
// annotation could be lost when skew(A) was stored inside a tensor_mul.
// The structural skew classifier in skew_classification.h was added as
// a defensive fallback. These tests lock in that on THIS build the
// annotation IS preserved through the common composition paths — any
// regression would be visible here (whether or not the platform-specific
// loss the original commit observed ever recurs).
// ---------------------------------------------------------------------------

namespace {
template <typename Holder> bool is_skew_annotated(Holder const &e) {
  if (auto const &sp = e.get().space())
    return std::holds_alternative<Skew>(sp->perm);
  return false;
}
} // namespace

TEST(CoreBugFix, SkewSpacePreservedThroughNegation) {
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto sA = skew(A);
  ASSERT_TRUE(is_skew_annotated(sA));
  EXPECT_TRUE(is_skew_annotated(-sA));
}

TEST(CoreBugFix, SkewSpacePreservedThroughScalarMul) {
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto sA = skew(A);
  ASSERT_TRUE(is_skew_annotated(sA));
  EXPECT_TRUE(is_skew_annotated(2 * sA));
}

TEST(CoreBugFix, SkewSpacePreservedAsTensorMulChild) {
  // The case the original commit (7e962e5) flagged as platform-dependent:
  // skew(A) stored inside a tensor_mul. On this build the child retains
  // its Skew annotation. The structural classifier in skew_classification.h
  // is the authoritative fallback when the annotation IS lost on other
  // platforms.
  auto [A, B] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}},
                           std::tuple{"B", std::size_t{3}, std::size_t{2}});
  auto sA = skew(A);
  ASSERT_TRUE(is_skew_annotated(sA));
  auto prod = sA * B;
  ASSERT_TRUE(is_same<tensor_mul>(prod));
  auto const &mul = prod.get<tensor_mul>();
  ASSERT_EQ(mul.data().size(), 2u);
  // Find the skew child (the one carrying the annotation)
  bool found_skew_child = false;
  for (auto const &ch : mul.data()) {
    if (is_skew_annotated(ch)) {
      found_skew_child = true;
      break;
    }
  }
  EXPECT_TRUE(found_skew_child)
      << "skew(A) child of tensor_mul lost its Skew annotation on this build";
}

TEST(CoreBugFix, SkewSpacePreservedThroughRebuild) {
  // #93 — rebuild reconstructs nodes via variadic ctors, dropping
  // post-construction space(); substituting a leaf inside skew(A) must
  // keep it Skew (deterministic reproducer).
  auto [A, B] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}},
                           std::tuple{"B", std::size_t{3}, std::size_t{2}});
  auto sA = skew(A);
  ASSERT_TRUE(is_skew_annotated(sA));
  auto rebuilt = substitute(sA, A, B);
  EXPECT_TRUE(is_skew_annotated(rebuilt))
      << "Skew annotation lost through rebuild (#93)";
}

// ---------------------------------------------------------------------------
// Division-by-reciprocal canonicalisation (issue #49).
// `a * (1/b)` should canonicalise to `a/b`. Both produce the same
// pow(b, -1)-based structural form on construction (comment in
// scalar_operators.h:106: "pow(c, -1) stays structural and the printer
// formats as x/c"). The two are == today; this test locks the contract.
// ---------------------------------------------------------------------------

TEST(CoreBugFix, ScalarMulByReciprocalEqualsDivide) {
  auto [a, b] = make_scalar_variable("a", "b");
  // a * (1/b) == a / b
  EXPECT_EQ(a * pow(b, -1), a / b);
  // (1/b) * a == a / b  (commutative case)
  EXPECT_EQ(pow(b, -1) * a, a / b);
}

// ---------------------------------------------------------------------------
// scalar_evaluator::forward_values_to filters non-scalar keys
// ---------------------------------------------------------------------------

TEST(CoreBugFix, ForwardValuesToSkipsNonScalarKey) {
  // scalar_evaluator::set is templated on ExprBase, so callers could
  // mistakenly store a tensor symbol in the scalar evaluator's map. The
  // forwarding loop uses dynamic_pointer_cast<scalar_expression> to skip such
  // keys instead of force-casting them (which would be UB the next time the
  // destination tried to treat the holder as a scalar). Confirm the loop
  // survives the bad key.
  auto [x] = make_scalar_variable("x");
  auto T = std::get<0>(
      make_tensor_variable(std::tuple{"T", std::size_t{3}, std::size_t{2}}));

  scalar_evaluator<double> src;
  src.set(x, 1.5);
  src.set(T, 2.5); // tensor key forced into the scalar evaluator

  // tensor_evaluator is the real-world forwarding target; it exposes
  // set_scalar that forward_values_to expects.
  tensor_evaluator<double> dst;
  EXPECT_NO_THROW(src.forward_values_to(dst));
}

// ---------------------------------------------------------------------------
// #33: scalar std overloads — log10, sinh, cosh, tanh, asinh, acosh, atanh
// Implemented as compositions of existing primitives. The functions should
// produce expressions whose numerical evaluation matches the math definition.
// ---------------------------------------------------------------------------

TEST(CoreBugFix, ScalarLog10MatchesNumerical) {
  auto [x] = make_scalar_variable("x");
  auto expr = log10(x);
  scalar_evaluator<double> ev;
  ev.set(x, 100.0);
  EXPECT_NEAR(ev.apply(expr), 2.0, 1e-12);
}

TEST(CoreBugFix, ScalarSinhMatchesNumerical) {
  auto [x] = make_scalar_variable("x");
  auto expr = sinh(x);
  scalar_evaluator<double> ev;
  ev.set(x, 0.5);
  EXPECT_NEAR(ev.apply(expr), std::sinh(0.5), 1e-12);
}

TEST(CoreBugFix, ScalarCoshMatchesNumerical) {
  auto [x] = make_scalar_variable("x");
  auto expr = cosh(x);
  scalar_evaluator<double> ev;
  ev.set(x, 0.5);
  EXPECT_NEAR(ev.apply(expr), std::cosh(0.5), 1e-12);
}

TEST(CoreBugFix, ScalarTanhMatchesNumerical) {
  auto [x] = make_scalar_variable("x");
  auto expr = tanh(x);
  scalar_evaluator<double> ev;
  ev.set(x, 0.7);
  EXPECT_NEAR(ev.apply(expr), std::tanh(0.7), 1e-12);
}

TEST(CoreBugFix, ScalarAsinhMatchesNumerical) {
  auto [x] = make_scalar_variable("x");
  auto expr = asinh(x);
  scalar_evaluator<double> ev;
  ev.set(x, 1.5);
  EXPECT_NEAR(ev.apply(expr), std::asinh(1.5), 1e-12);
}

TEST(CoreBugFix, ScalarAcoshMatchesNumerical) {
  auto [x] = make_scalar_variable("x");
  auto expr = acosh(x);
  scalar_evaluator<double> ev;
  ev.set(x, 2.5); // domain: x >= 1
  EXPECT_NEAR(ev.apply(expr), std::acosh(2.5), 1e-12);
}

TEST(CoreBugFix, ScalarAtanhMatchesNumerical) {
  auto [x] = make_scalar_variable("x");
  auto expr = atanh(x);
  scalar_evaluator<double> ev;
  ev.set(x, 0.5); // domain: -1 < x < 1
  EXPECT_NEAR(ev.apply(expr), std::atanh(0.5), 1e-12);
}

// ---------------------------------------------------------------------------
// Construction-time short-circuits for the composition-based hyperbolic
// functions. These mirror what the underlying primitives (exp/log/sqrt/pow)
// already do for special arguments, but lift the simplification to the
// public API so callers see a clean expression without round-tripping
// through the composed form.
// ---------------------------------------------------------------------------

TEST(CoreBugFix, ScalarSinhOfZeroIsZero) {
  EXPECT_TRUE(is_same<scalar_zero>(sinh(get_scalar_zero())));
}

TEST(CoreBugFix, ScalarCoshOfZeroIsOne) {
  EXPECT_TRUE(is_same<scalar_one>(cosh(get_scalar_zero())));
}

TEST(CoreBugFix, ScalarCoshOfNegateFoldsToCosh) {
  // cosh(-x) = cosh(x) — even function.
  auto [x] = make_scalar_variable("x");
  EXPECT_EQ(cosh(-x), cosh(x));
}

TEST(CoreBugFix, ScalarTanhOfZeroIsZero) {
  EXPECT_TRUE(is_same<scalar_zero>(tanh(get_scalar_zero())));
}

TEST(CoreBugFix, ScalarTanhOfNegateFoldsToNegTanh) {
  // tanh(-x) = -tanh(x) — odd function.
  auto [x] = make_scalar_variable("x");
  EXPECT_EQ(tanh(-x), -tanh(x));
}

TEST(CoreBugFix, ScalarAsinhOfZeroIsZero) {
  EXPECT_TRUE(is_same<scalar_zero>(asinh(get_scalar_zero())));
}

TEST(CoreBugFix, ScalarAcoshOfOneIsZero) {
  EXPECT_TRUE(is_same<scalar_zero>(acosh(get_scalar_one())));
}

TEST(CoreBugFix, ScalarAtanhOfZeroIsZero) {
  EXPECT_TRUE(is_same<scalar_zero>(atanh(get_scalar_zero())));
}

// Lock-in: the "no new AST nodes, diff derives automatically through
// composition" promise of #33's overloads. If a refactor breaks the
// chain rule for log/exp/sqrt/pow, these wouldn't blow up structurally
// — they'd just produce wrong numbers. Test pins the numerical
// derivative against the closed-form identity:
//
//   d/dx sinh(x) = cosh(x)   (and similarly cosh derives via sinh).
//
// (#180 was fixed by re-defining tanh as (exp(2x)-1)/(exp(2x)+1) — only
// one exp() term, so diff doesn't fan out into duplicate adds.)
TEST(CoreBugFix, ScalarHyperbolicDerivativesMatchClosedForm) {
  auto [x] = make_scalar_variable("x");
  scalar_evaluator<double> ev;
  ev.set(x, 0.5);

  auto d_sinh = diff(sinh(x), x);
  EXPECT_NEAR(ev.apply(d_sinh), std::cosh(0.5), 1e-12);

  auto d_cosh = diff(cosh(x), x);
  EXPECT_NEAR(ev.apply(d_cosh), std::sinh(0.5), 1e-12);

  // #180 lock-in: tanh' = sech²(x) = 1/cosh²(x). The earlier
  // sinh(e)/cosh(e) form threw "duplicate child insertion" here; the
  // exp(2x)-based reformulation makes diff() finite-fan-out.
  auto d_tanh = diff(tanh(x), x);
  EXPECT_NEAR(ev.apply(d_tanh), 1.0 / (std::cosh(0.5) * std::cosh(0.5)), 1e-12);
}

// ---------------------------------------------------------------------------
// #184: canonical form of constant×expr must NOT depend on construction path.
// `int * x` and `make_scalar_constant(int) * x` should produce expressions
// that compare equal under `==` and merge in the n_ary_tree symbol_map.
// ---------------------------------------------------------------------------

TEST(CoreBugFix, Issue184_IntPathAndConstantPathMatch_Negative) {
  auto [x, y] = make_scalar_variable("x", "y");
  // Lifted directly from the issue's repro.
  auto v1 = -2 * x + y;                       // int * holder route
  auto v2 = make_scalar_constant(-2) * x + y; // explicit-constant route
  EXPECT_EQ(v1, v2);
}

TEST(CoreBugFix, Issue184_IntPathAndConstantPathMatch_AcrossValues) {
  auto [x] = make_scalar_variable("x");
  // Probe a spread of values: small + large, both signs. Skip 0 and 1
  // — those go to scalar_zero / scalar_one singletons and have their
  // own equality lock-ins below.
  for (int v : {-1, -2, -7, -100, 2, 3, 17, 1000}) {
    auto lhs = v * x;
    auto rhs = make_scalar_constant(v) * x;
    EXPECT_EQ(lhs, rhs) << "Construction-path mismatch at value v = " << v
                        << "; lhs = " << to_string(lhs)
                        << "; rhs = " << to_string(rhs);
  }
}

TEST(CoreBugFix, Issue184_MakeScalarConstantZeroIsSingleton) {
  // make_scalar_constant(0) currently builds scalar_constant{0} as a
  // distinct node; the int(0) path returns the scalar_zero singleton.
  // Canonicalise both to the singleton.
  EXPECT_EQ(make_scalar_constant(0), get_scalar_zero());
}

TEST(CoreBugFix, Issue184_MakeScalarConstantOneIsSingleton) {
  EXPECT_EQ(make_scalar_constant(1), get_scalar_one());
}

TEST(CoreBugFix, Issue184_MakeScalarConstantNegOneMatchesNegSingleton) {
  EXPECT_EQ(make_scalar_constant(-1), -get_scalar_one());
}

TEST(CoreBugFix, Issue184_MakeScalarConstantNegativeMatchesNegated) {
  // make_scalar_constant(-2) must equal -make_scalar_constant(2)
  // regardless of which node shape is chosen as canonical — what
  // matters is that the two construction paths converge.
  EXPECT_EQ(make_scalar_constant(-2), -make_scalar_constant(2));
}

TEST(CoreBugFix, Issue184_MixedPathCoefficientsMergeInNAryTree) {
  // 2*x + make_scalar_constant(2)*x should collapse to 4*x via the
  // n_ary_tree symbol_map merge — proving the two construction paths
  // produce the same key in the underlying map. Pre-fix this stored
  // two separate entries and failed to fold.
  auto [x] = make_scalar_variable("x");
  auto e = 2 * x + make_scalar_constant(2) * x;
  EXPECT_EQ(e, 4 * x);
}

TEST(CoreBugFix, Issue184_DoublePathAndConstantPathMatch) {
  // Same canonical-form rule must apply to floating-point literals.
  auto [x] = make_scalar_variable("x");
  for (double v : {-1.5, 0.25, -7.3, 100.0}) {
    auto lhs = v * x;
    auto rhs = make_scalar_constant(v) * x;
    EXPECT_EQ(lhs, rhs) << "Construction-path mismatch at value v = " << v;
  }
}

// #284/#314 — the "const coeff/exponent → hash == inner.hash" invariant
// must also hold for the scalar_one/scalar_zero singletons. The hash
// checks now use the singleton-aware is_scalar_constant; a bare
// is_same<scalar_constant> missed scalar_one (built via make_expression to
// bypass the pow(A,1)->A / 1*A->A folds). scalar_negative(constant) is a
// distinct expression (not scalar_constant{-k}) and is correctly hashed
// via combine — intentionally NOT covered here.
TEST(TensorConstHashInvariant, PowScalarOneSingletonExponentHashesLikeBase) {
  auto A = make_expression<tensor>("A", 3, 2);
  auto p = make_expression<tensor_pow>(A, get_scalar_one());
  EXPECT_EQ(p.get().hash_value(), A.get().hash_value());
}

TEST(TensorConstHashInvariant,
     ScalarMulScalarOneSingletonCoeffHashesLikeInner) {
  auto A = make_expression<tensor>("A", 3, 2);
  auto m = make_expression<tensor_scalar_mul>(get_scalar_one(), A);
  EXPECT_EQ(m.get().hash_value(), A.get().hash_value());
}

// #93 — a tensor_mul's space() must survive copy reconstruction
// (tensor_add did this; mul dropped it).
TEST(CoreBugFix, TensorMulCopyPreservesSpaceAnnotation) {
  auto A = make_expression<tensor>("A", 3, 2);
  auto B = make_expression<tensor>("B", 3, 2);
  auto prod = A * B;
  ASSERT_TRUE(is_same<tensor_mul>(prod));
  prod.data()->set_space({Skew{}, AnyTraceTag{}});

  auto copy = make_expression<tensor_mul>(prod.get<tensor_mul>());
  ASSERT_TRUE(copy.get().space().has_value())
      << "tensor_mul copy ctor dropped space() (#93)";
  EXPECT_TRUE(std::holds_alternative<Skew>(copy.get().space()->perm));
}

// #266 — inner_product's hash must include its contraction indices; two
// products differing only in their contraction sequences must hash
// differently (the default binary_op hash ignored them, causing cache
// aliasing and structural lock-ins that couldn't see the difference).
TEST(InnerProductHash, ContractionIndicesAffectHash) {
  auto A = make_expression<tensor>("A", 3, 2);
  auto B = make_expression<tensor>("B", 3, 2);
  auto e1 = inner_product(A, sequence{2}, B, sequence{1});
  auto e2 = inner_product(A, sequence{1}, B, sequence{2});
  EXPECT_NE(e1.get().hash_value(), e2.get().hash_value());
}

// #339 — n_ary equality must compare the coefficient. The coefficient-blind
// hash stays (like-term map keying), but == is semantic identity.
TEST(NAryCoeffEquality, AddCoefficientDistinguishes) {
  auto [x] = make_scalar_variable("x");
  EXPECT_FALSE(*(x + 2.0) == *(x + 5.0));
  EXPECT_FALSE(*(2.0 * x) == *(3.0 * x));
  EXPECT_FALSE(*sin(x + 2.0) == *sin(x + 5.0));
  EXPECT_TRUE(*(x + 2.0) == *(2.0 + x));
  EXPECT_TRUE(*(2.0 * x) == *(x * 2.0));
}

TEST(NAryCoeffEquality, NoFalseIdentityFolds) {
  auto [x, y] = make_scalar_variable("x", "y");
  scalar_evaluator<double> ev;
  ev.set(x, 1.0);
  EXPECT_DOUBLE_EQ(ev.apply((x + 2.0) * (x + 5.0)), 18.0);
  EXPECT_DOUBLE_EQ(ev.apply((x + 2.0) + (-(x + 5.0))), -3.0);
  EXPECT_NE(to_string(sin(x + 2.0) + sin(x + 5.0)), "2*sin(5+x)");
  EXPECT_NE(to_string(eq(2.0 * x, 3.0 * x)), "1");
  // substitution must not match a subtree differing only in coefficient
  EXPECT_EQ(to_string(substitute(x + 5.0, x + 2.0, y)), to_string(x + 5.0));
}

TEST(NAryCoeffEquality, PrinterKeepsDistinctChildren) {
  auto [x] = make_scalar_variable("x");
  // both factors share the coefficient-blind hash; print must keep both
  auto m = (x + 2.0) * (x + 5.0);
  auto const s = to_string(m);
  EXPECT_NE(s.find("2+x"), std::string::npos);
  EXPECT_NE(s.find("5+x"), std::string::npos);
}

TEST(NAryCoeffEquality, LikeTermMergesPreserved) {
  auto [x, y] = make_scalar_variable("x", "y");
  EXPECT_EQ(to_string(2.0 * x + 3.0 * x), "5*x");
  EXPECT_EQ(to_string(x + 2.0 * x), "3*x");
  EXPECT_EQ(to_string(((x + y) + x) + x), "3*x+y"); // #347
  EXPECT_EQ(to_string((y + 2.0 * x) + 3.0 * x), "5*x+y");
  scalar_evaluator<double> ev;
  ev.set(x, 1.0);
  ev.set(y, 10.0);
  EXPECT_DOUBLE_EQ(ev.apply(((x + y) + x) + x), 13.0);
}

TEST(NAryCoeffEquality, HalfCoefficientsMerge) {
  auto [x] = make_scalar_variable("x");
  // (c1*T)+(c2*T) with c1+c2 == 1 collapses back to T
  EXPECT_EQ(to_string(0.5 * x + 0.5 * x), "x");
  // and with c1+c2 == 0 to zero
  EXPECT_EQ(to_string(2.0 * x + (-2.0) * x), "0");
}

// Review findings on #339 (PR #386): ordering/equality consistency, zero
// children, reverse-direction like-term probe, stale cached hashes.
TEST(NAryCoeffEquality, IntAndDoubleCoefficientSpellingsAreEquivalent) {
  auto [x, y] = make_scalar_variable("x", "y");
  auto a = (x + y) + x; // int-coefficient 2*x
  auto c = 2.0 * x + y; // double-coefficient 2*x
  EXPECT_TRUE(*a == *c);
  EXPECT_FALSE(*a < *c); // < equivalence must match ==
  EXPECT_FALSE(*c < *a);
  EXPECT_EQ(to_string(a - c), "0");
}

TEST(NAryCoeffEquality, CancellationLeavesNoZeroChild) {
  auto [A, B] = make_tensor_variable(std::tuple{"A", std::size_t{3}, 2},
                                     std::tuple{"B", std::size_t{3}, 2});
  auto e = (2.0 * trace(A) + trace(B)) + (-2.0) * trace(A);
  EXPECT_EQ(to_string(e), to_string(trace(B)));
  EXPECT_TRUE(*e == *trace(B));
}

TEST(NAryCoeffEquality, ReverseDirectionLikeTermMerge) {
  auto [x, y] = make_scalar_variable("x", "y");
  EXPECT_EQ(to_string((x + y) + 2.0 * x), "3*x+y");
  auto [A, B] = make_tensor_variable(std::tuple{"A", std::size_t{3}, 2},
                                     std::tuple{"B", std::size_t{3}, 2});
  auto e = (trace(A) + trace(B)) + 2.0 * trace(A);
  EXPECT_EQ(to_string(e), to_string(3.0 * trace(A) + trace(B)));
}

TEST(NAryCoeffEquality, MutatedCopyDropsStaleCachedHash) {
  auto [x, y] = make_scalar_variable("x", "y");
  auto a = x + y;
  (void)a.get().hash_value(); // force the cache before merging
  auto b = a + x;             // 2*x+y built by mutating a copy of a
  EXPECT_TRUE(*b == *(2.0 * x + y));
  EXPECT_EQ(to_string(sin(b) - sin(2.0 * x + y)), "0");
}

// Round-2 review on #339/#340: remaining stale-hash/collapse holes in the
// sub, Pythagorean, and t2s wrapper-cancel paths; pow-add fallback throw;
// mul-coefficient default in the sub dispatcher.
TEST(RoundTwoReview, SubCancelInvalidatesAndCollapses) {
  auto [x, y, z] = make_scalar_variable("x", "y", "z");
  auto a = x + y + z;
  (void)a.get().hash_value();
  auto b = a - x;
  EXPECT_TRUE(*b == *(y + z));
  EXPECT_EQ(to_string(sin(b) - sin(y + z)), "0");
}

TEST(RoundTwoReview, PythagoreanBranchHygiene) {
  auto [x, y] = make_scalar_variable("x", "y");
  auto p = pow(cos(x), 2.0) + y;
  (void)p.get().hash_value();
  auto q = p + pow(sin(x), 2.0); // 1 + y
  EXPECT_TRUE(*q == *(y + 1.0));
  auto r = (pow(cos(x), 2.0) + y + (-1.0)) + pow(sin(x), 2.0); // y
  EXPECT_TRUE(*r == *y);
}

TEST(RoundTwoReview, PowAddFallbackMerges) {
  auto [x, y] = make_scalar_variable("x", "y");
  expression_holder<scalar_expression> e;
  EXPECT_NO_THROW(e = (pow(x, 2.0) + y) + pow(x, 2.0));
  EXPECT_TRUE(*e == *(2.0 * pow(x, 2.0) + y));
}

TEST(RoundTwoReview, MulCoefficientDefaultIsOne) {
  auto [x, y] = make_scalar_variable("x", "y");
  auto r = (x * y) * pow(x, -1.0); // degenerate mul{y}
  scalar_evaluator<double> ev;
  ev.set(x, 2.0);
  ev.set(y, 3.0);
  EXPECT_DOUBLE_EQ(ev.apply(r - y), 0.0); // was -3
  EXPECT_DOUBLE_EQ(ev.apply(r + y), 6.0);
}

TEST(RoundTwoReview, T2sWrapperCancelCollapses) {
  auto [A] = make_tensor_variable(std::tuple{"A", std::size_t{3}, 2});
  auto [a] = make_scalar_variable("a");
  auto w = [](auto e) {
    return make_expression<tensor_to_scalar_scalar_wrapper>(e);
  };
  auto e1 = (trace(A) + w(a)) + w(-a);
  EXPECT_TRUE(*e1 == *trace(A));
  EXPECT_FALSE(is_same<tensor_to_scalar_add>(e1));
  auto e2 = (trace(A) + w(a)) - w(a);
  EXPECT_TRUE(*e2 == *trace(A));
}
// #340 — raw hash_value() comparisons replaced by deep equality / explicit
// like-term folds. hash(c*X)==hash(X) and hash(pow(X,c))==hash(X) stay by
// design; they must never merge without a deep check.
TEST(HashIdentitySweep, TensorAddNoFalseMerges) {
  auto [X] = make_tensor_variable(std::tuple{"X", 3, 2});
  EXPECT_EQ(to_string(X + pow(X, 2)), "X+pow(X,2)");
  EXPECT_EQ(to_string(X + 2.0 * X), "3*X");
  EXPECT_EQ(to_string(2.0 * X + 3.0 * X), "5*X");
  EXPECT_NE(to_string(X + (-pow(X, 2))), "0{2}");
  EXPECT_NE(to_string((-X) + 2.0 * pow(X, 2)), "pow(X,2)");
}

TEST(HashIdentitySweep, TensorMulNoFalsePow) {
  auto [X] = make_tensor_variable(std::tuple{"X", 3, 2});
  // X*(2X) and (2X)*X: the scalar factor must survive
  EXPECT_EQ(to_string(X * (2.0 * X)).find("pow(2*X"), std::string::npos);
  auto s1 = to_string(X * (2.0 * X));
  auto s2 = to_string((2.0 * X) * X);
  EXPECT_NE(s1.find("2"), std::string::npos);
  EXPECT_NE(s2.find("2"), std::string::npos);
  // exact same operand still folds to pow
  EXPECT_EQ(to_string(X * X), "pow(X,2)");
}

TEST(HashIdentitySweep, ProjectorMergeRequiresSameArgument) {
  auto [X] = make_tensor_variable(std::tuple{"X", 3, 2});
  auto e = vol(pow(X, 2)) + dev(X);
  EXPECT_NE(to_string(e), "sym(pow(X,2))");
  EXPECT_NE(to_string(e), "sym(X)");
  // same argument keeps merging
  EXPECT_EQ(to_string(vol(X) + dev(X)), "sym(X)");
  EXPECT_EQ(to_string(sym(X) + skew(X)), "X");
}

TEST(HashIdentitySweep, ScalarSubMaxMinDeepEquality) {
  auto [x] = make_scalar_variable("x");
  EXPECT_NE(to_string(sin(x + 2.0) - sin(x + 5.0)), "0");
  EXPECT_NE(to_string(max(2.0 * x, 3.0 * x)), "2*x");
  EXPECT_NE(to_string(min(2.0 * x, 3.0 * x)), "2*x");
  // exact duplicates still fold
  EXPECT_EQ(to_string(max(x, x)), "x");
  EXPECT_EQ(to_string(sin(x + 2.0) - sin(x + 2.0)), "0");
}

TEST(HashIdentitySweep, TensorAddSpaceJoinSurvivesMerge) {
  auto [A] = make_tensor_variable(std::tuple{"A", 3, 2});
  auto [B] = make_tensor_variable(std::tuple{"B", 3, 2});
  auto sa = sym(A);
  auto sb = sym(B);
  EXPECT_TRUE(is_symmetric(sa + sb));
  // like-term merge path keeps the join
  EXPECT_TRUE(is_symmetric((sa + sb) + 2.0 * sa));
}

// Review on #340: tensor-side zero-child filter, degenerate collapse, and
// the reverse-direction like-term probe.
TEST(HashIdentitySweep, TensorCancellationAndReverseProbe) {
  auto [X, Y] = make_tensor_variable(std::tuple{"X", std::size_t{3}, 2},
                                     std::tuple{"Y", std::size_t{3}, 2});
  auto e = (2.0 * X + Y) + (-2.0) * X;
  EXPECT_EQ(to_string(e), "Y");
  EXPECT_TRUE(*e == *Y);
  EXPECT_EQ(to_string((X + Y) + 2.0 * X), "3*X+Y"); // reverse probe
}

// Round-5 review: whole-stack cross-check + randomized property sweep.
// Remaining raw-hash identity holes, a constant-dropping add path, the
// rank-lexicographic sign test in sub, and fraction-printing defects.

// Sweep bug A: constant + (coeff+x) silently dropped the constant when the
// coefficients cancelled (returned rhs with its old coeff intact).
TEST(RoundFiveReview, ConstantPlusAddCancellingCoeff) {
  auto [x, y] = make_scalar_variable("x", "y");
  auto e = 5.0 + (x - 5.0);
  EXPECT_TRUE(*e == *x);
  auto e2 = 5.0 + (x + y - 5.0);
  EXPECT_TRUE(*e2 == *(x + y));
  auto e3 = get_scalar_one() + (x - 1.0);
  EXPECT_TRUE(*e3 == *x);
}

// R5-6: mul+symbol / symbol+mul used a raw map find; hash(x+2)==hash(x)
// falsely folded 2*(x+2) + x into 3*(x+2).
TEST(RoundFiveReview, MulAddNoAliasedCoeffFold) {
  auto [x, z] = make_scalar_variable("x", "z");
  scalar_evaluator<double> ev;
  ev.set(x, 1.0);
  EXPECT_DOUBLE_EQ(ev.apply(2.0 * (x + 2.0) + x), 7.0); // was 9
  EXPECT_DOUBLE_EQ(ev.apply(x + 2.0 * (x + 2.0)), 7.0); // was 9
  EXPECT_EQ(to_string(z + (-1.0) * z), "0");            // was 0*z
}

// R5-3: (-x) + (y - x) hit the no-duplicates assert in push_back.
TEST(RoundFiveReview, NegativePlusAddContainingSameNegative) {
  auto [x, y] = make_scalar_variable("x", "y");
  expression_holder<scalar_expression> e;
  EXPECT_NO_THROW(e = (-x) + (y - x));
  scalar_evaluator<double> ev;
  ev.set(x, 1.0);
  ev.set(y, 5.0);
  EXPECT_DOUBLE_EQ(ev.apply(e), 3.0);
}

// R5-1 + R5-2/sweep bug B: c1*expr - c2*expr merged on raw hash equality
// (falsely matching different children), and classified the sign of the
// result with rank-lexicographic operator< instead of numeric_less.
TEST(RoundFiveReview, MulSubDeepEqualityAndSign) {
  auto [x, y] = make_scalar_variable("x", "y");
  scalar_evaluator<double> ev;
  ev.set(x, 1.0);
  ev.set(y, 2.0);
  EXPECT_DOUBLE_EQ(ev.apply(2.0 * (x + 2.0) - 5.0 * (x + 9.0)), -44.0);
  EXPECT_DOUBLE_EQ(ev.apply(2.0 * y - 2.5 * y), -1.0); // was +1
  EXPECT_DOUBLE_EQ(ev.apply(2.5 * y - 2.0 * y), 1.0);
}

// R5-1 latent + R5-2 in symbol - c*expr: same two defects.
TEST(RoundFiveReview, SymbolMinusScaledAddNoFalseMerge) {
  auto [x] = make_scalar_variable("x");
  scalar_evaluator<double> ev;
  ev.set(x, 1.0);
  EXPECT_DOUBLE_EQ(ev.apply(x - 5.0 * (x + 2.0)), -14.0); // was -12
  EXPECT_EQ(to_string(x - 2.0 * x), "-x");
  EXPECT_DOUBLE_EQ(ev.apply(x - 2.5 * x), -1.5); // was +1.5
}

// Sweep bug C companion (scalar side): add + (-aliased_child) erased the
// wrong map entry because hash(sin(y+2))==hash(sin(y+5)).
TEST(RoundFiveReview, AddMinusAliasedFunctionChild) {
  auto [x, y] = make_scalar_variable("x", "y");
  auto e = (x + sin(y + 2.0)) + (-sin(y + 5.0));
  EXPECT_NE(to_string(e), "x");
  scalar_evaluator<double> ev;
  ev.set(x, 0.0);
  ev.set(y, 0.0);
  EXPECT_DOUBLE_EQ(ev.apply(e), std::sin(2.0) - std::sin(5.0));
}

// R5-4: tensor pow/mul folds compared raw hashes; hash(c*T)==hash(T) and
// hash(pow(T,c))==hash(T) made A*pow(2A,2) fold to pow(A,3).
TEST(RoundFiveReview, TensorPowMulDeepBaseComparison) {
  auto [A] = make_tensor_variable(std::tuple{"A", std::size_t{3}, 2});
  EXPECT_NE(to_string(A * pow(2.0 * A, 2)), "pow(A,3)");
  EXPECT_NE(to_string(pow(2.0 * A, 2) * A), "pow(A,3)");
  EXPECT_NE(to_string(pow(A, 2) * pow(2.0 * A, 3)), "pow(A,5)");
  EXPECT_EQ(to_string(A * pow(A, 2)), "pow(A,3)");
  EXPECT_EQ(to_string(pow(A, 2) * A), "pow(A,3)");
  EXPECT_EQ(to_string(pow(A, 2) * pow(A, 3)), "pow(A,5)");
}

// R5-5: fraction printing — trailing "*" before "/", glued denominator
// factors, missing "1" numerator, and rank-lexicographic classification of
// negative double exponents.
TEST(RoundFiveReview, ScalarFractionPrinting) {
  auto [x, y, z] = make_scalar_variable("x", "y", "z");
  EXPECT_EQ(to_string(2.0 * pow(x, -1.0)), "2/x"); // was "2*/x"
  auto s = to_string(z * pow(x, -2.0) * pow(y, -1.0));
  EXPECT_TRUE(s == "z/(pow(x,2)*y)" || s == "z/(y*pow(x,2))") << s;
  auto s2 = to_string(pow(x, -2.0) * pow(y, -1.0));
  EXPECT_TRUE(s2 == "1/(pow(x,2)*y)" || s2 == "1/(y*pow(x,2))") << s2;
  EXPECT_EQ(to_string(y * pow(x, -2.5)), "y/pow(x,2.5)");
}

TEST(RoundFiveReview, T2sFractionPrinting) {
  auto [A] = make_tensor_variable(std::tuple{"A", std::size_t{3}, 2});
  auto s = to_string(pow(trace(A), -1.0) * pow(det(A), -1.0));
  EXPECT_NE(s.find("1/"), std::string::npos) << s;
  EXPECT_EQ(s.find("*/"), std::string::npos) << s;
}

// Round-5 differential fuzzer: pow(-b, p) extracted the sign for every
// exponent; (-b)^2 = b^2, and for symbolic p no extraction is valid.
TEST(RoundFiveReview, PowNegativeBaseParity) {
  auto [x, y] = make_scalar_variable("x", "y");
  scalar_evaluator<double> ev;
  ev.set(x, 2.5);
  EXPECT_DOUBLE_EQ(ev.apply(pow(-x, 2.0)), 6.25); // was -6.25
  EXPECT_DOUBLE_EQ(ev.apply(pow(-x, 3.0)), -15.625);
  EXPECT_EQ(to_string(pow(-x, 2.0)), "pow(x,2)");
  EXPECT_EQ(to_string(pow(-x, 3.0)), "-pow(x,3)");
  EXPECT_EQ(to_string(pow(-x, y)), "pow(-x,y)");
}

// Round-5 differential fuzzer: exact-duplicate children reached raw
// push_back through the sub default and two mul fallbacks and hit the
// no-duplicates assert.
TEST(RoundFiveReview, DuplicateChildMergesInsteadOfThrow) {
  auto [x, z] = make_scalar_variable("x", "z");
  scalar_evaluator<double> ev;
  ev.set(x, 1.0);
  ev.set(z, 2.0);
  expression_holder<scalar_expression> e;
  EXPECT_NO_THROW(e = z - (-1.0) * z);
  EXPECT_EQ(to_string(e), "2*z");
  EXPECT_NO_THROW(e = sin(x) * (-(0.5 * sin(x) * z)));
  EXPECT_DOUBLE_EQ(ev.apply(e), -std::sin(1.0) * std::sin(1.0));
  auto A = sin(z) * sin(x + 1.0);
  auto B = sin(z) * sin(x + 2.0);
  EXPECT_NO_THROW(e = A * B); // shared sin(z) factor
  EXPECT_DOUBLE_EQ(ev.apply(e), std::sin(2.0) * std::sin(2.0) * std::sin(2.0) *
                                    std::sin(3.0));
}

// Sweep bug C: hash(w(a+3))==hash(w(a+1)) — cancelling against a negative
// wrapper erased the aliased child and lost both constants.
TEST(RoundFiveReview, T2sAliasedWrapperCancellation) {
  auto [A] = make_tensor_variable(std::tuple{"A", std::size_t{3}, 2});
  auto [a] = make_scalar_variable("a");
  auto w = [](auto e) {
    return make_expression<tensor_to_scalar_scalar_wrapper>(e);
  };
  auto e = (trace(A) + w(a + 3.0)) + (-w(a + 1.0));
  EXPECT_EQ(to_string(e), "2+tr(A)");
  auto e2 = (trace(A) + w(a + 3.0)) - w(a + 1.0);
  EXPECT_NE(to_string(e2), "tr(A)"); // value must not be silently lost
}

// Round-6 review: mirrors the round-5 fixes stopped short of.

// R6-1: the value==0 degenerate collapse was missing from the
// (add ± constant/one/negative) mirror sites — (x+5)-5 stayed a
// single-child add that printed "x" but did not compare equal to x.
TEST(RoundSixReview, AddSubConstantCollapseMirrors) {
  auto [x] = make_scalar_variable("x");
  auto e1 = (x + 5.0) - 5.0;
  EXPECT_TRUE(*e1 == *x);
  EXPECT_FALSE(is_same<scalar_add>(e1));
  auto e2 = (x - 5.0) + 5.0;
  EXPECT_TRUE(*e2 == *x);
  auto e3 = (x - 1.0) + get_scalar_one();
  EXPECT_TRUE(*e3 == *x);
  auto neg5 =
      make_expression<scalar_negative>(make_expression<scalar_constant>(5));
  auto e4 = (x + 5.0) + neg5;
  EXPECT_TRUE(*e4 == *x);
  EXPECT_EQ(to_string(x - ((x + 5.0) - 5.0)), "0");
  EXPECT_EQ(to_string(((x + 5.0) - 5.0) * pow(x, -1.0)), "1");
}

// R6-2: a symbolic t2s add coefficient (wrapper(s)) was silently deleted
// by every get_coefficient + coeff().free() fold site.
TEST(RoundSixReview, T2sSymbolicCoeffSurvivesConstantFold) {
  auto [A] = make_tensor_variable(std::tuple{"A", std::size_t{3}, 2});
  auto [s] = make_scalar_variable("s");
  auto w = [](auto e) {
    return make_expression<tensor_to_scalar_scalar_wrapper>(e);
  };
  auto c5 = make_expression<scalar_constant>(5);
  auto c2 = make_expression<scalar_constant>(2);
  auto base = w(s) - (trace(A) + det(A)); // coeff = wrapper(s)
  auto has_s = [](auto const &e) {
    return to_string(e).find('s') != std::string::npos;
  };
  EXPECT_TRUE(has_s(base));
  EXPECT_TRUE(has_s(base + w(c5))); // was "5-tr(A)-det(A)"
  EXPECT_TRUE(has_s(base - w(c2))); // was "-2-tr(A)-det(A)"
  EXPECT_TRUE(has_s(base + (-w(c5))));
  auto t2s_one = make_expression<tensor_to_scalar_one>();
  EXPECT_TRUE(has_s(t2s_one + base)); // was "1-tr(A)-det(A)"
}

// R6-3: the t2s sub-side wrapper merge pushed a numeric result as a child
// instead of folding it into the coeff — equal values compared unequal.
TEST(RoundSixReview, T2sSubWrapperMergeFoldsToCoeff) {
  auto [A] = make_tensor_variable(std::tuple{"A", std::size_t{3}, 2});
  auto [a] = make_scalar_variable("a");
  auto w = [](auto e) {
    return make_expression<tensor_to_scalar_scalar_wrapper>(e);
  };
  auto c2 = make_expression<scalar_constant>(2);
  auto e1 = (trace(A) + w(a + 3.0)) - w(a + 1.0);
  auto e2 = trace(A) + w(c2);
  auto e3 = (trace(A) + w(a + 3.0)) + (-w(a + 1.0));
  EXPECT_TRUE(*e1 == *e2);
  EXPECT_TRUE(*e1 == *e3);
  EXPECT_EQ(to_string(e1 - w(c2)), "tr(A)");
}

// Round-7 review: negative(zero) minting, negative-wrapper duals, and
// missing negation-pair cancellation in add merges.

// R7-1: -e1 - e2 built a raw negative node; a fully-cancelling sum minted
// negative(zero), defeating every zero-singleton filter.
TEST(RoundSevenReview, NegativeLhsSubNormalizesZero) {
  auto [x, y, z] = make_scalar_variable("x", "y", "z");
  auto e1 = (-x) - (-x);
  EXPECT_TRUE(is_same<scalar_zero>(e1)) << to_string(e1);
  auto e2 = (z - y) - (x - y);
  EXPECT_TRUE(*e2 == *(z - x)) << to_string(e2);
  auto [A] = make_tensor_variable(std::tuple{"A", std::size_t{3}, 2});
  auto t = (-trace(A)) - (-trace(A));
  EXPECT_TRUE(is_same<tensor_to_scalar_zero>(t)) << to_string(t);
}

// R7-2: -w(a) and w(-a) are the same value; neg_fn now normalizes so both
// build routes agree and round-trip cancellation works.
TEST(RoundSevenReview, T2sNegativeWrapperNormalized) {
  auto [A] = make_tensor_variable(std::tuple{"A", std::size_t{3}, 2});
  auto [a] = make_scalar_variable("a");
  auto w = [](auto e) {
    return make_expression<tensor_to_scalar_scalar_wrapper>(e);
  };
  auto base = trace(A) + det(A);
  auto e1 = base + (-w(a));
  auto e2 = base - w(a);
  EXPECT_TRUE(*e1 == *e2) << to_string(e1) << " vs " << to_string(e2);
  auto e3 = (base - w(a)) + w(a);
  EXPECT_TRUE(*e3 == *base) << to_string(e3);
}

// R7-4: negation pairs share no hash, so (3-x)+x and (3+x)+(1-x) never
// cancelled; find_like now retries with the exact negation.
TEST(RoundSevenReview, AddCancelsAgainstNegativeChild) {
  auto [x, y] = make_scalar_variable("x", "y");
  EXPECT_EQ(to_string((3.0 - x) + x), "3");
  EXPECT_EQ(to_string((3.0 + x) + (1.0 - x)), "4");
  auto e = (y - x) + (x + 2.0);
  EXPECT_TRUE(*e == *(y + 2.0)) << to_string(e);
  // t2s merged-wrapper interiors reduce through the same path
  auto [A] = make_tensor_variable(std::tuple{"A", std::size_t{3}, 2});
  auto [a] = make_scalar_variable("a");
  auto w = [](auto ex) {
    return make_expression<tensor_to_scalar_scalar_wrapper>(ex);
  };
  auto f = (trace(A) + w(a + 3.0)) + w(1.0 - a);
  auto c4 = make_expression<scalar_constant>(4);
  EXPECT_TRUE(*f == *(trace(A) + w(c4))) << to_string(f);
}

// Round-8 review: regressions from the round-7 negation probe.

// R8-1: merge_add consumed the same rhs child twice when the lhs held an
// exact {t,-t} pair — 5x neg-matched -(5x), then -(5x) direct-matched it
// again, turning y-5x into y-10x. Also kills the enabler: signed inserts
// keep {t,-t} from coexisting at all.
TEST(RoundEightReview, MergeAddNoDoubleConsume) {
  auto [x, y] = make_scalar_variable("x", "y");
  auto f = (2.0 * x + (-(5.0 * x))) + 3.0 * x;
  EXPECT_TRUE(is_same<scalar_zero>(f)) << to_string(f);
  scalar_evaluator<double> ev;
  ev.set(x, 1.0);
  ev.set(y, 0.0);
  EXPECT_DOUBLE_EQ(ev.apply(f + (y - 5.0 * x)), -5.0);
  // an add manually holding the exact pair must still merge correctly
  auto pair_add = make_expression<scalar_add>();
  auto &pa = pair_add.get<scalar_add>();
  pa.push_back(5.0 * x);
  pa.push_back(-(5.0 * x));
  auto g = pair_add + (y - 5.0 * x);
  EXPECT_DOUBLE_EQ(ev.apply(g), -5.0) << to_string(g); // was -10
}

// R8-2: the tensor add-merge got round-7's cancellation power without the
// zero filter — (A+B)+(C-A) held a literal 0{2} child.
TEST(RoundEightReview, TensorMergeAddZeroFiltered) {
  auto [A, B, C] = make_tensor_variable(std::tuple{"A", std::size_t{3}, 2},
                                        std::tuple{"B", std::size_t{3}, 2},
                                        std::tuple{"C", std::size_t{3}, 2});
  auto e1 = (A + B) + (C - A);
  EXPECT_TRUE(*e1 == *(B + C)) << to_string(e1);
  EXPECT_EQ(to_string(e1).find("0{2}"), std::string::npos) << to_string(e1);
  auto e2 = e1 - (B + C);
  EXPECT_TRUE(is_same<tensor_zero>(e2)) << to_string(e2);
  // full cancellation collapses to the zero singleton, not an empty add
  auto e3 = (A + B) + ((-A) + (-B));
  EXPECT_TRUE(is_same<tensor_zero>(e3)) << to_string(e3);
}

// Round-9 review: nested adds from the negative fall-through, one
// unconverted tensor insert, and a literal zero coefficient.

// R9-1: add + (-t) with a non-cancelling t fell to get_default, which
// nested the whole lhs add as a single child; buried terms then defeated
// merge cancellation.
TEST(RoundNineReview, AddNegativeStaysFlat) {
  auto [x, y] = make_scalar_variable("x", "y");
  auto e = (x + 5.0 * y) + (-y);
  // flat children: the exact 5*y child stays reachable for cancellation
  auto e2 = e + (-(5.0 * y));
  EXPECT_TRUE(*e2 == *(x - y)) << to_string(e2);
  auto [A, B, C] = make_tensor_variable(std::tuple{"A", std::size_t{3}, 2},
                                        std::tuple{"B", std::size_t{3}, 2},
                                        std::tuple{"C", std::size_t{3}, 2});
  auto t = (A + B) + ((C - A) - B);
  EXPECT_TRUE(*t == *C) << to_string(t);
  EXPECT_TRUE(is_same<tensor_zero>(t - C)) << to_string(t - C);
}

// R9-2: the tensor n-ary fallback still plain-inserted the combined term;
// (5A-2A)+(-3A) held an exact {2A, -(2A)} pair instead of collapsing.
TEST(RoundNineReview, TensorCombinedInsertIsSigned) {
  auto [A] = make_tensor_variable(std::tuple{"A", std::size_t{3}, 2});
  auto e = (5.0 * A + (-(2.0 * A))) + (-3.0) * A;
  EXPECT_TRUE(is_same<tensor_zero>(e)) << to_string(e);
}

// R9-3: merge_add stored a cancelled coefficient as a literal zero —
// (2+x)+(y-2) printed "0+x+y" and compared unequal to x+y.
TEST(RoundNineReview, MergeAddDropsCancelledCoeff) {
  auto [x, y] = make_scalar_variable("x", "y");
  auto e = (2.0 + x) + (y - 2.0);
  EXPECT_TRUE(*e == *(x + y)) << to_string(e);
  auto [A] = make_tensor_variable(std::tuple{"A", std::size_t{3}, 2});
  auto t = (2.0 + trace(A)) + (det(A) - 2.0);
  EXPECT_TRUE(*t == *(trace(A) + det(A))) << to_string(t);
}

// Round-10 review: the tensor n-ary add fallback was the last insertion
// path without the exact-negation probe — ((A+B)-C)+C kept {C,-C}.
TEST(RoundTenReview, TensorAddCancelsExactNegativeChild) {
  auto [A, B, C] = make_tensor_variable(std::tuple{"A", std::size_t{3}, 2},
                                        std::tuple{"B", std::size_t{3}, 2},
                                        std::tuple{"C", std::size_t{3}, 2});
  auto e1 = ((A + B) - C) + C;
  EXPECT_TRUE(*e1 == *(A + B)) << to_string(e1);
  auto e2 = (A - C) + C;
  EXPECT_TRUE(*e2 == *A) << to_string(e2);
  auto e3 = ((A + B) - 2.0 * C) + 2.0 * C;
  EXPECT_TRUE(*e3 == *(A + B)) << to_string(e3);
  auto e4 = ((A + B) - trans(C)) + trans(C);
  EXPECT_TRUE(*e4 == *(A + B)) << to_string(e4);
}

// Round-11 review: tensor non-add + add nested the rhs add as one child
// (the swap-to-n-ary dispatch was guarded on a non-void mul_type), so
// A+(B+C) and (A+B)+C were different trees.
TEST(RoundElevenReview, TensorAddRhsFlattens) {
  auto [A, B, C] = make_tensor_variable(std::tuple{"A", std::size_t{3}, 2},
                                        std::tuple{"B", std::size_t{3}, 2},
                                        std::tuple{"C", std::size_t{3}, 2});
  EXPECT_TRUE(*(A + (B + C)) == *((A + B) + C));
  EXPECT_TRUE(*(2.0 * C + (A + B)) == *((A + B) + 2.0 * C));
  EXPECT_TRUE(*(trans(C) + (A + B)) == *((A + B) + trans(C)));
  EXPECT_TRUE(*((-C) + (A + B)) == *((A + B) - C));
  EXPECT_TRUE(is_same<tensor_zero>((A + (B + C)) - (A + B + C)));
  auto e = (A + (B + C)) + B;
  EXPECT_TRUE(*e == *(A + 2.0 * B + C)) << to_string(e);
}

// #344 — pow simplifier applied division-style rules to pow(a, -b) and
// pulled signs out of pow(-e, p) for every exponent. Locked via evaluation.
TEST(PowDivisionConfusion, NegativeBaseEvenOddSymbolic) {
  auto [x, y] = make_scalar_variable("x", "y");
  scalar_evaluator<double> ev;
  ev.set(x, 3.0);
  ev.set(y, 2.0);
  EXPECT_DOUBLE_EQ(ev.apply(pow(-x, 2.0)), 9.0);   // was −9
  EXPECT_DOUBLE_EQ(ev.apply(pow(-x, 3.0)), -27.0); // odd pull-out stays
  EXPECT_EQ(to_string(pow(-x, y)), "pow(-x,y)");   // symbolic: structural
}

TEST(PowDivisionConfusion, NegativeExponentIsNotDivision) {
  auto [x, y] = make_scalar_variable("x", "y");
  scalar_evaluator<double> ev;
  ev.set(x, 6.0);
  ev.set(y, 2.0);
  EXPECT_NEAR(ev.apply(pow(x, -x)), std::pow(6.0, -6.0), 1e-15);
  EXPECT_NEAR(ev.apply(pow(x, -y) * y), 2.0 * std::pow(6.0, -2.0), 1e-15);
  EXPECT_NEAR(ev.apply(pow(x * y, -y)), std::pow(12.0, -2.0), 1e-15);
  // exponent-addition cancellation still works
  EXPECT_EQ(to_string(pow(x, -1.0) * x), "1");
}

// Review on #344: the sign pull-out must recurse through pow() so nested
// bases keep canonicalizing.
TEST(PowDivisionConfusion, SignPullOutCanonicalizesNestedBase) {
  auto [x] = make_scalar_variable("x");
  EXPECT_EQ(to_string(pow(-pow(x, 2.0), 2.0)), "pow(x,4)");
  EXPECT_EQ(to_string(pow(-pow(x, 2.0), 3.0)), "-pow(x,6)");
  EXPECT_EQ(to_string(pow(-pow(x, 2.0), 2.0) - pow(x, 4.0)), "0");
}

// #342 — permute_indices_wrapper identity must include the permutation.
TEST(IndexSequenceIdentity, PermutationsDistinguish) {
  auto [T] = make_tensor_variable(std::tuple{"T", 3, 3});
  auto p1 = permute_indices(T, sequence{2, 1, 3});
  auto p2 = permute_indices(T, sequence{1, 3, 2});
  EXPECT_FALSE(*p1 == *p2);
  EXPECT_NE(p1.get().hash_value(), p2.get().hash_value());
  EXPECT_NE(to_string(p1 - p2), "0{3}");
  // identical permutation still cancels
  auto q = permute_indices(T, sequence{2, 1, 3});
  EXPECT_EQ(to_string(p1 - q), "0{3}");
}

// #343 — tensor_inner_product_to_scalar identity must include the
// contraction sequences (A:B is not A:B^T).
TEST(IndexSequenceIdentity, T2sContractionSequencesDistinguish) {
  auto [A, B] =
      make_tensor_variable(std::tuple{"A", 3, 2}, std::tuple{"B", 3, 2});
  auto ab = dot_product(A, sequence{1, 2}, B, sequence{1, 2});
  auto abt = dot_product(A, sequence{1, 2}, B, sequence{2, 1});
  EXPECT_FALSE(*ab == *abt);
  EXPECT_NE(ab.get().hash_value(), abt.get().hash_value());
  EXPECT_NE(to_string(ab - abt), "0");
  EXPECT_NE(to_string(ab + abt), to_string(2.0 * abt));
  // identical sequences still cancel
  auto ab2 = dot_product(A, sequence{1, 2}, B, sequence{1, 2});
  EXPECT_EQ(to_string(ab - ab2), "0");
}

// #346 — multiplying a function factor into a product already containing it
// must fold to a pow instead of throwing "duplicate child insertion".
TEST(MulDuplicateFactor, FunctionFactorIntoProduct) {
  auto [x, y] = make_scalar_variable("x", "y");
  scalar_evaluator<double> ev;
  ev.set(x, 2.0);
  ev.set(y, 3.0);
  expression_holder<scalar_expression> e;
  EXPECT_NO_THROW(e = sin(x) * (sin(x) * y));
  EXPECT_EQ(to_string(e), to_string(pow(sin(x), 2.0) * y));
  EXPECT_NEAR(ev.apply(e), std::sin(2.0) * std::sin(2.0) * 3.0, 1e-15);

  expression_holder<scalar_expression> e2;
  EXPECT_NO_THROW(e2 = log(x) * (log(x) * y));
  EXPECT_NEAR(ev.apply(e2), std::log(2.0) * std::log(2.0) * 3.0, 1e-15);

  // sin(x) and pow(sin(x),2) are distinct exact keys, so the product keeps
  // both factors (value-correct; pow-base folding is epic #379's scope)
  expression_holder<scalar_expression> e3;
  EXPECT_NO_THROW(e3 = sin(x) * (pow(sin(x), 2.0) * y));
  EXPECT_NEAR(ev.apply(e3), std::pow(std::sin(2.0), 3.0) * 3.0, 1e-12);
}

// Review on #346: the duplicate-factor fold must work in both operand
// orders and through the mul*mul factor chain.
TEST(MulDuplicateFactor, MirroredOrdersFold) {
  auto [x, y, z] = make_scalar_variable("x", "y", "z");
  scalar_evaluator<double> ev;
  ev.set(x, 2.0);
  ev.set(y, 3.0);
  ev.set(z, 5.0);
  expression_holder<scalar_expression> a, b, c;
  EXPECT_NO_THROW(a = (sin(x) * y) * sin(x));
  EXPECT_NEAR(ev.apply(a), std::sin(2.0) * std::sin(2.0) * 3.0, 1e-15);
  EXPECT_NO_THROW(b = (sin(x) * y) * (sin(x) * z));
  EXPECT_NEAR(ev.apply(b), std::sin(2.0) * std::sin(2.0) * 15.0, 1e-14);
  EXPECT_NO_THROW(c = (sin(x) * y) * (sin(x) * y));
  EXPECT_NEAR(ev.apply(c), std::pow(std::sin(2.0) * 3.0, 2.0), 1e-13);
}

// #345 — pow distributes/merges over products only for integer exponents.
TEST(PowDistributeGuard, FractionalExponentDoesNotDistribute) {
  auto [x, y] = make_scalar_variable("x", "y");
  scalar_evaluator<double> ev;
  ev.set(x, 1.0);
  ev.set(y, -2.0);
  auto e = pow(x * pow(y, 2.0), 0.5); // ((1)·4)^0.5 = 2, NOT y·sqrt(x) = −2
  EXPECT_DOUBLE_EQ(ev.apply(e), 2.0);
  // integer exponent still distributes
  EXPECT_EQ(to_string(pow(x * pow(y, 2.0), 3.0)), "pow(x,3)*pow(y,6)");
}

TEST(PowDistributeGuard, FractionalSameExponentDoesNotMerge) {
  auto [x, y] = make_scalar_variable("x", "y");
  EXPECT_NE(to_string(pow(x, 0.5) * pow(y, 0.5)), "pow(x*y,1/2)");
  // integer exponent still merges
  EXPECT_EQ(to_string(pow(x, 2.0) * pow(y, 2.0)), "pow(x*y,2)");
}

// Round-2 review on #345: pow-split canonical form and the mul producer.
TEST(RoundTwoReview, PowSplitCollapsesSingleChildMul) {
  auto [x, y] = make_scalar_variable("x", "y");
  auto e = pow(pow(x, 2.0) * y, 2.0); // was pow(mul{y},2)*pow(x,4)
  EXPECT_TRUE(*e == *(pow(x, 4.0) * pow(y, 2.0)));
  EXPECT_EQ(to_string(e - pow(x, 4.0) * pow(y, 2.0)), "0");
}

TEST(RoundTwoReview, MulPowEraseProducesCanonicalRemainder) {
  auto [x, y] = make_scalar_variable("x", "y");
  auto r = (x * y) * pow(x, -1.0); // must be the bare symbol y
  EXPECT_TRUE(*r == *y);
  EXPECT_TRUE(is_same<scalar>(r));
  EXPECT_EQ(to_string(sin(r) - sin(y)), "0"); // no stale hash either
}

// #349 — scalar_number int64 arithmetic must demote to double instead of
// wrapping (UB / silent corruption).
TEST(ScalarNumberOverflow, PowDoesNotWrap) {
  auto p = pow(make_scalar_constant(10), make_scalar_constant(30));
  scalar_evaluator<double> ev;
  EXPECT_NEAR(ev.apply(p), 1e30, 1e16); // was 5076944270305263616 (mod 2^64)
}

TEST(ScalarNumberOverflow, RationalAddLargeMagnitudes) {
  const auto big = std::int64_t{1} << 40;
  auto a = scalar_number(rational_t{big + 1, big});
  auto b = scalar_number(rational_t{big + 3, big + 2});
  auto s = a + b; // cross-products overflow int64; must not be UB
  double val = std::visit(
      [](auto const &v) -> double {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, double>)
          return v;
        else if constexpr (std::is_same_v<T, std::int64_t>)
          return static_cast<double>(v);
        else if constexpr (std::is_same_v<T, rational_t>)
          return static_cast<double>(v.num) / static_cast<double>(v.den);
        else
          return 0.0;
      },
      s.raw());
  EXPECT_NEAR(val, 2.0, 1e-9);
}

TEST(ScalarNumberOverflow, RationalDivByZeroIsInf) {
  auto q = scalar_number(1, 2) / scalar_number(std::int64_t{0});
  auto const *d = std::get_if<double>(&q.raw());
  ASSERT_NE(d, nullptr); // not a stored 1/0 rational
  EXPECT_TRUE(std::isinf(*d));
  EXPECT_GT(*d, 0.0);
}

TEST(ScalarNumberOverflow, ExactArithmeticUnchanged) {
  auto a = scalar_number(1, 3) + scalar_number(1, 6); // = 1/2 exact
  auto const *r = std::get_if<rational_t>(&a.raw());
  ASSERT_NE(r, nullptr);
  EXPECT_EQ(r->num, 1);
  EXPECT_EQ(r->den, 2);
  auto b = scalar_number(std::int64_t{2}) * scalar_number(std::int64_t{3});
  EXPECT_EQ(b, scalar_number(std::int64_t{6}));
}

// Review on #349: INT64_MIN reaches the rational cross-cancel via the
// int->rational promotion, which skips normalization.
TEST(ScalarNumberOverflow, Int64MinTimesRational) {
  constexpr auto mn = std::numeric_limits<std::int64_t>::min();
  auto p = scalar_number(mn) * scalar_number(1, 2); // was std::abs(mn) UB
  auto q = scalar_number(1, 2) * scalar_number(mn);
  auto d = scalar_number(mn) / scalar_number(1, 2);
  auto const *pd = std::get_if<double>(&p.raw());
  ASSERT_NE(pd, nullptr);
  EXPECT_NEAR(*pd, static_cast<double>(mn) / 2.0, 1e3);
  EXPECT_TRUE(std::get_if<double>(&q.raw()) != nullptr);
  EXPECT_TRUE(std::get_if<double>(&d.raw()) != nullptr);
}

// #361 — hash_combine(double) hashed via static_cast<size_t>: UB for
// negatives, and every fraction in (0,1) collided with 0.
TEST(HashCombineDouble, BitPatternNoTruncation) {
  std::size_t a = 0, b = 0, c = 0, d = 0, e = 0;
  hash_combine(a, 0.5);
  hash_combine(b, 0.9);
  EXPECT_NE(a, b);
  hash_combine(c, -2.5); // UB-free under -fsanitize=float-cast-overflow
  hash_combine(d, 0.0);
  hash_combine(e, -0.0);
  EXPECT_EQ(d, e); // ±0 normalize together
}

TEST(HashCombineDouble, NumericallyEqualConstantsHashEqual) {
  auto ci = make_scalar_constant(2);
  auto cd = make_expression<scalar_constant>(2.0);
  EXPECT_EQ(ci.get().hash_value(), cd.get().hash_value());
  EXPECT_EQ(to_string(ci * cd), "4"); // folding across alternatives intact
  // fractional constants distinct
  auto h1 = make_expression<scalar_constant>(0.5);
  auto h2 = make_expression<scalar_constant>(0.9);
  EXPECT_NE(h1.get().hash_value(), h2.get().hash_value());
}

// Review on #361: the int64 cast in the value-normalizing hash ran before
// its range guard - UB for NaN, inf, and huge doubles.
TEST(HashCombineDouble, HugeAndNonFiniteConstantsHashSafely) {
  auto big = make_expression<scalar_constant>(1e300);
  auto nan = make_expression<scalar_constant>(
      std::numeric_limits<double>::quiet_NaN());
  auto inf =
      make_expression<scalar_constant>(std::numeric_limits<double>::infinity());
  // must be UB-free under -fsanitize=float-cast-overflow (CI leg, #356)
  (void)big.get().hash_value();
  (void)nan.get().hash_value();
  (void)inf.get().hash_value();
  EXPECT_NE(big.get().hash_value(), inf.get().hash_value());
}

// #351 — rank-4 identity is major-symmetric only; the MinorMajor tag routed
// inv() through the symmetric Voigt path, evaluating inv(-I4) to -0.25 at
// component (0,1,0,1) instead of -1 (the inverse of -I4 is -I4).
TEST(Rank4IdentityTag, InvOfNegatedIdentity) {
  auto I4 = make_expression<identity_tensor>(std::size_t{3}, std::size_t{4});
  tensor_evaluator<double> ev;
  auto r = ev.apply(inv(-I4));
  // (0,1,0,1) flattens to ((0*3+1)*3+0)*3+1 = 10
  EXPECT_NEAR(r->raw_data()[10], -1.0, 1e-12);
  EXPECT_NEAR(r->raw_data()[0], -1.0, 1e-12); // (0,0,0,0)}

} // namespace numsim::cas

#endif // COREBUGFIXTEST_H
