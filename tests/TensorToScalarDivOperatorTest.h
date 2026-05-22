#ifndef TENSORTOSCALARDIVOPERATORTEST_H
#define TENSORTOSCALARDIVOPERATORTEST_H

// Lock-in tests for `tag_invoke(div_fn, …)` on (tensor, t2s) (#147).
// Closes the symmetric gap to the mul operator added in #145. The
// no-fold path routes through `lhs × pow(rhs, -1)`, reusing the
// tensor × t2s mul (#145) and the existing t2s pow simplifier — the
// dead `tensor_to_scalar_with_tensor_div` node is deliberately not
// constructed.

#include "cas_test_helpers.h"
#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"

#include <numsim_cas/tensor/visitors/tensor_evaluator.h>

namespace numsim::cas {

namespace {

using TestDims = ::testing::Types<std::integral_constant<std::size_t, 1>,
                                  std::integral_constant<std::size_t, 2>,
                                  std::integral_constant<std::size_t, 3>>;

} // namespace

template <typename DimTag>
class TensorToScalarDivOperatorTest : public ::testing::Test {
protected:
  static constexpr std::size_t Dim = DimTag::value;

  using tensor_t = expression_holder<tensor_expression>;
  using scalar_t = expression_holder<scalar_expression>;

  TensorToScalarDivOperatorTest() {
    std::tie(X) = make_tensor_variable(std::tuple{"X", Dim, 2});
  }

  tensor_t X;
};

TYPED_TEST_SUITE(TensorToScalarDivOperatorTest, TestDims);

// --- 1. Compile-smoke: `A / trace(A)` and `A / det(A)` build.

TYPED_TEST(TensorToScalarDivOperatorTest, CompileSmokeBothFunctions) {
  auto &X = this->X;
  auto e_tr = X / trace(X);
  auto e_det = X / det(X);
  ASSERT_TRUE(e_tr.is_valid());
  ASSERT_TRUE(e_det.is_valid());
  // Both go through the mul path (the dead t2s_with_tensor_div node
  // is deliberately bypassed — see #147 rationale).
  EXPECT_TRUE(is_same<tensor_to_scalar_with_tensor_mul>(e_tr));
  EXPECT_TRUE(is_same<tensor_to_scalar_with_tensor_mul>(e_det));
}

// --- 2. Result shape: produces `tensor × pow(t2s, -1)` (#145 mul node
//        with a pow rhs), not the dedicated div node.
//
// This pins the *current* shape produced by the div operator. If t2s
// pow ever folds `pow(x, -1)` into a different node (e.g. a dedicated
// reciprocal), this test will need updating even though the
// behaviour-level contract (evaluator round-trip below) still holds.
// The shape lock-in is deliberate — if someone changes the
// implementation strategy they should re-confirm the test intent.

TYPED_TEST(TensorToScalarDivOperatorTest, ResultShapeIsMulPow) {
  auto &X = this->X;
  auto result = X / trace(X);
  ASSERT_TRUE(is_same<tensor_to_scalar_with_tensor_mul>(result));

  auto const &node = result.template get<tensor_to_scalar_with_tensor_mul>();
  // The t2s side should be a pow expression of trace(X).
  ASSERT_TRUE(is_same<tensor_to_scalar_pow>(node.expr_rhs()));
  // Tensor side untouched.
  EXPECT_EQ(node.expr_lhs(), X);

  // Pin the pow's structure exactly — base is trace(X), exponent is
  // -1 (wrapped as a t2s_scalar_wrapper). Without these two checks the
  // test would silently pass if a sign-flip bug ever made the operator
  // emit `pow(trace(X), +1)` or `pow(trace(X), 0)`.
  auto const &pow_node = node.expr_rhs().template get<tensor_to_scalar_pow>();
  EXPECT_EQ(pow_node.expr_lhs(), trace(X));
  auto expected_exponent =
      make_expression<tensor_to_scalar_scalar_wrapper>(-get_scalar_one());
  EXPECT_EQ(pow_node.expr_rhs(), expected_exponent);
}

// --- 3. Zero fold: tensor_zero / anything → tensor_zero.

TYPED_TEST(TensorToScalarDivOperatorTest, TensorZeroFold) {
  constexpr std::size_t Dim = TestFixture::Dim;
  auto &X = this->X;
  auto Z = make_expression<tensor_zero>(Dim, 2);
  EXPECT_TRUE(is_same<tensor_zero>(Z / trace(X)));
  EXPECT_TRUE(is_same<tensor_zero>(Z / det(X)));
}

// --- 4. One fold: tensor / tensor_to_scalar_one → tensor.

TYPED_TEST(TensorToScalarDivOperatorTest, T2sOneFoldReturnsTensor) {
  auto &X = this->X;
  auto t2s_one = make_expression<tensor_to_scalar_one>();
  EXPECT_EQ(X / t2s_one, X);
}

// --- 5. Wrapper unwrap routes through tensor ÷ scalar.

TYPED_TEST(TensorToScalarDivOperatorTest, WrapperUnwrapsToScalarPath) {
  auto &X = this->X;
  auto two = make_scalar_constant(2);
  auto wrapped = make_expression<tensor_to_scalar_scalar_wrapper>(two);

  auto result = X / wrapped;
  // Positive lock-in: the wrapper-unwrap branch is supposed to route
  // through the existing tensor ÷ scalar path, which yields a
  // tensor_scalar_mul with the scalar coefficient pow(2, -1). Assert
  // that exact result type — a weaker `EXPECT_FALSE(t2s_with_tensor_mul)`
  // would pass for any non-t2s-mul shape, including buggy fallbacks.
  EXPECT_TRUE(is_same<tensor_scalar_mul>(result));
}

// --- 5b. Dividing by a pow exponent should flatten via t2s pow-of-pow.
//
// `A / pow(trace(A), 2)` routes through `A * pow(pow(trace(A), 2), -1)`.
// The t2s pow simplifier folds `pow(pow(x, m), n) → pow(x, m*n)`, so
// the resulting tree should be `A * pow(trace(A), -2)` — *not* a
// nested `pow(pow(...))`. This locks in the interaction between the
// new div operator and the existing t2s pow flattening.

TYPED_TEST(TensorToScalarDivOperatorTest, DivByPowFlattensExponent) {
  auto &X = this->X;
  auto two = make_scalar_constant(2);

  auto result = X / pow(trace(X), two);
  ASSERT_TRUE(is_same<tensor_to_scalar_with_tensor_mul>(result));

  auto const &node = result.template get<tensor_to_scalar_with_tensor_mul>();
  ASSERT_TRUE(is_same<tensor_to_scalar_pow>(node.expr_rhs()));

  // The pow's base is the inner trace(X) — the outer pow node from the
  // user-written `pow(trace(X), 2)` is gone, collapsed away by the
  // flattening rule.
  auto const &pow_node = node.expr_rhs().template get<tensor_to_scalar_pow>();
  EXPECT_EQ(pow_node.expr_lhs(), trace(X));
  EXPECT_FALSE(is_same<tensor_to_scalar_pow>(pow_node.expr_lhs()));
}

// --- 6. Evaluator round-trip: numeric A/trace(A) matches tmech.

TEST(TensorToScalarDivOperatorEval, EvaluatorMatchesTmech) {
  constexpr double tol = 1e-12;
  auto A = make_expression<tensor>("A", 2, 2);

  tensor_evaluator<double> ev;
  auto data = std::make_shared<tensor_data<double, 2, 2>>();
  auto *raw = data->raw_data();
  // clang-format off
  raw[0] = 1.0; raw[1] = 2.0;
  raw[2] = 3.0; raw[3] = 4.0;
  // clang-format on
  ev.set(A, data);

  // trace(A) = 5; expect A/5 → [[0.2,0.4],[0.6,0.8]].
  auto expr = A / trace(A);
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);

