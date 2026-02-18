#ifndef LIMITVISITORTEST_H
#define LIMITVISITORTEST_H

#include <gtest/gtest.h>

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/contains_expression.h>
#include <numsim_cas/core/limit_result.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/scalar/visitors/scalar_limit_visitor.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_functions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_std.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_limit_visitor.h>

namespace numsim::cas {

using dir = limit_result::direction;
using gtype = growth_rate::type;
using pt = limit_target::point;

// ═══════════════════════════════════════════════════════════════════
// contains_expression tests
// ═══════════════════════════════════════════════════════════════════

TEST(ContainsExpr, ScalarSelf) {
  auto x = make_expression<scalar>("x");
  EXPECT_TRUE(contains_expression(x, x));
}

TEST(ContainsExpr, ScalarInAdd) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  auto expr = x + y;
  EXPECT_TRUE(contains_expression(expr, x));
  EXPECT_TRUE(contains_expression(expr, y));
}

TEST(ContainsExpr, ScalarNotPresent) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  auto z = make_expression<scalar>("z");
  auto expr = x + y;
  EXPECT_FALSE(contains_expression(expr, z));
}

TEST(ContainsExpr, ScalarNested) {
  auto x = make_expression<scalar>("x");
  auto expr = log(sin(x));
  EXPECT_TRUE(contains_expression(expr, x));
}

TEST(ContainsExpr, TensorSelf) {
  auto F = make_expression<tensor>("F", 3, 2);
  EXPECT_TRUE(contains_expression(F, F));
}

TEST(ContainsExpr, TensorNotPresent) {
  auto F = make_expression<tensor>("F", 3, 2);
  auto G = make_expression<tensor>("G", 3, 2);
  EXPECT_FALSE(contains_expression(F, G));
}

TEST(ContainsExpr, DependsOnTensorTrue) {
  auto F = make_expression<tensor>("F", 3, 2);
  auto expr = det(F);
  EXPECT_TRUE(depends_on_tensor(expr, F));
}

TEST(ContainsExpr, DependsOnTensorFalse) {
  auto F = make_expression<tensor>("F", 3, 2);
  auto G = make_expression<tensor>("G", 3, 2);
  auto expr = det(F);
  EXPECT_FALSE(depends_on_tensor(expr, G));
}

TEST(ContainsExpr, DependsOnTensorInArithmetic) {
  auto F = make_expression<tensor>("F", 3, 2);
  auto G = make_expression<tensor>("G", 3, 2);
  // log(det(F)) + trace(G)
  auto expr = log(det(F)) + trace(G);
  EXPECT_TRUE(depends_on_tensor(expr, F));
  EXPECT_TRUE(depends_on_tensor(expr, G));
}

TEST(ContainsExpr, DependsOnTensorScalarWrapperFalse) {
  auto F = make_expression<tensor>("F", 3, 2);
  auto c = make_expression<scalar_constant>(2.0);
  auto expr = make_expression<tensor_to_scalar_scalar_wrapper>(c);
  EXPECT_FALSE(depends_on_tensor(expr, F));
}

// ═══════════════════════════════════════════════════════════════════
// Scalar limit visitor tests
// ═══════════════════════════════════════════════════════════════════

TEST(ScalarLimit, ConstantLimit) {
  auto x = make_expression<scalar>("x");
  auto c = make_scalar_constant(5);
  scalar_limit_visitor v(x, {pt::zero_plus});
  auto result = v.apply(c);
  EXPECT_EQ(result.dir, dir::finite_positive);
}

TEST(ScalarLimit, ZeroLimit) {
  auto x = make_expression<scalar>("x");
  scalar_limit_visitor v(x, {pt::zero_plus});
  auto result = v.apply(get_scalar_zero());
  EXPECT_EQ(result.dir, dir::zero);
}

TEST(ScalarLimit, OneLimit) {
  auto x = make_expression<scalar>("x");
  scalar_limit_visitor v(x, {pt::zero_plus});
  auto result = v.apply(get_scalar_one());
  EXPECT_EQ(result.dir, dir::finite_positive);
}

TEST(ScalarLimit, VariableToZeroPlus) {
  auto x = make_expression<scalar>("x");
  scalar_limit_visitor v(x, {pt::zero_plus});
  auto result = v.apply(x);
  EXPECT_EQ(result.dir, dir::zero);
}

