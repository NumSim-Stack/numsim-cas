#ifndef COREBUGFIXTEST_H
#define COREBUGFIXTEST_H

#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"

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
  // combine to (-2*x) + y.
  EXPECT_NO_THROW({
    auto r = (-x + y) - x;
    (void)r;
  });
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
// scalar_evaluator::forward_values_to filters non-scalar keys
// ---------------------------------------------------------------------------

TEST(CoreBugFix, ForwardValuesToSkipsNonScalarKey) {
  // scalar_evaluator::set is templated on ExprBase, so callers could mistakenly
  // store a tensor symbol in the scalar evaluator's map. The forwarding loop
  // uses dynamic_pointer_cast<scalar_expression> to skip such keys instead of
  // force-casting them (which would be UB the next time the destination tried
  // to treat the holder as a scalar). Confirm the loop survives the bad key.
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

} // namespace numsim::cas

#endif // COREBUGFIXTEST_H
