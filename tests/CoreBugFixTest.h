#ifndef COREBUGFIXTEST_H
#define COREBUGFIXTEST_H

#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"
#include <cmath>

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
  EXPECT_NEAR(ev.apply(d_tanh), 1.0 / (std::cosh(0.5) * std::cosh(0.5)),
              1e-12);
}

} // namespace numsim::cas

#endif // COREBUGFIXTEST_H
