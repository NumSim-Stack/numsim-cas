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
// trace() simplification rules (issue #72).
// Existing: trace(0)=0, trace(I)=dim, trace(alpha*A)=alpha*trace(A),
// trace(A+B+...)=trace(A)+trace(B)+... .
// New: trace(-A)=-trace(A), trace(trans(A))=trace(A),
// trace(otimes(u,v))=dot_product(u,v).
// ---------------------------------------------------------------------------

TEST(CoreBugFix, TraceNegativeIsNegated) {
  // trace(-A) -> -trace(A)
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  EXPECT_EQ(trace(-A), -trace(A));
}

TEST(CoreBugFix, TraceTransIsSelf) {
  // trace(trans(A)) -> trace(A)
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  EXPECT_EQ(trace(trans(A)), trace(A));
}

TEST(CoreBugFix, TraceOuterProductIsDot) {
  // trace(u ⊗ v) -> dot_product(u, v) for rank-1 vectors.
  auto [u, v] =
      make_tensor_variable(std::tuple{"u", std::size_t{3}, std::size_t{1}},
                           std::tuple{"v", std::size_t{3}, std::size_t{1}});
  auto r = trace(otimes(u, v));
  EXPECT_EQ(r, dot_product(u, sequence{1}, v, sequence{1}));
}

// inv(alpha * A) -> (1/alpha) * inv(A) (issue #71)
// Scalar factor is lifted outside the inverse so the canonical form keeps
// the tensor inverse on the inner operand.
// ---------------------------------------------------------------------------

TEST(CoreBugFix, InvLiftsConstantScalarFactor) {
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto two = make_scalar_constant(2);
  auto r = inv(two * A);
  // Result should be a tensor_scalar_mul wrapping inv(A).
  ASSERT_TRUE(is_same<tensor_scalar_mul>(r));
  auto const &sm = r.get<tensor_scalar_mul>();
  ASSERT_TRUE(is_same<tensor_inv>(sm.expr_rhs()));
  EXPECT_EQ(sm.expr_rhs().get<tensor_inv>().expr(), A);
}

TEST(CoreBugFix, InvLiftsSymbolicScalarFactor) {
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto [s] = make_scalar_variable("s");
  auto r = inv(s * A);
  ASSERT_TRUE(is_same<tensor_scalar_mul>(r));
  EXPECT_TRUE(is_same<tensor_inv>(r.get<tensor_scalar_mul>().expr_rhs()));
}

TEST(CoreBugFix, InvLiftsThenRejectsSkewInner) {
  // inv(2 * skew(A)) in odd dim: the scalar lifts out (canonical form),
  // then the recursive inv(skew(A)) throws via contains_skew_factor.
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto two = make_scalar_constant(2);
  auto sA = skew(A);
  EXPECT_THROW(
      { [[maybe_unused]] auto r = inv(two * sA); }, invalid_expression_error);
}

TEST(CoreBugFix, InvLiftsNestedScalarFactor) {
  // inv(s * (t * A)) — nested tensor_scalar_mul. The recursive rule should
  // produce a canonical (s*t)-scaled inv(A) rather than a doubly-nested
  // tensor_scalar_mul.
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto [s, t] = make_scalar_variable("s", "t");
  auto r = inv(s * (t * A));
  // Result should be a tensor_scalar_mul whose tensor child is inv(A),
  // not another tensor_scalar_mul (the operator* on scalar*tensor_scalar_mul
  // collapses the nesting via mul_base::dispatch(tensor_scalar_mul)).
  ASSERT_TRUE(is_same<tensor_scalar_mul>(r));
  EXPECT_TRUE(is_same<tensor_inv>(r.get<tensor_scalar_mul>().expr_rhs()))
      << "Inner tensor should be tensor_inv directly (collapsed), got: "
      << to_string(r);
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
  if (!e.is_valid())
    return false;
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
  // Verify the skew child is specifically the operand we passed as skew —
  // not just "some child happens to be annotated." Find the child equal
  // to sA and assert ITS annotation is preserved; the other child (B)
  // must not be spuriously annotated as Skew.
  bool found_sA_with_skew = false;
  bool other_child_spuriously_skew = false;
  for (auto const &ch : mul.data()) {
    if (ch == sA) {
      EXPECT_TRUE(is_skew_annotated(ch))
          << "skew(A) child lost its Skew annotation on this build";
      found_sA_with_skew = is_skew_annotated(ch);
    } else if (is_skew_annotated(ch)) {
      other_child_spuriously_skew = true;
    }
  }
  EXPECT_TRUE(found_sA_with_skew) << "skew(A) child not found in tensor_mul";
  EXPECT_FALSE(other_child_spuriously_skew)
      << "non-skew child spuriously annotated as Skew";
}

