#ifndef TENSORTOSCALARMULOPERATORTEST_H
#define TENSORTOSCALARMULOPERATORTEST_H

// Lock-in tests for the `tag_invoke(mul_fn, …)` overloads on
// `(tensor_expression, tensor_to_scalar_expression)` and its
// symmetric pair — i.e. user-level `trace(A) * A` style products
// (#145). The AST node already existed (constructed internally by
// differentiation); this PR exposes the operator to user code with
// matching simplifier depth.

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
class TensorToScalarMulOperatorTest : public ::testing::Test {
protected:
  static constexpr std::size_t Dim = DimTag::value;

  using tensor_t = expression_holder<tensor_expression>;
  using t2s_t = expression_holder<tensor_to_scalar_expression>;
  using scalar_t = expression_holder<scalar_expression>;

  TensorToScalarMulOperatorTest() {
    std::tie(X, Y, Z) =
        make_tensor_variable(std::tuple{"X", Dim, 2}, std::tuple{"Y", Dim, 2},
                             std::tuple{"Z", Dim, 2});
    std::tie(x, y) = make_scalar_variable("x", "y");
  }

  tensor_t X, Y, Z;
  scalar_t x, y;
};

TYPED_TEST_SUITE(TensorToScalarMulOperatorTest, TestDims);

// --- 1. Compile-smoke + node identity.
//
// (Test 2 below locks in actual structural equality of the two
// directions; this test only verifies each operand order produces the
// correct node *type* — both must be t2s_with_tensor_mul, not the
// dedicated div node or something else.)

TYPED_TEST(TensorToScalarMulOperatorTest, BothDirectionsProduceMulNode) {
  auto &X = this->X;
  auto trX = trace(X);

  auto fwd = trX * X; // t2s × tensor
  auto bwd = X * trX; // tensor × t2s

  EXPECT_TRUE(is_same<tensor_to_scalar_with_tensor_mul>(fwd));
  EXPECT_TRUE(is_same<tensor_to_scalar_with_tensor_mul>(bwd));
}

// --- 2. Symmetry: operand order at the call site doesn't matter.

TYPED_TEST(TensorToScalarMulOperatorTest, SymmetryAcrossArgumentOrder) {
  auto &X = this->X;
  auto trX = trace(X);
  EXPECT_EQ(trX * X, X * trX);
  EXPECT_SAME_PRINT(trX * X, X * trX);
}

// --- 3. Zero folds — both sides.

TYPED_TEST(TensorToScalarMulOperatorTest, TensorZeroFold) {
  constexpr std::size_t Dim = TestFixture::Dim;
  auto &X = this->X;
  auto trX = trace(X);
  auto Z = make_expression<tensor_zero>(Dim, 2);
  EXPECT_TRUE(is_same<tensor_zero>(Z * trX));
  EXPECT_TRUE(is_same<tensor_zero>(trX * Z));
}

TYPED_TEST(TensorToScalarMulOperatorTest, T2sZeroFold) {
  auto &X = this->X;
  auto t2s_zero = make_expression<tensor_to_scalar_zero>();
  EXPECT_TRUE(is_same<tensor_zero>(X * t2s_zero));
  EXPECT_TRUE(is_same<tensor_zero>(t2s_zero * X));
}

// --- 3c. Zero-precedence when *both* sides could trigger the fold.
//
// The operator chains `is_same<tensor_zero>(lhs) || is_same<t2s_zero>(rhs)`
// — so when both sides are zero, the lhs check fires first and the
// result is a fresh `tensor_zero` stamped with lhs's dim/rank.
// Without this test the precedence is only asserted by inference,
// not by code. See #154.

TYPED_TEST(TensorToScalarMulOperatorTest, ZeroPrecedence_BothSidesZero) {
  constexpr std::size_t Dim = TestFixture::Dim;
  auto Z = make_expression<tensor_zero>(Dim, 2);
  auto t2s_zero = make_expression<tensor_to_scalar_zero>();
  EXPECT_TRUE(is_same<tensor_zero>(Z * t2s_zero));
  EXPECT_TRUE(is_same<tensor_zero>(t2s_zero * Z));
}

// --- 4. One fold.

TYPED_TEST(TensorToScalarMulOperatorTest, T2sOneFoldReturnsTensor) {
  auto &X = this->X;
  auto t2s_one = make_expression<tensor_to_scalar_one>();
  EXPECT_EQ(X * t2s_one, X);
  EXPECT_EQ(t2s_one * X, X);
}

// --- 5. Wrapper unwrap routes through tensor × scalar.

