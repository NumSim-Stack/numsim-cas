#ifndef SCALARDIFFERENTIATIONTEST_H
#define SCALARDIFFERENTIATIONTEST_H

#include <gtest/gtest.h>

#include <numsim_cas/core/diff.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_diff.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>

namespace numsim::cas {

namespace {
using expr_t = expression_holder<scalar_expression>;

#define expect_is_zero(e)                                                      \
  EXPECT_TRUE(is_same<scalar_zero>(e)) << "Expected 0, got: " << to_string(e);

#define expect_is_one(e)                                                       \
  EXPECT_TRUE(is_same<scalar_one>(e)) << "Expected 1, got: " << to_string(e);

} // namespace

TEST(ScalarDifferentiation, VariableAndConstant) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  auto c = make_expression<scalar_constant>(5.0);

  expect_is_one(diff(x, x));
  expect_is_zero(diff(y, x));
  expect_is_zero(diff(c, x));
  expect_is_zero(diff(get_scalar_one(), x));
  expect_is_zero(diff(get_scalar_zero(), x));
}

TEST(ScalarDifferentiation, AddAndMul) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");

  // d/dx (x + y) = 1
  {
    auto d = diff(x + y, x);
    expect_is_one(d);
  }

  // d/dx (x * y) = y
  {
    auto d = diff(x * y, x);
    expect_is_zero(d - y);
  }

  // d/dx (x * x) = x + x (or 2*x, depending on simplification)
  {
    auto d = diff(x * x, x);
    expect_is_zero(d - x - x);
  }
}

TEST(ScalarDifferentiation, DivisionRule) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");

  // d/dx (x / y) = 1 / y
  auto d = diff(x / y, x);
  expect_is_zero(d - (1 / y));
}

TEST(ScalarDifferentiation, TrigonometryChainRule) {
  auto x = make_expression<scalar>("x");

  // d/dx sin(x) = cos(x)
  {
    auto d = diff(sin(x), x);
    expect_is_zero(d - cos(x));
  }

  // d/dx cos(x) = -sin(x)
  {
    auto d = diff(cos(x), x);
    expect_is_zero(d + sin(x));
  }

  // d/dx tan(x) = (1/cos(x))^2
  {
    auto one = get_scalar_one();
    auto two = make_expression<scalar_constant>(2);
    auto expected = pow(1 / cos(x), two);

    auto d = diff(tan(x), x);
    expect_is_zero(d - expected);
  }
}

TEST(ScalarDifferentiation, PowConstantExponent) {
  auto x = make_expression<scalar>("x");

  auto three = make_expression<scalar_constant>(3.0);
  auto two = make_expression<scalar_constant>(2.0);

  // d/dx x^3 = 3 * x^2
  auto expr = pow(x, three);
  auto d = diff(expr, x);

  auto expected = three * pow(x, two);
  expect_is_zero(d - expected);
}

// ---------------------------------------------------------------------------
// Audit #76: lock-in coverage tests for scalar_differentiation.
// One test per node type in NUMSIM_CAS_SCALAR_NODE_LIST. The differentiator
// has explicit operator() overloads for all 20 node types, with a
// static_assert fallback that fails to compile if a new node lacks an
// override. These tests pin the symbolic-derivative contract so a future
// refactor cannot silently break a rule.
// ---------------------------------------------------------------------------

TEST(ScalarDifferentiationAudit, ScalarNegative) {
  auto x = make_expression<scalar>("x");
  // d/dx (-x) = -1
  auto d = diff(-x, x);
  expect_is_zero(d + get_scalar_one());
}

TEST(ScalarDifferentiationAudit, ScalarSignTreatedAsZero) {
  auto x = make_expression<scalar>("x");
  // d/dx sign(x) is 0 (per documented CAS choice; strictly 2*delta(x)).
  auto d = diff(sign(x), x);
  expect_is_zero(d);
}

TEST(ScalarDifferentiationAudit, ScalarLog) {
  auto x = make_expression<scalar>("x");
  // d/dx log(x) = 1/x
  auto d = diff(log(x), x);
  expect_is_zero(d - (get_scalar_one() / x));
}

TEST(ScalarDifferentiationAudit, ScalarExp) {
  auto x = make_expression<scalar>("x");
  // d/dx exp(x) = exp(x)
  auto d = diff(exp(x), x);
  expect_is_zero(d - exp(x));
}

TEST(ScalarDifferentiationAudit, ScalarSqrt) {
  auto x = make_expression<scalar>("x");
  auto two = make_expression<scalar_constant>(2.0);
  // d/dx sqrt(x) = 1 / (2*sqrt(x))
  auto d = diff(sqrt(x), x);
  expect_is_zero(d - (get_scalar_one() / (two * sqrt(x))));
}

TEST(ScalarDifferentiationAudit, ScalarAsin) {
  auto x = make_expression<scalar>("x");
  auto one = get_scalar_one();
  auto two = make_expression<scalar_constant>(2.0);
  // d/dx asin(x) = 1 / sqrt(1 - x^2)
  auto d = diff(asin(x), x);
  expect_is_zero(d - (one / sqrt(one - pow(x, two))));
}