  tmech::tensor<double, 2, 2> expected;
  auto *e = expected.raw_data();
  e[0] = 0.2;
  e[1] = 0.4;
  e[2] = 0.6;
  e[3] = 0.8;

  auto const &got =
      static_cast<tensor_data<double, 2, 2> const &>(*result).data();
  EXPECT_TRUE(tmech::almost_equal(got, expected, tol));
}

// --- 7. Round-trip equivalence: A/t2s ≡ A * (t2s_one/t2s).
//
// Uses `norm(A)` rather than `trace(A)` because norm is always
// strictly positive (modulus of components), guaranteeing the
// denominator is never zero across whatever random values the
// evaluator happens to see. `trace(A)` can be zero for non-degenerate
// inputs (any matrix on the deviatoric subspace), which would flip
// this from a behaviour check into an undefined-arithmetic check.

TEST(TensorToScalarDivOperatorEval, DivEquivalentToMulReciprocal) {
  constexpr double tol = 1e-12;
  auto A = make_expression<tensor>("A", 2, 2);

  tensor_evaluator<double> ev;
  auto data = std::make_shared<tensor_data<double, 2, 2>>();
  auto *raw = data->raw_data();
  raw[0] = 4.0;
  raw[1] = 6.0;
  raw[2] = 8.0;
  raw[3] = 10.0;
  ev.set(A, data);

  auto div_form = A / norm(A);
  auto mul_form = A * (make_expression<tensor_to_scalar_one>() / norm(A));

  auto r_div = ev.apply(div_form);
  auto r_mul = ev.apply(mul_form);
  ASSERT_NE(r_div, nullptr);
  ASSERT_NE(r_mul, nullptr);

  auto const &got_div =
      static_cast<tensor_data<double, 2, 2> const &>(*r_div).data();
  auto const &got_mul =
      static_cast<tensor_data<double, 2, 2> const &>(*r_mul).data();
  EXPECT_TRUE(tmech::almost_equal(got_div, got_mul, tol));
}

} // namespace numsim::cas

#endif // TENSORTOSCALARDIVOPERATORTEST_H