TEST(ScalarLimit, VariableToPosInfinity) {
  auto x = make_expression<scalar>("x");
  scalar_limit_visitor v(x, {pt::pos_infinity});
  auto result = v.apply(x);
  EXPECT_EQ(result.dir, dir::pos_infinity);
}

TEST(ScalarLimit, OtherVariableFinite) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  scalar_limit_visitor v(x, {pt::zero_plus});
  auto result = v.apply(y);
  EXPECT_EQ(result.dir, dir::finite_positive);
}

// ─── log(x) as x -> 0+ ────────────────────────────────────────────

TEST(ScalarLimit, LogToZeroPlus) {
  auto x = make_expression<scalar>("x");
  auto expr = log(x);
  scalar_limit_visitor v(x, {pt::zero_plus});
  auto result = v.apply(expr);
  EXPECT_EQ(result.dir, dir::neg_infinity);
  EXPECT_EQ(result.rate.rate, gtype::logarithmic);
}

// ─── log(x) as x -> +inf ──────────────────────────────────────────

TEST(ScalarLimit, LogToPosInfinity) {
  auto x = make_expression<scalar>("x");
  auto expr = log(x);
  scalar_limit_visitor v(x, {pt::pos_infinity});
  auto result = v.apply(expr);
  EXPECT_EQ(result.dir, dir::pos_infinity);
  EXPECT_EQ(result.rate.rate, gtype::logarithmic);
}

// ─── x^(-1) as x -> 0+ ────────────────────────────────────────────

TEST(ScalarLimit, ReciprocalToZeroPlus) {
  auto x = make_expression<scalar>("x");
  auto expr = pow(x, make_scalar_constant(-1));
  scalar_limit_visitor v(x, {pt::zero_plus});
  auto result = v.apply(expr);
  EXPECT_EQ(result.dir, dir::pos_infinity);
}

// ─── sin(x) as x -> +inf ──────────────────────────────────────────

TEST(ScalarLimit, SinToInfinity) {
  auto x = make_expression<scalar>("x");
  auto expr = sin(x);
  scalar_limit_visitor v(x, {pt::pos_infinity});
  auto result = v.apply(expr);
  EXPECT_EQ(result.dir, dir::unknown);
}

// ─── exp(x) as x -> +inf ──────────────────────────────────────────

TEST(ScalarLimit, ExpToPosInfinity) {
  auto x = make_expression<scalar>("x");
  auto expr = exp(x);
  scalar_limit_visitor v(x, {pt::pos_infinity});
  auto result = v.apply(expr);
  EXPECT_EQ(result.dir, dir::pos_infinity);
  EXPECT_EQ(result.rate.rate, gtype::exponential);
}

// ─── exp(-x) as x -> +inf ─────────────────────────────────────────

TEST(ScalarLimit, ExpNegToPosInfinity) {
  auto x = make_expression<scalar>("x");
  auto expr = exp(-x);
  scalar_limit_visitor v(x, {pt::pos_infinity});
  auto result = v.apply(expr);
  EXPECT_EQ(result.dir, dir::zero);
}

// ─── sqrt(x) as x -> +inf ─────────────────────────────────────────

TEST(ScalarLimit, SqrtToPosInfinity) {
  auto x = make_expression<scalar>("x");
  auto expr = sqrt(x);
  scalar_limit_visitor v(x, {pt::pos_infinity});
  auto result = v.apply(expr);
  EXPECT_EQ(result.dir, dir::pos_infinity);
}

// ─── abs(-x) as x -> +inf ─────────────────────────────────────────

TEST(ScalarLimit, AbsNegToPosInfinity) {
  auto x = make_expression<scalar>("x");
  auto expr = abs(-x);
  scalar_limit_visitor v(x, {pt::pos_infinity});
  auto result = v.apply(expr);
  EXPECT_EQ(result.dir, dir::pos_infinity);
}

// ─── Indeterminate: x * log(x) as x -> 0+ ─────────────────────────

TEST(ScalarLimit, IndeterminateZeroTimesLog) {
  auto x = make_expression<scalar>("x");
  auto expr = x * log(x);
  scalar_limit_visitor v(x, {pt::zero_plus});
  auto result = v.apply(expr);
  // 0 * (-inf) = indeterminate
  EXPECT_EQ(result.dir, dir::indeterminate);
}

// ─── x + log(x) as x -> 0+ ────────────────────────────────────────