// ---------------------------------------------------------------------------
// Tensor pow simplifications (issue #96) — extends the construction-time
// rules in tensor_std.h::pow with pow(I, n) → I and pow(inv(A), n) →
// inv(pow(A, n)). The existing rules pow(0, n) → 0, pow(A, 0) → I,
// pow(A, 1) → A, pow(pow(A,m), n) → pow(A, m*n) are already covered.
// ---------------------------------------------------------------------------

TEST(CoreBugFix, TensorPowIdentityIsIdempotent) {
  // pow(I, n) → I (kronecker_delta as I)
  auto I = make_expression<kronecker_delta>(std::size_t{3});
  EXPECT_EQ(pow(I, 5), I);
  EXPECT_EQ(pow(I, 0), I); // also covered by existing pow(A, 0) → I rule
  // For pow(A, 1) the existing rule returns A directly; identity round-trips
  EXPECT_EQ(pow(I, 1), I);
}

TEST(CoreBugFix, TensorPowOfInvLiftsToInvOfPow) {
  // pow(inv(A), n) → inv(pow(A, n))
  // Use an even-dim symmetric A so inv() doesn't reject and any structural
  // checks downstream are well-defined.
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{4}, std::size_t{2}});
  EXPECT_EQ(pow(inv(A), 2), inv(pow(A, 2)));
  EXPECT_EQ(pow(inv(A), 3), inv(pow(A, 3)));
}

TEST(CoreBugFix, TensorPowPowChains) {
  // pow(pow(A, 2), 3) → pow(A, 6) — already existed; lock in.
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  EXPECT_EQ(pow(pow(A, 2), 3), pow(A, 6));
}

// ---------------------------------------------------------------------------
// mul × mul merges like factors (verifies issue #97's claim was wrong —
// the rules ARE implemented, in per-domain wrappers rather than a generic
// dispatcher; these regressions lock in the contract).
// ---------------------------------------------------------------------------

TEST(CoreBugFix, MulMulMergesLikeFactorsScalar) {
  // EXPECT_EQ compares via expression_holder::operator==, which is
  // hash-based — child ordering inside the resulting mul is not
  // observable through this assertion. The "alphabetical-looking"
  // expected forms below match the canonical hash order on this
  // platform but the test passes regardless of order.
  auto [x, y, z, a] = make_scalar_variable("x", "y", "z", "a");
  auto two = make_scalar_constant(2);
  auto three = make_scalar_constant(3);

  // (2*x*y) * (3*x*z) -> 6*pow(x,2)*y*z
  EXPECT_EQ((two * x * y) * (three * x * z), 6 * pow(x, 2) * y * z);
  // (x*y) * (x*z) -> pow(x,2)*y*z
  EXPECT_EQ((x * y) * (x * z), pow(x, 2) * y * z);
  // (x*y*z) * (x*a) -> a*pow(x,2)*y*z
  EXPECT_EQ((x * y * z) * (x * a), a * pow(x, 2) * y * z);
}

TEST(CoreBugFix, MulMulMergesLikeFactorsT2s) {
  // Same contract for tensor-to-scalar mul × mul, via the push_or_combine
  // helper in tensor_to_scalar_simplifier_mul.cpp.
  auto [X, Y, Z] =
      make_tensor_variable(std::tuple{"X", std::size_t{3}, std::size_t{2}},
                           std::tuple{"Y", std::size_t{3}, std::size_t{2}},
                           std::tuple{"Z", std::size_t{3}, std::size_t{2}});
  auto tX = trace(X);
  auto tY = trace(Y);
  auto tZ = trace(Z);
  // (tr(X) * tr(Y)) * (tr(X) * tr(Z)) -> pow(tr(X), 2) * tr(Y) * tr(Z)
  EXPECT_EQ((tX * tY) * (tX * tZ), pow(tX, 2) * tY * tZ);
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
