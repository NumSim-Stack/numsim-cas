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

TEST(CoreBugFix, AddCompoundConstructionSmoke) {
  auto [x, y] = make_scalar_variable("x", "y");
  auto one = make_scalar_constant(1);
  EXPECT_NO_THROW({
    auto lhs = pow(cos(x), 2) + pow(sin(x), 2) + y;
    auto rhs = one + one + y;
    auto sum = lhs + rhs;
    (void)sum;
  });
}

} // namespace numsim::cas

#endif // COREBUGFIXTEST_H