TEST(ScalarLimit, AddFiniteAndNegInf) {
  auto x = make_expression<scalar>("x");
  auto expr = x + log(x);
  scalar_limit_visitor v(x, {pt::zero_plus});
  auto result = v.apply(expr);
  // 0 + (-inf) = -inf
  EXPECT_EQ(result.dir, dir::neg_infinity);
}

// ─── atan(x) as x -> +inf ─────────────────────────────────────────

TEST(ScalarLimit, AtanToPosInfinity) {
  auto x = make_expression<scalar>("x");
  auto expr = atan(x);
  scalar_limit_visitor v(x, {pt::pos_infinity});
  auto result = v.apply(expr);
  EXPECT_EQ(result.dir, dir::finite_positive);
}

// ─── atan(x) as x -> -inf ─────────────────────────────────────────

TEST(ScalarLimit, AtanToNegInfinity) {
  auto x = make_expression<scalar>("x");
  auto expr = atan(x);
  scalar_limit_visitor v(x, {pt::neg_infinity});
  auto result = v.apply(expr);
  EXPECT_EQ(result.dir, dir::finite_negative);
}

// ─── Negative constant ─────────────────────────────────────────────

TEST(ScalarLimit, NegativeConstant) {
  auto x = make_expression<scalar>("x");
  auto c = make_scalar_constant(-3);
  scalar_limit_visitor v(x, {pt::zero_plus});
  auto result = v.apply(c);
  EXPECT_EQ(result.dir, dir::finite_negative);
}

// ─── Negation ───────────────────────────────────────────────────────

TEST(ScalarLimit, NegationFlips) {
  auto x = make_expression<scalar>("x");
  auto expr = -x;
  scalar_limit_visitor v(x, {pt::pos_infinity});
  auto result = v.apply(expr);
  EXPECT_EQ(result.dir, dir::neg_infinity);
}

// ═══════════════════════════════════════════════════════════════════
// T2S limit visitor tests (exact match mode)
// ═══════════════════════════════════════════════════════════════════

TEST(T2sLimit, ExactMatchZero) {
  auto F = make_expression<tensor>("F", 3, 2);
  auto J = det(F);
  tensor_to_scalar_limit_visitor v(J, {pt::zero_plus});
  auto result = v.apply(make_expression<tensor_to_scalar_zero>());
  EXPECT_EQ(result.dir, dir::zero);
}

TEST(T2sLimit, ExactMatchOne) {
  auto F = make_expression<tensor>("F", 3, 2);
  auto J = det(F);
  tensor_to_scalar_limit_visitor v(J, {pt::zero_plus});
  auto result = v.apply(make_expression<tensor_to_scalar_one>());
  EXPECT_EQ(result.dir, dir::finite_positive);
}

TEST(T2sLimit, ExactMatchSelf) {
  auto F = make_expression<tensor>("F", 3, 2);
  auto J = det(F);
  tensor_to_scalar_limit_visitor v(J, {pt::zero_plus});
  auto result = v.apply(J);
  EXPECT_EQ(result.dir, dir::zero);
}

TEST(T2sLimit, ExactMatchLogDetToZeroPlus) {
  auto F = make_expression<tensor>("F", 3, 2);
  auto J = det(F);
  auto expr = log(J);
  tensor_to_scalar_limit_visitor v(J, {pt::zero_plus});
  auto result = v.apply(expr);
  // log(0+) = -inf (logarithmic)
  EXPECT_EQ(result.dir, dir::neg_infinity);
  EXPECT_EQ(result.rate.rate, gtype::logarithmic);
}

TEST(T2sLimit, ExactMatchLogDetSquaredToZeroPlus) {
  auto F = make_expression<tensor>("F", 3, 2);
  auto J = det(F);
  // (log(J))^2 as J -> 0+
  auto expr = pow(log(J), make_scalar_constant(2));
  tensor_to_scalar_limit_visitor v(J, {pt::zero_plus});
  auto result = v.apply(expr);
  // log(0+) = -inf, (-inf)^2 -> unknown (sign depends on parity)
  // Actually: pow(neg_inf, finite_positive) -> unknown for neg base
  EXPECT_EQ(result.dir, dir::unknown);
}

TEST(T2sLimit, ExactMatchDetToPosInfinity) {
  auto F = make_expression<tensor>("F", 3, 2);
  auto J = det(F);
  tensor_to_scalar_limit_visitor v(J, {pt::pos_infinity});
  auto result = v.apply(J);
  EXPECT_EQ(result.dir, dir::pos_infinity);
}