TEST(ScalarDifferentiationAudit, ScalarAcos) {
  auto x = make_expression<scalar>("x");
  auto one = get_scalar_one();
  auto two = make_expression<scalar_constant>(2.0);
  // d/dx acos(x) = -1 / sqrt(1 - x^2)
  auto d = diff(acos(x), x);
  expect_is_zero(d + (one / sqrt(one - pow(x, two))));
}

TEST(ScalarDifferentiationAudit, ScalarAtan) {
  auto x = make_expression<scalar>("x");
  auto one = get_scalar_one();
  auto two = make_expression<scalar_constant>(2.0);
  // d/dx atan(x) = 1 / (1 + x^2)
  auto d = diff(atan(x), x);
  expect_is_zero(d - (one / (one + pow(x, two))));
}

TEST(ScalarDifferentiationAudit, ScalarAbsPositiveOperand) {
  auto x = make_expression<scalar>("x");
  // When |u| has a known-positive operand, d|u|/dx degenerates to u'.
  // Using a positive constant inside abs as the simplest positive case:
  // d/dx abs(exp(x)) — exp is always positive — should be exp(x).
  auto d = diff(abs(exp(x)), x);
  expect_is_zero(d - exp(x));
}

TEST(ScalarDifferentiationAudit, ScalarNamedExpression) {
  auto x = make_expression<scalar>("x");
  // A named expression wrapping x^2: d/dx should produce a named
  // expression "df" wrapping 2*x.
  auto named = make_expression<scalar_named_expression>("f", pow(x, 2));
  auto d = diff(named, x);
  EXPECT_TRUE(is_same<scalar_named_expression>(d))
      << "Expected scalar_named_expression, got: " << to_string(d);
  EXPECT_EQ(d.get<scalar_named_expression>().name(), "df");
}

TEST(ScalarDifferentiationAudit, NestedChainRule) {
  auto x = make_expression<scalar>("x");
  // d/dx sin(cos(x)) = -sin(x) * cos(cos(x))
  // Use EXPECT_EQ rather than expect_is_zero(d - expected) because the
  // canonical simplification of `-sin*cos - (-sin*cos)` produces
  // scalar_negative(scalar_zero) rather than scalar_zero; hash-based
  // equality compares them as equal but is_same<scalar_zero> doesn't.
  auto d = diff(sin(cos(x)), x);
  auto expected = -sin(x) * cos(cos(x));
  EXPECT_EQ(d, expected);
}

TEST(ScalarDifferentiationAudit, PowVariableBaseVariableExponent) {
  auto x = make_expression<scalar>("x");
  auto one = get_scalar_one();
  // d/dx x^x = x^(x-1) * (x + log(x)*x)
  // Implementation: g=x, h=x → general case
  // pow(x, x-1) * (x*1 + 1*log(x)*x) = pow(x, x-1) * (x + x*log(x))
  auto d = diff(pow(x, x), x);
  auto expected = pow(x, x - one) * (x + log(x) * x);
  expect_is_zero(d - expected);
}

// ─── Accumulator-init regression (follow-up to #305) ───────────────
// scalar_differentiation's scalar_mul / scalar_add visitors previously
// default-constructed the accumulator (invalid holder) and relied on
// expression_holder::operator+=/*='s "invalid lhs → assign rhs"
// safety net. That worked for the visitor itself but left an invalid
// intermediate visible to any code that read `accumulator.data()->...`
// directly — surfaced as a segfault when the #305 positivity-
// propagation read() landed in the mul instrumentation (FuzzyScalar
// Depth3 seed 35). Lock-in tests ensure the accumulator is initialized
// to the identity element (scalar_zero for additive, scalar_one for
// multiplicative) so any future read() in the visitor stays valid.

TEST(ScalarDifferentiationAudit, MulAccumulatorReturnsScalarZeroForConstant) {
  // d(c1·c2)/dx = 0. The mul visitor's outer accumulator is a
  // sum; if not initialized to scalar_zero, the result could be
  // invalid (and apply_imp's fallback would handle it). With the
  // identity-init, the result is structurally scalar_zero already.
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  auto z = make_expression<scalar>("z");
  auto d = diff(y * z, x);
  EXPECT_TRUE(is_same<scalar_zero>(d));
}

TEST(ScalarDifferentiationAudit, AddAccumulatorReturnsScalarZeroForConstant) {
  // Same shape for scalar_add.
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  auto z = make_expression<scalar>("z");
  auto d = diff(y + z, x);
  EXPECT_TRUE(is_same<scalar_zero>(d));
}

TEST(ScalarDifferentiationAudit, MulAccumulatorProductRuleStillWorks) {
  // Sanity that initializing the inner accumulator to scalar_one
  // doesn't bloat the product rule output. d/dx(x * y) = y.
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  auto d = diff(x * y, x);
  // After folding scalar_one * y = y.
  EXPECT_EQ(d, y);
}

} // namespace numsim::cas

#endif // SCALARDIFFERENTIATIONTEST_H