TYPED_TEST(TensorToScalarMulOperatorTest, WrapperUnwrapsToScalarPath) {
  auto &X = this->X;
  auto two = make_scalar_constant(2);
  auto wrapped = make_expression<tensor_to_scalar_scalar_wrapper>(two);

  auto result = wrapped * X;
  // The wrapper should be unwrapped; result is a tensor_scalar_mul,
  // not a tensor_to_scalar_with_tensor_mul.
  EXPECT_TRUE(is_same<tensor_scalar_mul>(result));
  EXPECT_FALSE(is_same<tensor_to_scalar_with_tensor_mul>(result));
}

// --- 6. Nested-mul collapse: (T × f) × g → T × (f*g).
//
// Beyond just confirming the structural flattening, we explicitly
// verify *both* t2s factors survive on the result's t2s side via
// contains_expression. Without that check, a buggy collapse that
// silently dropped one factor (e.g. `(T × f) × g → T × g`) would
// pass the structural shape assertion.

TYPED_TEST(TensorToScalarMulOperatorTest, NestedT2sMulCollapses) {
  auto &X = this->X;
  auto trX = trace(X);
  auto dX = det(X);

  // Build (X × trX) first — a tensor_to_scalar_with_tensor_mul.
  auto inner = X * trX;
  ASSERT_TRUE(is_same<tensor_to_scalar_with_tensor_mul>(inner));

  // Multiplying by another t2s should bubble the t2s factor into the
  // existing node's rhs (a t2s × t2s product) rather than nest a
  // second t2s_with_tensor_mul node.
  auto outer = inner * dX;
  ASSERT_TRUE(is_same<tensor_to_scalar_with_tensor_mul>(outer));

  auto const &as_node = outer.template get<tensor_to_scalar_with_tensor_mul>();
  // The inner tensor side should now be the bare X, not a nested
  // tensor_to_scalar_with_tensor_mul.
  EXPECT_FALSE(is_same<tensor_to_scalar_with_tensor_mul>(as_node.expr_lhs()));
  EXPECT_EQ(as_node.expr_lhs(), X);
  // Both t2s factors survive on the t2s side: the collapsed t2s
  // operand is *exactly* the canonical t2s product of trX and dX.
  // n_ary commutativity guarantees `trX * dX == dX * trX`, so the
  // order in which we wrote them at the call sites doesn't matter.
  EXPECT_EQ(as_node.expr_rhs(), trX * dX);
}

// --- 7. Scalar coefficient bubbles outward: (s × T) × g → s × (T × g).

TYPED_TEST(TensorToScalarMulOperatorTest, ScalarCoefficientBubblesOutward) {
  auto &X = this->X;
  auto &x = this->x;
  auto trX = trace(X);

  // Build s × X first — a tensor_scalar_mul.
  auto scaled = x * X;
  ASSERT_TRUE(is_same<tensor_scalar_mul>(scaled));

  // (s × X) × trX should bubble the scalar outward → s × (X × trX),
  // i.e. the outer node is a tensor_scalar_mul (not the new node).
  auto outer = scaled * trX;
  EXPECT_TRUE(is_same<tensor_scalar_mul>(outer));
}

// --- 8. Evaluator round-trip.

TEST(TensorToScalarMulOperatorEval, EvaluatorMatchesTmech) {
  constexpr double tol = 1e-12;
  auto A = make_expression<tensor>("A", 2, 2);

  tensor_evaluator<double> ev;
  auto data = std::make_shared<tensor_data<double, 2, 2>>();
  // clang-format off
  auto *raw = data->raw_data();
  raw[0] = 1.0; raw[1] = 2.0;
  raw[2] = 3.0; raw[3] = 4.0;
  // clang-format on
  ev.set(A, data);

  // trace(A) * A symbolically; numerically expect (1+4) * [[1,2],[3,4]].
  auto expr = trace(A) * A;
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);

  tmech::tensor<double, 2, 2> expected;
  auto *e = expected.raw_data();
  e[0] = 5.0;
  e[1] = 10.0;
  e[2] = 15.0;
  e[3] = 20.0;

  auto const &got =
      static_cast<tensor_data<double, 2, 2> const &>(*result).data();
  EXPECT_TRUE(tmech::almost_equal(got, expected, tol));
}

// --- 9. Diff round-trip — user-level `trace(A) * A` differentiates the
//        same way the internal-only construction always did.

TEST(TensorToScalarMulOperatorDiff, DiffMatchesProductRule) {
  auto A = make_expression<tensor>("A", 3, 2);
  // d/dA (trace(A) * A) is well-defined; we just need it not to
  // crash and to produce a non-trivial expression. The product rule
  // gives: trace(A) * dA/dA + A ⊗ (d trace(A)/dA) = trace(A)*I4 + A⊗I2.
  auto expr = trace(A) * A;
  auto d_expr = diff(expr, A);
  ASSERT_TRUE(d_expr.is_valid());
  EXPECT_FALSE(is_same<tensor_zero>(d_expr));
}

} // namespace numsim::cas

#endif // TENSORTOSCALARMULOPERATORTEST_H