TEST(T2sLimit, ExactMatchNegativeLog) {
  auto F = make_expression<tensor>("F", 3, 2);
  auto J = det(F);
  auto expr = -log(J);
  tensor_to_scalar_limit_visitor v(J, {pt::zero_plus});
  auto result = v.apply(expr);
  // -log(0+) = -(−inf) = +inf
  EXPECT_EQ(result.dir, dir::pos_infinity);
}

// ═══════════════════════════════════════════════════════════════════
// T2S limit visitor tests (tensor dependency mode)
// ═══════════════════════════════════════════════════════════════════

TEST(T2sLimit, TensorDepNormToPosInfinity) {
  auto F = make_expression<tensor>("F", 3, 2);
  auto expr = norm(F);
  tensor_to_scalar_limit_visitor v(F, {pt::pos_infinity});
  auto result = v.apply(expr);
  // norm(F) as ||F|| -> inf should be +inf
  EXPECT_EQ(result.dir, dir::pos_infinity);
}

TEST(T2sLimit, TensorDepIndependentExpr) {
  auto F = make_expression<tensor>("F", 3, 2);
  auto G = make_expression<tensor>("G", 3, 2);
  auto expr = det(G); // independent of F
  tensor_to_scalar_limit_visitor v(F, {pt::pos_infinity});
  auto result = v.apply(expr);
  EXPECT_EQ(result.dir, dir::finite_positive);
}

TEST(T2sLimit, TensorDepDetIsUnknown) {
  auto F = make_expression<tensor>("F", 3, 2);
  auto expr = det(F);
  tensor_to_scalar_limit_visitor v(F, {pt::pos_infinity});
  auto result = v.apply(expr);
  // det(F) as F -> inf is unknown (det can be anything)
  EXPECT_EQ(result.dir, dir::unknown);
}

TEST(T2sLimit, TensorDepScalarWrapperFinite) {
  auto F = make_expression<tensor>("F", 3, 2);
  auto c = make_expression<scalar_constant>(3.0);
  auto expr = make_expression<tensor_to_scalar_scalar_wrapper>(c);
  tensor_to_scalar_limit_visitor v(F, {pt::pos_infinity});
  auto result = v.apply(expr);
  EXPECT_EQ(result.dir, dir::finite_positive);
}

// ═══════════════════════════════════════════════════════════════════
// Growth rate tracking
// ═══════════════════════════════════════════════════════════════════

TEST(GrowthRate, LogIsLogarithmic) {
  auto x = make_expression<scalar>("x");
  scalar_limit_visitor v(x, {pt::pos_infinity});
  auto result = v.apply(log(x));
  EXPECT_EQ(result.rate.rate, gtype::logarithmic);
}

TEST(GrowthRate, ExpIsExponential) {
  auto x = make_expression<scalar>("x");
  scalar_limit_visitor v(x, {pt::pos_infinity});
  auto result = v.apply(exp(x));
  EXPECT_EQ(result.rate.rate, gtype::exponential);
}

TEST(GrowthRate, VariableIsPoly) {
  auto x = make_expression<scalar>("x");
  scalar_limit_visitor v(x, {pt::pos_infinity});
  auto result = v.apply(x);
  EXPECT_EQ(result.rate.rate, gtype::polynomial);
}

// ═══════════════════════════════════════════════════════════════════
// Neo-Hookean-style energy function
// W = mu/2*(tr(F^T F) - 3) - mu*log(J) + lambda/2*(log(J))^2
// As J -> 0+, -mu*log(J) -> +inf (logarithmic)
// So W -> +inf
// ═══════════════════════════════════════════════════════════════════

TEST(T2sLimit, NeoHookeanCompressionLimit) {
  auto F = make_expression<tensor>("F", 3, 2);
  auto J = det(F);

  // Build: -mu*log(J) where mu > 0
  // Using exact match mode with J as limit variable
  auto mu = make_expression<tensor_to_scalar_scalar_wrapper>(
      make_scalar_constant(1));
  auto neg_mu_log_J = -(mu * log(J));

  tensor_to_scalar_limit_visitor v(J, {pt::zero_plus});
  auto result = v.apply(neg_mu_log_J);
  // -log(0+) = -(-inf) = +inf
  EXPECT_EQ(result.dir, dir::pos_infinity);
}

} // namespace numsim::cas

#endif // LIMITVISITORTEST_H
