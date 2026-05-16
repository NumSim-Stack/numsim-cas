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

TEST(CoreBugFix, SubSymbolDispatchSmoke) {
  // Exercises merge_or_insert on n_ary_sub_dispatch::dispatch(symbol):
  // lhs is an add expression, rhs matches one of its children, and the
  // combined entry is re-inserted via merge_or_insert.
  auto [x, y, z] = make_scalar_variable("x", "y", "z");
  EXPECT_NO_THROW({
    auto lhs = x + y + z;
    auto diff = lhs - x;
    (void)diff;
  });
}

TEST(CoreBugFix, NegativeSubAddDispatchSmoke) {
  // Exercises merge_or_insert on negative_sub_dispatch::dispatch(add):
  // lhs is a negative, rhs is an add — the inner expr is folded into the
  // rhs add via merge_or_insert before being wrapped in the outer negation.
  auto [x, y, z] = make_scalar_variable("x", "y", "z");
  EXPECT_NO_THROW({
    auto diff = -x - (y + z);
    (void)diff;
  });
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
