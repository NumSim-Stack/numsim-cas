#ifndef TENSOREVALUATORTEST_H
#define TENSOREVALUATORTEST_H

#include <gtest/gtest.h>
#include <memory>

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/diff.h>
#include <numsim_cas/eigen_decomposition.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor/visitors/tensor_evaluator.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_functions.h>

namespace numsim::cas {

// ---------------------------------------------------------------------------
// Audit #43 (2026-05-17): tensor_evaluator overload coverage verified.
// All 16 node types in NUMSIM_CAS_TENSOR_NODE_LIST have explicit operator()
// overrides in tensor_evaluator (header-only, see tensor/visitors/
// tensor_evaluator.h). Fallback uses static_assert(sizeof(T) == 0, ...) so
// a new node type without an override is a compile error.
//
// The issue body's concern about "apply() returns std::move(m_data) without
// initialization or visiting" is stale — the current apply() calls
// accept(*this) correctly and dispatches through the virtual visitor.
//
// All 16 node types have explicit EvalTensor* / Eval* lock-in tests below.
// ---------------------------------------------------------------------------

namespace {

using tensor_expr_t = expression_holder<tensor_expression>;

template <std::size_t Dim, std::size_t Rank>
auto make_test_data(std::initializer_list<double> values) {
  auto ptr = std::make_shared<tensor_data<double, Dim, Rank>>();
  auto *raw = ptr->raw_data();
  std::size_t i = 0;
  for (auto v : values)
    raw[i++] = v;
  return ptr;
}

template <std::size_t Dim, std::size_t Rank>
tmech::tensor<double, Dim, Rank>
make_tmech(std::initializer_list<double> values) {
  tmech::tensor<double, Dim, Rank> t;
  auto *raw = t.raw_data();
  std::size_t i = 0;
  for (auto v : values)
    raw[i++] = v;
  return t;
}

template <std::size_t Dim, std::size_t Rank>
auto const &as_tmech(tensor_data_base<double> const &data) {
  return static_cast<tensor_data<double, Dim, Rank> const &>(data).data();
}

constexpr double tol = 1e-12;

} // namespace

// --- Individual operator() tests ---

TEST(TensorEval, EvalTensorSymbol) {
  tensor_evaluator<double> ev;
  auto T = make_expression<tensor>("T", 2, 2);
  // clang-format off
  ev.set(T, make_test_data<2, 2>({1.0, 2.0,
                                   3.0, 4.0}));
  // clang-format on
  auto result = ev.apply(T);
  ASSERT_NE(result, nullptr);
  auto expected = make_tmech<2, 2>({1.0, 2.0, 3.0, 4.0});
  EXPECT_TRUE(tmech::almost_equal(as_tmech<2, 2>(*result), expected, tol));
}

TEST(TensorEval, EvalTensorZero) {
  tensor_evaluator<double> ev;
  auto Z = make_expression<tensor_zero>(3, 2);
  auto result = ev.apply(Z);
  ASSERT_NE(result, nullptr);
  tmech::tensor<double, 3, 2> expected;
  EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 2>(*result), expected, tol));
}

TEST(TensorEval, EvalKroneckerDelta) {
  tensor_evaluator<double> ev;
  auto delta = make_expression<identity_tensor>(3, std::size_t{2});
  auto result = ev.apply(delta);
  ASSERT_NE(result, nullptr);
  auto expected = tmech::eye<double, 3, 2>();
  EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 2>(*result), expected, tol));
}

TEST(TensorEval, EvalIdentityTensor2) {
  tensor_evaluator<double> ev;
  auto I = make_expression<identity_tensor>(2, 2);
  auto result = ev.apply(I);
  ASSERT_NE(result, nullptr);
  auto expected = tmech::eye<double, 2, 2>();
  EXPECT_TRUE(tmech::almost_equal(as_tmech<2, 2>(*result), expected, tol));
}

TEST(TensorEval, EvalTensorAdd) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto B = make_expression<tensor>("B", 2, 2);
  // clang-format off
  ev.set(A, make_test_data<2, 2>({1.0, 2.0,
                                   3.0, 4.0}));
  ev.set(B, make_test_data<2, 2>({5.0, 6.0,
                                   7.0, 8.0}));
  // clang-format on
  auto expr = A + B;
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto expected = make_tmech<2, 2>({6.0, 8.0, 10.0, 12.0});
  EXPECT_TRUE(tmech::almost_equal(as_tmech<2, 2>(*result), expected, tol));
}

TEST(TensorEval, EvalTensorNegative) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  // clang-format off
  ev.set(A, make_test_data<2, 2>({1.0, 2.0,
                                   3.0, 4.0}));
  // clang-format on
  auto expr = -A;
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto expected = make_tmech<2, 2>({-1.0, -2.0, -3.0, -4.0});
  EXPECT_TRUE(tmech::almost_equal(as_tmech<2, 2>(*result), expected, tol));
}

TEST(TensorEval, EvalTensorScalarMul) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  // clang-format off
  ev.set(A, make_test_data<2, 2>({1.0, 2.0,
                                   3.0, 4.0}));
  // clang-format on
  auto expr = make_scalar_constant(3) * A;
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto expected = make_tmech<2, 2>({3.0, 6.0, 9.0, 12.0});
  EXPECT_TRUE(tmech::almost_equal(as_tmech<2, 2>(*result), expected, tol));
}

TEST(TensorEval, EvalPermuteIndices) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  // clang-format off
  ev.set(A, make_test_data<2, 2>({1.0, 2.0,
                                   3.0, 4.0}));
  // clang-format on
  // trans(A) = permute_indices(A, {2,1})
  auto expr = trans(A);
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto A_val = make_tmech<2, 2>({1.0, 2.0, 3.0, 4.0});
  auto expected = tmech::eval(tmech::trans(A_val));
  EXPECT_TRUE(tmech::almost_equal(as_tmech<2, 2>(*result), expected, tol));
}

TEST(TensorEval, EvalInnerProduct) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto B = make_expression<tensor>("B", 2, 2);
  // clang-format off
  // A = [1 2; 3 4], B = [5 6; 7 8]
  ev.set(A, make_test_data<2, 2>({1.0, 2.0,
                                   3.0, 4.0}));
  ev.set(B, make_test_data<2, 2>({5.0, 6.0,
                                   7.0, 8.0}));
  // clang-format on
  // A * B (contract last index of A with first of B)
  auto expr = inner_product(A, sequence{2}, B, sequence{1});
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  // [1*5+2*7, 1*6+2*8; 3*5+4*7, 3*6+4*8] = [19 22; 43 50]
  auto expected = make_tmech<2, 2>({19.0, 22.0, 43.0, 50.0});
  EXPECT_TRUE(tmech::almost_equal(as_tmech<2, 2>(*result), expected, tol));
}

TEST(TensorEval, EvalOuterProduct) {
  tensor_evaluator<double> ev;
  auto u = make_expression<tensor>("u", 2, 1);
  auto v = make_expression<tensor>("v", 2, 1);
  ev.set(u, make_test_data<2, 1>({1.0, 2.0}));
  ev.set(v, make_test_data<2, 1>({3.0, 4.0}));
  // u ⊗ v
  auto expr = otimes(u, v);
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  // [1*3, 1*4; 2*3, 2*4] = [3 4; 6 8]
  auto u_val = make_tmech<2, 1>({1.0, 2.0});
  auto v_val = make_tmech<2, 1>({3.0, 4.0});
  auto expected = tmech::eval(tmech::otimes(u_val, v_val));
  EXPECT_TRUE(tmech::almost_equal(as_tmech<2, 2>(*result), expected, tol));
}

TEST(TensorEval, EvalDeviatoric) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  // Diagonal matrix: diag(6, 3, 3) -> tr=12, vol=4*I
  // clang-format off
  ev.set(A, make_test_data<3, 2>({6.0, 0.0, 0.0,
                                   0.0, 3.0, 0.0,
                                   0.0, 0.0, 3.0}));
  // clang-format on
  auto expr = dev(A);
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto A_val = make_tmech<3, 2>({6.0, 0.0, 0.0, 0.0, 3.0, 0.0, 0.0, 0.0, 3.0});
  auto expected = tmech::eval(tmech::dev(A_val));
  EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 2>(*result), expected, tol));
}

TEST(TensorEval, EvalInverse2x2) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  // clang-format off
  ev.set(A, make_test_data<2, 2>({1.0, 2.0,
                                   3.0, 4.0}));
  // clang-format on
  auto expr = inv(A);
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto A_val = make_tmech<2, 2>({1.0, 2.0, 3.0, 4.0});
  auto expected = tmech::eval(tmech::inv(A_val));
  EXPECT_TRUE(tmech::almost_equal(as_tmech<2, 2>(*result), expected, tol));
}

TEST(TensorEval, EvalSymmetry) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  // clang-format off
  ev.set(A, make_test_data<2, 2>({1.0, 2.0,
                                   4.0, 3.0}));
  // clang-format on
  auto sym_expr = sym(A);
  auto result = ev.apply(sym_expr);
  ASSERT_NE(result, nullptr);
  auto A_val = make_tmech<2, 2>({1.0, 2.0, 4.0, 3.0});
  auto expected = tmech::eval(tmech::sym(A_val));
  EXPECT_TRUE(tmech::almost_equal(as_tmech<2, 2>(*result), expected, tol));
}

TEST(TensorEval, EvalMissingSymbolThrows) {
  tensor_evaluator<double> ev;
  auto T = make_expression<tensor>("T", 2, 2);
  EXPECT_THROW(ev.apply(T), evaluation_error);
}

TEST(TensorEval, EvalMissingSymbolInCompoundExpr) {
  // Only A is set, B is missing — fails inside A + B
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto B = make_expression<tensor>("B", 2, 2);
  ev.set(A, make_test_data<2, 2>({1.0, 2.0, 3.0, 4.0}));
  EXPECT_THROW(ev.apply(A + B), evaluation_error);
}

TEST(TensorEval, EvalMissingScalarSymbolInScalarMul) {
  // Tensor A is set but scalar x is not — fails inside x * A
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto x = make_expression<scalar>("x");
  ev.set(A, make_test_data<2, 2>({1.0, 2.0, 3.0, 4.0}));
  EXPECT_THROW(ev.apply(x * A), evaluation_error);
}

TEST(TensorEval, EvalMissingSymbolInNestedExpr) {
  // Missing symbol buried inside trans(inv(T))
  tensor_evaluator<double> ev;
  auto T = make_expression<tensor>("T", 2, 2);
  EXPECT_THROW(ev.apply(trans(inv(T))), evaluation_error);
}

TEST(TensorEval, EvalT2sTensorMul) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto t2s_expr = trace(A);
  auto expr = make_expression<tensor_to_scalar_with_tensor_mul>(A, t2s_expr);
  // clang-format off
  ev.set(A, make_test_data<2, 2>({1.0, 2.0,
                                   3.0, 4.0}));
  // clang-format on
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto *raw = result->raw_data();
  // trace(A) = 1 + 4 = 5, so result = 5 * A
  EXPECT_NEAR(raw[0], 5.0, tol);
  EXPECT_NEAR(raw[1], 10.0, tol);
  EXPECT_NEAR(raw[2], 15.0, tol);
  EXPECT_NEAR(raw[3], 20.0, tol);
}

TEST(TensorEval, EvalProjectorSym) {
  tensor_evaluator<double> ev;
  auto expr = P_sym(3);
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  // P_sym is rank-4: (1/2)(δ_ik δ_jl + δ_il δ_jk)
  auto *raw = result->raw_data();
  // Check P_sym_{0000} = 1, P_sym_{0101} = 0.5, P_sym_{0110} = 0.5
  EXPECT_NEAR(raw[0 * 27 + 0 * 9 + 0 * 3 + 0], 1.0, tol);
  EXPECT_NEAR(raw[0 * 27 + 1 * 9 + 0 * 3 + 1], 0.5, tol);
  EXPECT_NEAR(raw[0 * 27 + 1 * 9 + 1 * 3 + 0], 0.5, tol);
  EXPECT_NEAR(raw[0 * 27 + 0 * 9 + 1 * 3 + 0], 0.0, tol);
}

TEST(TensorEval, EvalT2sTensorMul3D) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  auto t2s_expr = trace(A);
  auto expr = make_expression<tensor_to_scalar_with_tensor_mul>(A, t2s_expr);
  // clang-format off
  ev.set(A, make_test_data<3, 2>({1.0, 0.0, 0.0,
                                   0.0, 2.0, 0.0,
                                   0.0, 0.0, 3.0}));
  // clang-format on
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto *raw = result->raw_data();
  // trace(A) = 1 + 2 + 3 = 6, so result = 6 * A
  EXPECT_NEAR(raw[0], 6.0, tol);
  EXPECT_NEAR(raw[4], 12.0, tol);
  EXPECT_NEAR(raw[8], 18.0, tol);
  // off-diag stays zero
  EXPECT_NEAR(raw[1], 0.0, tol);
}

TEST(TensorEval, NotImplementedErrorIsCatchableAsCasError) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto t2s_expr = trace(A);
  auto expr = make_expression<tensor_to_scalar_with_tensor_mul>(A, t2s_expr);
  EXPECT_THROW(ev.apply(expr), cas_error);
}

TEST(TensorEval, NotImplementedErrorIsCatchableAsRuntimeError) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto t2s_expr = trace(A);
  auto expr = make_expression<tensor_to_scalar_with_tensor_mul>(A, t2s_expr);
  EXPECT_THROW(ev.apply(expr), std::runtime_error);
}

TEST(TensorEval, EvaluationErrorIsCatchableAsCasError) {
  tensor_evaluator<double> ev;
  auto T = make_expression<tensor>("T", 2, 2);
  EXPECT_THROW(ev.apply(T), cas_error);
}

TEST(TensorEval, EvaluationErrorIsCatchableAsRuntimeError) {
  tensor_evaluator<double> ev;
  auto T = make_expression<tensor>("T", 2, 2);
  EXPECT_THROW(ev.apply(T), std::runtime_error);
}

TEST(TensorEval, EvaluationErrorCarriesMessage) {
  tensor_evaluator<double> ev;
  auto T = make_expression<tensor>("T", 2, 2);
  try {
    ev.apply(T);
    FAIL() << "Expected evaluation_error";
  } catch (evaluation_error const &e) {
    EXPECT_TRUE(std::string(e.what()).find("symbol not found") !=
                std::string::npos);
  }
}

TEST(TensorEval, EvalT2sTensorMulMissingSymbol) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto t2s_expr = trace(A);
  auto expr = make_expression<tensor_to_scalar_with_tensor_mul>(A, t2s_expr);
  // No values set — evaluating should throw evaluation_error
  EXPECT_THROW(ev.apply(expr), evaluation_error);
}

// --- Combination tests ---

TEST(TensorEval, EvalAddSubtract) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto B = make_expression<tensor>("B", 2, 2);
  // clang-format off
  ev.set(A, make_test_data<2, 2>({5.0, 6.0,
                                   7.0, 8.0}));
  ev.set(B, make_test_data<2, 2>({1.0, 2.0,
                                   3.0, 4.0}));
  // clang-format on
  auto expr = A - B;
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto expected = make_tmech<2, 2>({4.0, 4.0, 4.0, 4.0});
  EXPECT_TRUE(tmech::almost_equal(as_tmech<2, 2>(*result), expected, tol));
}

TEST(TensorEval, EvalScalarMulWithVariable) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto x = make_expression<scalar>("x");
  // clang-format off
  ev.set(A, make_test_data<2, 2>({1.0, 2.0,
                                   3.0, 4.0}));
  // clang-format on
  ev.set_scalar(x, 2.0);
  auto expr = x * A;
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto expected = make_tmech<2, 2>({2.0, 4.0, 6.0, 8.0});
  EXPECT_TRUE(tmech::almost_equal(as_tmech<2, 2>(*result), expected, tol));
}

TEST(TensorEval, EvalInverse3x3) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  // clang-format off
  ev.set(A, make_test_data<3, 2>({2.0, 1.0, 1.0,
                                   1.0, 3.0, 2.0,
                                   1.0, 0.0, 0.0}));
  // clang-format on
  auto expr = inv(A);
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  // Verify A * inv(A) = I via tmech
  auto A_val = make_tmech<3, 2>({2.0, 1.0, 1.0, 1.0, 3.0, 2.0, 1.0, 0.0, 0.0});
  auto expected = tmech::eval(tmech::inv(A_val));
  EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 2>(*result), expected, tol));
}

// ─── Rank-4 inv dispatch (#248) ────────────────────────────────────────
// The evaluator picks tmech::inv (minor-symmetric) for Minor/MinorMajor-
// annotated rank-4 operands and tmech::invf (fully anisotropic)
// otherwise. Verify both branches numerically against tmech directly.

TEST(TensorEval, EvalInverseRank4MinorMajor) {
  // MinorMajor-annotated rank-4 routes to tmech::inv (Voigt 6×6) —
  // the major-pair swap makes the 9×9 representation rank-deficient
  // so tmech::invf is unstable for this input class. The split is
  // locked in by `Rank4InvDispatchSplitsOnMajorPair`.
  //
  // #283: rank-4 `tmech::inv` requires the `inverse_wrapper_base`
  // rewrite (petlenz/tmech commit db5d8aa or newer); the top-level
  // CMakeLists.txt uses the pinned FetchContent copy so the linked
  // tmech is known good.
  tensor_evaluator<double> ev;
  auto C = make_expression<tensor>("C", 3, 4);
  assume_minor_major(C);

  // Build a numerically PD rank-4 by hand: C = 2μ I_sym + λ I⊗I.
  // For an isotropic linear-elastic stiffness in dim=3, this gives a
  // matrix that's invertible in both conventions.
  tmech::tensor<double, 3, 2> I2;
  for (std::size_t i = 0; i < 3; ++i)
    for (std::size_t j = 0; j < 3; ++j)
      I2(i, j) = (i == j) ? 1.0 : 0.0;
  double const mu = 1.0, lam = 2.0;
  auto C_tmech = tmech::eval(2.0 * mu * tmech::otimesu(I2, I2) +
                             lam * tmech::otimes(I2, I2));

  auto C_data = std::make_shared<tensor_data<double, 3, 4>>();
  C_data->data() = C_tmech;
  ev.set(C, std::static_pointer_cast<tensor_data_base<double>>(C_data));

  auto result = ev.apply(inv(C));
  ASSERT_NE(result, nullptr);
  // Expected: tmech::inv (minor-symmetric default convention).
  auto expected = tmech::eval(tmech::inv(C_tmech));
  EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 4>(*result), expected, 1e-10));
}

TEST(TensorEval, EvalInverseRank4UnannotatedRoutesToInvf) {
  // No annotation → tmech::invf (fully anisotropic). Same numerical
  // tensor as above, but without the assume_minor_major call. The two
  // results should differ in general (different conventions), so the
  // assertion is "matches tmech::invf, NOT tmech::inv".
  tensor_evaluator<double> ev;
  auto C = make_expression<tensor>("C", 3, 4);
  // NO annotation — the evaluator must route to invf.

  tmech::tensor<double, 3, 2> I2;
  for (std::size_t i = 0; i < 3; ++i)
    for (std::size_t j = 0; j < 3; ++j)
      I2(i, j) = (i == j) ? 1.0 : 0.0;
  double const mu = 1.0, lam = 2.0;
  auto C_tmech = tmech::eval(2.0 * mu * tmech::otimesu(I2, I2) +
                             lam * tmech::otimes(I2, I2));

  auto C_data = std::make_shared<tensor_data<double, 3, 4>>();
  C_data->data() = C_tmech;
  ev.set(C, std::static_pointer_cast<tensor_data_base<double>>(C_data));

  auto result = ev.apply(inv(C));
  ASSERT_NE(result, nullptr);
  // Expected: tmech::invf (fully anisotropic).
  auto expected_invf = tmech::eval(tmech::invf(C_tmech));
  EXPECT_TRUE(
      tmech::almost_equal(as_tmech<3, 4>(*result), expected_invf, 1e-10))
      << "unannotated rank-4 inv must route to tmech::invf";
}

TEST(TensorEval, Rank4InvDispatchUsesInvUnlessMinorMajor) {
  // Lock-in: rank-4 inv dispatches to tmech::invf unless the input
  // collapses to the 6×6 Voigt symmetric form (MinorMajor):
  //   - MinorMajor only → tmech::inv (Voigt 6×6 symmetric, 21
  //     components). Plain 9×9 inversion is unstable here (Windows
  //     fuzz blow-up at seed 10036's `inner(..., inv(M_mm), ...)`).
  //   - Minor only (6×6 anisotropic, 36 components), Major only
  //     (9×9 symmetric, 45 components), unannotated, Skew → all
  //     tmech::invf. This is what unblocks seed-19 for Minor and
  //     M_maj coverage for Major.
  tmech::tensor<double, 3, 2> I2;
  for (std::size_t i = 0; i < 3; ++i)
    for (std::size_t j = 0; j < 3; ++j)
      I2(i, j) = (i == j) ? 1.0 : 0.0;
  auto C_tmech =
      tmech::eval(2.0 * tmech::otimes(I2, I2) + 1.0 * tmech::otimesu(I2, I2));

  // Materialize to concrete tensors so they share a common type and
  // can be used in a ternary inside the parametric check lambda. Bare
  // `tmech::eval(tmech::inv(...))` returns an expression-template
  // wrapper whose template arguments differ from
  // `tmech::eval(tmech::invf(...))`, which breaks the `?:` operand-
  // type unification (CI: build failure across gcc / clang / msvc).
  tmech::tensor<double, 3, 4> inv_data = tmech::inv(C_tmech);
  tmech::tensor<double, 3, 4> invf_data = tmech::invf(C_tmech);
  // Sanity: tmech::inv and tmech::invf disagree on this input so the
  // assertions below are discriminating.
  EXPECT_FALSE(tmech::almost_equal(inv_data, invf_data, 1e-10))
      << "Test setup assumes tmech::inv != tmech::invf on this input";

  auto C_data = std::make_shared<tensor_data<double, 3, 4>>();
  C_data->data() = C_tmech;

  auto check = [&](auto annotate_fn, std::string const &annotation_name,
                   bool expect_invf) {
    tensor_evaluator<double> ev;
    auto C = make_expression<tensor>("C", 3, 4);
    annotate_fn(C);
    ev.set(C, std::static_pointer_cast<tensor_data_base<double>>(C_data));
    auto result = ev.apply(inv(C));
    ASSERT_NE(result, nullptr);
    tmech::tensor<double, 3, 4> const &expected =
        expect_invf ? invf_data : inv_data;
    tmech::tensor<double, 3, 4> const &not_expected =
        expect_invf ? inv_data : invf_data;
    EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 4>(*result), expected, 1e-10))
        << annotation_name << "-annotated rank-4 routed to wrong inv";
    EXPECT_FALSE(
        tmech::almost_equal(as_tmech<3, 4>(*result), not_expected, 1e-10))
        << annotation_name
        << "-annotated rank-4 also matched the other inv "
           "(test premise violated)";
  };

  check([](auto const &) {}, "Unannotated", /*expect_invf=*/true);
  check([](auto const &C) { assume_minor(C); }, "Minor", true);
  check([](auto const &C) { assume_skew(C); }, "Skew", true);
  check([](auto const &C) { assume_major(C); }, "Major", true);
  // Only MinorMajor stays on tmech::inv (Voigt 6×6 symmetric).
  check([](auto const &C) { assume_minor_major(C); }, "MinorMajor",
        /*expect_invf=*/false);
}

// --- Compound expression tests ---
//
// tmech binary operators (+, -, scalar*) are not found via ADL inside
// namespace numsim::cas, so we compute expected values through a helper
// that closes the CAS namespace and reopens tmech's.

} // namespace numsim::cas

namespace tmech_test_helpers {

// Helpers for tmech binary ops — ADL finds tmech operators here.
template <typename A, typename B> auto add(A const &a, B const &b) {
  return tmech::eval(a + b);
}

template <typename A, typename B> auto sub(A const &a, B const &b) {
  return tmech::eval(a - b);
}

template <typename A> auto scale(double s, A const &a) {
  return tmech::eval(s * a);
}

template <typename A, typename B> auto matmul(A const &a, B const &b) {
  constexpr auto RankA = std::decay_t<A>::rank();
  return tmech::eval(
      tmech::inner_product<tmech::sequence<RankA>, tmech::sequence<1>>(a, b));
}

} // namespace tmech_test_helpers

namespace numsim::cas {

TEST(TensorEval, CompoundDevOfSum) {
  // dev(A + B)
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  auto B = make_expression<tensor>("B", 3, 2);
  // clang-format off
  auto A_val = make_tmech<3, 2>({1.0, 2.0, 3.0,
                                  4.0, 5.0, 6.0,
                                  7.0, 8.0, 9.0});
  auto B_val = make_tmech<3, 2>({9.0, 8.0, 7.0,
                                  6.0, 5.0, 4.0,
                                  3.0, 2.0, 1.0});
  // clang-format on
  ev.set(A, std::make_shared<tensor_data<double, 3, 2>>(A_val));
  ev.set(B, std::make_shared<tensor_data<double, 3, 2>>(B_val));
  auto expr = dev(A + B);
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto sum = tmech_test_helpers::add(A_val, B_val);
  auto expected = tmech::eval(tmech::dev(sum));
  EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 2>(*result), expected, tol));
}

TEST(TensorEval, CompoundTransOfInnerProduct) {
  // trans(A · B) where · = contraction on index 2↔1
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto B = make_expression<tensor>("B", 2, 2);
  // clang-format off
  auto A_val = make_tmech<2, 2>({1.0, 2.0,
                                  3.0, 4.0});
  auto B_val = make_tmech<2, 2>({5.0, 6.0,
                                  7.0, 8.0});
  // clang-format on
  ev.set(A, std::make_shared<tensor_data<double, 2, 2>>(A_val));
  ev.set(B, std::make_shared<tensor_data<double, 2, 2>>(B_val));
  auto AB = inner_product(A, sequence{2}, B, sequence{1});
  auto expr = trans(AB);
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto AB_val = tmech_test_helpers::matmul(A_val, B_val);
  auto expected = tmech::eval(tmech::trans(AB_val));
  EXPECT_TRUE(tmech::almost_equal(as_tmech<2, 2>(*result), expected, tol));
}

TEST(TensorEval, CompoundAddWithOuterProduct) {
  // A + u ⊗ v  (rank-2 + rank-2 from outer product of rank-1 vectors)
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  auto u = make_expression<tensor>("u", 3, 1);
  auto v = make_expression<tensor>("v", 3, 1);
  // clang-format off
  auto A_val = make_tmech<3, 2>({1.0, 0.0, 0.0,
                                  0.0, 2.0, 0.0,
                                  0.0, 0.0, 3.0});
  // clang-format on
  auto u_val = make_tmech<3, 1>({1.0, 2.0, 3.0});
  auto v_val = make_tmech<3, 1>({4.0, 5.0, 6.0});
  ev.set(A, std::make_shared<tensor_data<double, 3, 2>>(A_val));
  ev.set(u, std::make_shared<tensor_data<double, 3, 1>>(u_val));
  ev.set(v, std::make_shared<tensor_data<double, 3, 1>>(v_val));
  auto expr = A + otimes(u, v);
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto uv_val = tmech::eval(tmech::otimes(u_val, v_val));
  auto expected = tmech_test_helpers::add(A_val, uv_val);
  EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 2>(*result), expected, tol));
}

TEST(TensorEval, CompoundScalarSymMinusDev) {
  // 3 * sym(A) - dev(B)
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  auto B = make_expression<tensor>("B", 3, 2);
  // clang-format off
  auto A_val = make_tmech<3, 2>({1.0, 4.0, 6.0,
                                  2.0, 5.0, 8.0,
                                  3.0, 7.0, 9.0});
  auto B_val = make_tmech<3, 2>({9.0, 3.0, 1.0,
                                  3.0, 5.0, 2.0,
                                  1.0, 2.0, 4.0});
  // clang-format on
  ev.set(A, std::make_shared<tensor_data<double, 3, 2>>(A_val));
  ev.set(B, std::make_shared<tensor_data<double, 3, 2>>(B_val));
  auto sym_A = sym(A);
  auto expr = make_scalar_constant(3) * sym_A - dev(B);
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto sym_val = tmech::eval(tmech::sym(A_val));
  auto dev_val = tmech::eval(tmech::dev(B_val));
  auto expected =
      tmech_test_helpers::sub(tmech_test_helpers::scale(3.0, sym_val), dev_val);
  EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 2>(*result), expected, tol));
}

TEST(TensorEval, CompoundInvTimesMatrix) {
  // inv(A) · B via inner_product
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  auto B = make_expression<tensor>("B", 3, 2);
  // clang-format off
  auto A_val = make_tmech<3, 2>({2.0, 1.0, 0.0,
                                  1.0, 3.0, 1.0,
                                  0.0, 1.0, 2.0});
  auto B_val = make_tmech<3, 2>({1.0, 0.0, 0.0,
                                  0.0, 1.0, 0.0,
                                  0.0, 0.0, 1.0});
  // clang-format on
  ev.set(A, std::make_shared<tensor_data<double, 3, 2>>(A_val));
  ev.set(B, std::make_shared<tensor_data<double, 3, 2>>(B_val));
  auto invA = inv(A);
  auto expr = inner_product(invA, sequence{2}, B, sequence{1});
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto invA_val = tmech::eval(tmech::inv(A_val));
  auto expected = tmech_test_helpers::matmul(invA_val, B_val);
  EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 2>(*result), expected, tol));
}

TEST(TensorEval, CompoundLinearCombinationWithTrans) {
  // 3*A + 2*B - trans(A)
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto B = make_expression<tensor>("B", 2, 2);
  // clang-format off
  auto A_val = make_tmech<2, 2>({1.0, 2.0,
                                  3.0, 4.0});
  auto B_val = make_tmech<2, 2>({5.0, 6.0,
                                  7.0, 8.0});
  // clang-format on
  ev.set(A, std::make_shared<tensor_data<double, 2, 2>>(A_val));
  ev.set(B, std::make_shared<tensor_data<double, 2, 2>>(B_val));
  auto expr =
      make_scalar_constant(3) * A + make_scalar_constant(2) * B - trans(A);
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto t1 = tmech_test_helpers::scale(3.0, A_val);
  auto t2 = tmech_test_helpers::scale(2.0, B_val);
  auto transA = tmech::eval(tmech::trans(A_val));
  auto expected =
      tmech_test_helpers::sub(tmech_test_helpers::add(t1, t2), transA);
  EXPECT_TRUE(tmech::almost_equal(as_tmech<2, 2>(*result), expected, tol));
}

TEST(TensorEval, CompoundNestedDevSym) {
  // dev(sym(A)) simplifies to dev(A) at construction time (subspace rule).
  // Use symmetric A so evaluator short-circuit tmech::dev matches projector
  // algebra semantics.
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  // clang-format off
  auto A_val = make_tmech<3, 2>({5.0, 1.0, 2.0,
                                  1.0, 7.0, 4.0,
                                  2.0, 4.0, 9.0});
  // clang-format on
  ev.set(A, std::make_shared<tensor_data<double, 3, 2>>(A_val));
  auto expr = dev(sym(A));
  // dev(sym(A)) simplifies to dev(A) at construction
  auto result = ev.apply(expr);
  auto result_dev = ev.apply(dev(A));
  ASSERT_NE(result, nullptr);
  ASSERT_NE(result_dev, nullptr);
  auto expected = tmech::eval(tmech::dev(A_val));
  EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 2>(*result), expected, tol));
  EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 2>(*result),
                                  as_tmech<3, 2>(*result_dev), tol));
}

TEST(TensorEval, CompoundSubOuterPlusTrans) {
  // A - 2*(u ⊗ v) + trans(A)
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto u = make_expression<tensor>("u", 2, 1);
  auto v = make_expression<tensor>("v", 2, 1);
  // clang-format off
  auto A_val = make_tmech<2, 2>({10.0, 20.0,
                                  30.0, 40.0});
  // clang-format on
  auto u_val = make_tmech<2, 1>({1.0, 2.0});
  auto v_val = make_tmech<2, 1>({3.0, 4.0});
  ev.set(A, std::make_shared<tensor_data<double, 2, 2>>(A_val));
  ev.set(u, std::make_shared<tensor_data<double, 2, 1>>(u_val));
  ev.set(v, std::make_shared<tensor_data<double, 2, 1>>(v_val));
  auto expr = A - make_scalar_constant(2) * otimes(u, v) + trans(A);
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto uv_val = tmech::eval(tmech::otimes(u_val, v_val));
  auto scaled_uv = tmech_test_helpers::scale(2.0, uv_val);
  auto transA = tmech::eval(tmech::trans(A_val));
  auto expected = tmech_test_helpers::add(
      tmech_test_helpers::sub(A_val, scaled_uv), transA);
  EXPECT_TRUE(tmech::almost_equal(as_tmech<2, 2>(*result), expected, tol));
}

TEST(TensorEval, CompoundInvOfSymmetricSum) {
  // inv(A + trans(A)) — inverse of a symmetric matrix
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  // clang-format off
  auto A_val = make_tmech<3, 2>({4.0, 1.0, 0.0,
                                  2.0, 5.0, 1.0,
                                  0.0, 3.0, 6.0});
  // clang-format on
  ev.set(A, std::make_shared<tensor_data<double, 3, 2>>(A_val));
  auto expr = inv(A + trans(A));
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto transA = tmech::eval(tmech::trans(A_val));
  auto sum_val = tmech_test_helpers::add(A_val, transA);
  auto expected = tmech::eval(tmech::inv(sum_val));
  EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 2>(*result), expected, tol));
}

TEST(TensorEval, CompoundVolPlusDevEqualsOriginal) {
  // vol(A) + dev(A) simplifies to sym(A) at construction (projector addition).
  // For symmetric A, sym(A) == A, so the decomposition identity holds.
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  // clang-format off
  auto A_val = make_tmech<3, 2>({1.0, 2.0, 3.0,
                                  2.0, 5.0, 6.0,
                                  3.0, 6.0, 9.0});
  // clang-format on
  ev.set(A, std::make_shared<tensor_data<double, 3, 2>>(A_val));
  auto expr = vol(A) + dev(A);
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 2>(*result), A_val, tol));
}

TEST(TensorEval, CompoundMultipleInnerProducts) {
  // (A · B) · C  — chained matrix multiplication
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto B = make_expression<tensor>("B", 2, 2);
  auto C = make_expression<tensor>("C", 2, 2);
  // clang-format off
  auto A_val = make_tmech<2, 2>({1.0, 2.0,
                                  3.0, 4.0});
  auto B_val = make_tmech<2, 2>({2.0, 0.0,
                                  1.0, 3.0});
  auto C_val = make_tmech<2, 2>({1.0, 1.0,
                                  0.0, 2.0});
  // clang-format on
  ev.set(A, std::make_shared<tensor_data<double, 2, 2>>(A_val));
  ev.set(B, std::make_shared<tensor_data<double, 2, 2>>(B_val));
  ev.set(C, std::make_shared<tensor_data<double, 2, 2>>(C_val));
  auto AB = inner_product(A, sequence{2}, B, sequence{1});
  auto expr = inner_product(AB, sequence{2}, C, sequence{1});
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto AB_val = tmech_test_helpers::matmul(A_val, B_val);
  auto expected = tmech_test_helpers::matmul(AB_val, C_val);
  EXPECT_TRUE(tmech::almost_equal(as_tmech<2, 2>(*result), expected, tol));
}

TEST(TensorEval, CompoundOuterInnerMixed) {
  // (u ⊗ v) · w — contracts index 2 of (u⊗v) with index 1 of w
  tensor_evaluator<double> ev;
  auto u = make_expression<tensor>("u", 3, 1);
  auto v = make_expression<tensor>("v", 3, 1);
  auto w = make_expression<tensor>("w", 3, 1);
  auto u_val = make_tmech<3, 1>({1.0, 0.0, 0.0});
  auto v_val = make_tmech<3, 1>({0.0, 1.0, 0.0});
  auto w_val = make_tmech<3, 1>({2.0, 3.0, 4.0});
  ev.set(u, std::make_shared<tensor_data<double, 3, 1>>(u_val));
  ev.set(v, std::make_shared<tensor_data<double, 3, 1>>(v_val));
  ev.set(w, std::make_shared<tensor_data<double, 3, 1>>(w_val));
  auto uv = otimes(u, v);
  auto expr = inner_product(uv, sequence{2}, w, sequence{1});
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto uv_val = tmech::eval(tmech::otimes(u_val, v_val));
  auto expected = tmech_test_helpers::matmul(uv_val, w_val);
  EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 1>(*result), expected, tol));
}

// --- Projector tests ---

TEST(TensorEval, EvalSkew) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  // clang-format off
  auto A_val = make_tmech<3, 2>({1.0, 4.0, 6.0,
                                  2.0, 5.0, 8.0,
                                  3.0, 7.0, 9.0});
  // clang-format on
  ev.set(A, std::make_shared<tensor_data<double, 3, 2>>(A_val));
  auto expr = skew(A);
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto expected = tmech::eval(tmech::skew(A_val));
  EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 2>(*result), expected, tol));
}

TEST(TensorEval, EvalSymPlusSkewEqualsOriginal) {
  // sym(A) + skew(A) == A
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  // clang-format off
  auto A_val = make_tmech<3, 2>({1.0, 2.0, 3.0,
                                  4.0, 5.0, 6.0,
                                  7.0, 8.0, 9.0});
  // clang-format on
  ev.set(A, std::make_shared<tensor_data<double, 3, 2>>(A_val));
  auto expr = sym(A) + skew(A);
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 2>(*result), A_val, tol));
}

TEST(TensorEval, EvalProjectorDev) {
  tensor_evaluator<double> ev;
  auto expr = P_devi(3);
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  // P_dev = P_sym - P_vol
  // Check P_dev_{0000} = 1 - 1/3 = 2/3
  auto *raw = result->raw_data();
  EXPECT_NEAR(raw[0], 2.0 / 3.0, tol); // P_dev_{0000}
  // P_dev_{0011} should be -1/3 (vol subtracted from sym)
  EXPECT_NEAR(raw[0 * 27 + 0 * 9 + 1 * 3 + 1], -1.0 / 3.0, tol);
}

TEST(TensorEval, EvalProjectorVol) {
  tensor_evaluator<double> ev;
  auto expr = P_vol(3);
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto *raw = result->raw_data();
  // P_vol_{0000} = 1/3
  EXPECT_NEAR(raw[0], 1.0 / 3.0, tol);
  // P_vol_{0011} = 1/3
  EXPECT_NEAR(raw[0 * 27 + 0 * 9 + 1 * 3 + 1], 1.0 / 3.0, tol);
  // P_vol_{0100} = 0
  EXPECT_NEAR(raw[0 * 27 + 1 * 9 + 0 * 3 + 0], 0.0, tol);
}

TEST(TensorEval, EvalProjectorSkew) {
  tensor_evaluator<double> ev;
  auto expr = P_skew(2);
  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  auto *raw = result->raw_data();
  // P_skew_{0000} = 0 (diag entries zero because (1+(-1))/2 = 0)
  EXPECT_NEAR(raw[0], 0.0, tol);
  // P_skew_{0101} = 0.5
  EXPECT_NEAR(raw[0 * 8 + 1 * 4 + 0 * 2 + 1], 0.5, tol);
  // P_skew_{0110} = -0.5
  EXPECT_NEAR(raw[0 * 8 + 1 * 4 + 1 * 2 + 0], -0.5, tol);
}

TEST(TensorEval, EvalGenericProjectorContraction) {
  // Generic path: construct rank-4 projector and contract with tensor
  // P_vol : A should equal vol(A)
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  // clang-format off
  auto A_val = make_tmech<3, 2>({6.0, 0.0, 0.0,
                                  0.0, 3.0, 0.0,
                                  0.0, 0.0, 3.0});
  // clang-format on
  ev.set(A, std::make_shared<tensor_data<double, 3, 2>>(A_val));
  // Evaluate vol(A) which uses the short-circuit path
  auto result_vol = ev.apply(vol(A));
  auto expected_vol = tmech::eval(tmech::vol(A_val));
  ASSERT_NE(result_vol, nullptr);
  EXPECT_TRUE(
      tmech::almost_equal(as_tmech<3, 2>(*result_vol), expected_vol, tol));
}

TEST(TensorEval, EvalStandaloneProjectorExpression) {
  // C = 3*K*P_vol(d) + 2*G*P_dev(d) creates valid expression tree
  auto K = make_expression<scalar>("K");
  auto G = make_expression<scalar>("G");
  auto C_expr = make_scalar_constant(3) * K * P_vol(3) +
                make_scalar_constant(2) * G * P_devi(3);
  // Just verify the expression tree is valid (doesn't crash)
  tensor_evaluator<double> ev;
  ev.set_scalar(K, 100.0);
  ev.set_scalar(G, 50.0);
  auto result = ev.apply(C_expr);
  ASSERT_NE(result, nullptr);
  // Spot-check: C_{0000} = 3K*(1/3) + 2G*(2/3) = K + 4G/3 = 100 + 200/3
  auto *raw = result->raw_data();
  EXPECT_NEAR(raw[0], 100.0 + 200.0 / 3.0, tol);
}

// --- Projector algebra simplifier tests ---

TEST(TensorProjAlgebra, IdempotentDevDev) {
  // dev(dev(A)) simplifies to dev(A) at construction time
  auto A = make_expression<tensor>("A", 3, 2);
  auto dev_dev_A = dev(dev(A));
  auto dev_A = dev(A);

  // Construction-time simplification: single inner_product_wrapper, not nested
  EXPECT_EQ(dev_dev_A.get().hash_value(), dev_A.get().hash_value());

  // Verify via evaluation
  // clang-format off
  auto A_val = make_tmech<3, 2>({1.0, 2.0, 3.0,
                                  2.0, 5.0, 6.0,
                                  3.0, 6.0, 9.0});
  // clang-format on
  tensor_evaluator<double> ev;
  ev.set(A, std::make_shared<tensor_data<double, 3, 2>>(A_val));

  auto result_dd = ev.apply(dev_dev_A);
  auto result_d = ev.apply(dev_A);

  ASSERT_NE(result_dd, nullptr);
  ASSERT_NE(result_d, nullptr);
  EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 2>(*result_dd),
                                  as_tmech<3, 2>(*result_d), tol));
}

TEST(TensorProjAlgebra, OrthogonalDevVol) {
  // vol(dev(A)) simplifies to zero at construction time (orthogonal projectors)
  auto A = make_expression<tensor>("A", 3, 2);
  auto expr = vol(dev(A));

  // Construction-time simplification produces tensor_zero
  EXPECT_TRUE(is_same<tensor_zero>(expr));
}

TEST(TensorProjAlgebra, SubspaceDevSym) {
  // dev(sym(A)) simplifies to dev(A) at construction time (subspace rule)
  auto A = make_expression<tensor>("A", 3, 2);
  auto dev_sym_A = dev(sym(A));
  auto dev_A = dev(A);

  // Construction-time simplification: same expression tree
  EXPECT_EQ(dev_sym_A.get().hash_value(), dev_A.get().hash_value());

  // Verify via evaluation (use symmetric A for evaluator consistency)
  // clang-format off
  auto A_val = make_tmech<3, 2>({1.0, 2.0, 3.0,
                                  2.0, 5.0, 6.0,
                                  3.0, 6.0, 9.0});
  // clang-format on
  tensor_evaluator<double> ev;
  ev.set(A, std::make_shared<tensor_data<double, 3, 2>>(A_val));

  auto result_ds = ev.apply(dev_sym_A);
  auto result_d = ev.apply(dev_A);
  ASSERT_NE(result_ds, nullptr);
  ASSERT_NE(result_d, nullptr);
  EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 2>(*result_ds),
                                  as_tmech<3, 2>(*result_d), tol));
}

TEST(TensorProjAlgebra, AdditionVolDevEqualsSymViaEval) {
  // vol(A) + dev(A) → sym(A) via binary add simplifier at construction time
  auto A = make_expression<tensor>("A", 3, 2);
  auto expr = vol(A) + dev(A);
  auto sym_A = sym(A);

  // Construction-time simplification: same expression tree as sym(A)
  EXPECT_EQ(expr.get().hash_value(), sym_A.get().hash_value());

  // Verify via evaluation (use symmetric A for evaluator consistency)
  // clang-format off
  auto A_val = make_tmech<3, 2>({1.0, 2.0, 3.0,
                                  2.0, 5.0, 6.0,
                                  3.0, 6.0, 9.0});
  // clang-format on
  tensor_evaluator<double> ev;
  ev.set(A, std::make_shared<tensor_data<double, 3, 2>>(A_val));

  auto result_sum = ev.apply(expr);
  auto result_sym = ev.apply(sym_A);
  ASSERT_NE(result_sum, nullptr);
  ASSERT_NE(result_sym, nullptr);
  EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 2>(*result_sum),
                                  as_tmech<3, 2>(*result_sym), tol));
}

TEST(TensorProjAlgebra, AdditionSymSkewEqualsIdentityViaEval) {
  // sym(A) + skew(A) → A (identity) via binary add simplifier at construction
  auto A = make_expression<tensor>("A", 3, 2);
  auto expr = sym(A) + skew(A);

  // Construction-time simplification: should be the bare symbol A
  EXPECT_EQ(expr.get().hash_value(), A.get().hash_value());

  // Verify via evaluation
  // clang-format off
  auto A_val = make_tmech<3, 2>({1.0, 2.0, 3.0,
                                  4.0, 5.0, 6.0,
                                  7.0, 8.0, 9.0});
  // clang-format on
  tensor_evaluator<double> ev;
  ev.set(A, std::make_shared<tensor_data<double, 3, 2>>(A_val));

  auto result = ev.apply(expr);
  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(tmech::almost_equal(as_tmech<3, 2>(*result), A_val, tol));
}

// --- Projector hash tests ---

TEST(TensorProjAlgebra, ProjectorHashDistinguishesYoungBlocks) {
  auto p1 = make_projector(3, 3, Young{{{1, 2}, {3}}}, AnyTraceTag{});
  auto p2 = make_projector(3, 3, Young{{{1, 3}, {2}}}, AnyTraceTag{});
  EXPECT_NE(p1.get().hash_value(), p2.get().hash_value());
}

TEST(TensorProjAlgebra, ProjectorHashDistinguishesPartialTrace) {
  auto p1 = make_projector(3, 2, Symmetric{}, PartialTraceTag{{{1, 2}}});
  auto p2 = make_projector(3, 2, Symmetric{}, PartialTraceTag{{{1, 3}}});
  EXPECT_NE(p1.get().hash_value(), p2.get().hash_value());
}

TEST(TensorProjAlgebra, ProjectorHashSameForIdenticalSpaces) {
  auto p1 = make_projector(3, 3, Young{{{1, 2}, {3}}}, AnyTraceTag{});
  auto p2 = make_projector(3, 3, Young{{{1, 2}, {3}}}, AnyTraceTag{});
  EXPECT_EQ(p1.get().hash_value(), p2.get().hash_value());
}

// ─── if_then_else lazy-evaluation lock-in (#242) ─────────────────
// Mirror of the scalar IfThenElseEvaluatorLazyOnUnselectedBranch test.
// The unselected branch references an unbound tensor symbol — eager
// evaluation would throw evaluation_error trying to look it up.
// Lazy evaluation skips it entirely.
TEST(TensorEval, IfThenElseLazyOnUnselectedBranch) {
  tensor_evaluator<double> ev;
  auto x = make_expression<scalar>("x");
  auto A = make_expression<tensor>("A", 2, 2); // deliberately UNBOUND
  auto _zero = make_expression<scalar_constant>(0.0);
  auto Z = make_expression<tensor_zero>(std::size_t{2}, std::size_t{2});

  // cond = ge(x, 0). At x=1 → cond truthy → then = Z is selected.
  // The else branch is inv(A) — accessing the unbound A would throw.
  ev.set_scalar(x, 1.0);
  auto expr_then_safe = if_then_else(ge(x, _zero), Z, inv(A));
  auto result = ev.apply(expr_then_safe);
  ASSERT_NE(result, nullptr);
  auto expected = make_tmech<2, 2>({0.0, 0.0, 0.0, 0.0});
  EXPECT_TRUE(tmech::almost_equal(as_tmech<2, 2>(*result), expected, tol));

  // cond = ge(x, 0). At x=-1 → cond falsy → else = Z is selected.
  // The then branch is inv(A) — same unbound-symbol trap, on the
  // other arm.
  ev.set_scalar(x, -1.0);
  auto expr_else_safe = if_then_else(ge(x, _zero), inv(A), Z);
  auto result2 = ev.apply(expr_else_safe);
  ASSERT_NE(result2, nullptr);
  EXPECT_TRUE(tmech::almost_equal(as_tmech<2, 2>(*result2), expected, tol));
}

// ─── Eigenprojection E_i = n_i ⊗ n_i (#226) ───────────────────────────

TEST(TensorEval, EigenprojectionDiagonal3x3) {
  // diag(5,3,2): eigenpairs sorted ascending are (2,ẑ),(3,ŷ),(5,x̂), so
  // E_0 = diag(0,0,1), E_1 = diag(0,1,0), E_2 = diag(1,0,0).
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  // clang-format off
  ev.set(A, make_test_data<3, 2>({5.0, 0.0, 0.0,
                                   0.0, 3.0, 0.0,
                                   0.0, 0.0, 2.0}));
  // clang-format on
  auto E0 = ev.apply(eigen_decomposition(A).basis(0));
  auto E1 = ev.apply(eigen_decomposition(A).basis(1));
  auto E2 = ev.apply(eigen_decomposition(A).basis(2));
  ASSERT_NE(E0, nullptr);
  EXPECT_TRUE(tmech::almost_equal(
      as_tmech<3, 2>(*E0), make_tmech<3, 2>({0, 0, 0, 0, 0, 0, 0, 0, 1}), tol));
  EXPECT_TRUE(tmech::almost_equal(
      as_tmech<3, 2>(*E1), make_tmech<3, 2>({0, 0, 0, 0, 1, 0, 0, 0, 0}), tol));
  EXPECT_TRUE(tmech::almost_equal(
      as_tmech<3, 2>(*E2), make_tmech<3, 2>({1, 0, 0, 0, 0, 0, 0, 0, 0}), tol));
}

TEST(TensorEval, EigenprojectionCompletenessAndReconstruction) {
  // Non-diagonal symmetric A: Σ E_i = I, and A = Σ λ_i E_i (spectral thm).
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  // clang-format off
  ev.set(A, make_test_data<3, 2>({2.0, 1.0, 0.0,
                                   1.0, 3.0, 1.0,
                                   0.0, 1.0, 2.0}));
  // clang-format on
  auto E0 = as_tmech<3, 2>(*ev.apply(eigen_decomposition(A).basis(0)));
  auto E1 = as_tmech<3, 2>(*ev.apply(eigen_decomposition(A).basis(1)));
  auto E2 = as_tmech<3, 2>(*ev.apply(eigen_decomposition(A).basis(2)));

  auto I = tmech::eval(tmech::eye<double, 3, 2>());
  EXPECT_TRUE(tmech::almost_equal(tmech::eval(E0 + E1 + E2), I, tol));

  // λ_i = A : E_i (double contraction), so A = Σ λ_i E_i must recover A.
  auto A_val = make_tmech<3, 2>({2, 1, 0, 1, 3, 1, 0, 1, 2});
  double l0 = tmech::dcontract(A_val, E0);
  double l1 = tmech::dcontract(A_val, E1);
  double l2 = tmech::dcontract(A_val, E2);
  EXPECT_TRUE(tmech::almost_equal(tmech::eval(l0 * E0 + l1 * E1 + l2 * E2),
                                  A_val, tol));
}

TEST(TensorEval, EigenvectorMatchesProjectionAndIsUnit) {
  // normal(i) is a rank-1 unit eigenvector; its outer product with itself
  // must reproduce the (independently-validated) eigenprojection basis(i),
  // which also pins |n_i| = 1 (tr(n⊗n) = |n|²). Sign-independent by
  // construction (n⊗n is sign-free).
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  // clang-format off
  ev.set(A, make_test_data<3, 2>({2.0, 1.0, 0.0,
                                   1.0, 3.0, 1.0,
                                   0.0, 1.0, 2.0}));
  // clang-format on
  eigen_decomposition eig(A);
  for (std::size_t i = 0; i < 3; ++i) {
    auto n_data = ev.apply(eig.normal(i));
    auto E_data = ev.apply(eig.basis(i));
    ASSERT_NE(n_data, nullptr);
    ASSERT_NE(E_data, nullptr);
    EXPECT_EQ(n_data->rank(), std::size_t{1});
    // Keep n_data/E_data alive: as_tmech returns a reference into them.
    auto const &n = as_tmech<3, 1>(*n_data);
    auto const &E = as_tmech<3, 2>(*E_data);
    EXPECT_TRUE(tmech::almost_equal(tmech::eval(tmech::otimes(n, n)), E, tol))
        << "n_" << i << " ⊗ n_" << i << " != E_" << i;
  }
}

TEST(TensorEval, EigenvectorDiagonalAxes) {
  // diag(5,3,2): ascending eigenpairs are (2,ẑ),(3,ŷ),(5,x̂). Eigenvectors
  // are the coordinate axes up to sign, so n_i ⊗ n_i are the axis
  // projections regardless of the ± convention.
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  // clang-format off
  ev.set(A, make_test_data<3, 2>({5.0, 0.0, 0.0,
                                   0.0, 3.0, 0.0,
                                   0.0, 0.0, 2.0}));
  // clang-format on
  eigen_decomposition eig(A);
  auto n0_data = ev.apply(eig.normal(0)); // keep alive: as_tmech aliases it
  auto const &n0 = as_tmech<3, 1>(*n0_data);
  EXPECT_TRUE(tmech::almost_equal(tmech::eval(tmech::otimes(n0, n0)),
                                  make_tmech<3, 2>({0, 0, 0, 0, 0, 0, 0, 0, 1}),
                                  tol));
}

TEST(TensorEval, EigenRepeatedEigenvalue) {
  // A = [[3,1,1],[1,3,1],[1,1,3]] has eigenvalues {2, 2, 5}: 5 for the
  // (1,1,1) direction, 2 (doubled) on its orthogonal complement. Verifies
  // the wrappers stay correct across the eigenvalue degeneracy — where the
  // per-index eigenvector/projection split is arbitrary but the
  // basis-invariant identities must still hold.
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  // clang-format off
  ev.set(A, make_test_data<3, 2>({3.0, 1.0, 1.0,
                                   1.0, 3.0, 1.0,
                                   1.0, 1.0, 3.0}));
  // clang-format on
  eigen_decomposition eig(A);

  auto E0d = ev.apply(eig.basis(0));
  auto E1d = ev.apply(eig.basis(1));
  auto E2d = ev.apply(eig.basis(2));
  auto const &E0 = as_tmech<3, 2>(*E0d);
  auto const &E1 = as_tmech<3, 2>(*E1d);
  auto const &E2 = as_tmech<3, 2>(*E2d);

  // Eigenvalues via Rayleigh quotient λ_i = A : E_i — ascending 2, 2, 5.
  auto A_val = make_tmech<3, 2>({3, 1, 1, 1, 3, 1, 1, 1, 3});
  EXPECT_NEAR(tmech::dcontract(A_val, E0), 2.0, 1e-10);
  EXPECT_NEAR(tmech::dcontract(A_val, E1), 2.0, 1e-10);
  EXPECT_NEAR(tmech::dcontract(A_val, E2), 5.0, 1e-10);

  // Completeness holds despite the degenerate split.
  auto I = tmech::eval(tmech::eye<double, 3, 2>());
  EXPECT_TRUE(tmech::almost_equal(tmech::eval(E0 + E1 + E2), I, 1e-10));

  // Spectral reconstruction: 2·E0 + 2·E1 + 5·E2 == A.
  EXPECT_TRUE(tmech::almost_equal(tmech::eval(2.0 * E0 + 2.0 * E1 + 5.0 * E2),
                                  A_val, 1e-10));

  // The λ=5 projection is unique (multiplicity 1): (1/3)·ones.
  double t{1.0 / 3.0};
  EXPECT_TRUE(tmech::almost_equal(
      E2, make_tmech<3, 2>({t, t, t, t, t, t, t, t, t}), 1e-9));

  // The λ=2 eigenspace projector E0+E1 is unique too: I − E2.
  EXPECT_TRUE(
      tmech::almost_equal(tmech::eval(E0 + E1), tmech::eval(I - E2), 1e-9));
}

TEST(TensorEval, EigenprojectionDerivativeMatchesFiniteDiff) {
  // Validate ∂E_a/∂A (the 4th-order spectral derivative) against a central
  // finite difference of E_a, on a symmetric A with well-separated
  // eigenvalues {1, 2.382, 4.618}. For a symmetric direction H:
  //   (∂E_a/∂A : H)  ==  d/dt E_a(A + t H) |_{t=0}.
  using T2 = tmech::tensor<double, 3, 2>;
  T2 A_val;
  A_val = {4.0, 1.0, 0.0, 1.0, 3.0, 0.0, 0.0, 0.0, 1.0};
  T2 H;
  H = {0.1, 0.2, 0.3, 0.2, 0.4, 0.1, 0.3, 0.1, 0.5}; // symmetric direction

  auto data_from = [](T2 const &t) {
    auto ptr = std::make_shared<tensor_data<double, 3, 2>>();
    auto *dst = ptr->raw_data();
    auto const *src = t.raw_data();
    for (std::size_t i = 0; i < 9; ++i)
      dst[i] = src[i];
    return ptr;
  };

  auto A = make_expression<tensor>("A", 3, 2);
  tensor_evaluator<double> ev;
  eigen_decomposition eig(A);

  for (std::size_t a = 0; a < 3; ++a) {
    // Symbolic ∂E_a/∂A — differentiating the eigenprojection w.r.t. A.
    auto dEa = diff(eig.basis(a), A);
    ASSERT_TRUE(dEa.is_valid());
    EXPECT_EQ(dEa.get().rank(), std::size_t{4});

    // Analytical directional derivative D : H.
    ev.set(A, data_from(A_val));
    auto D_data = ev.apply(dEa);
    auto const &D = as_tmech<3, 4>(*D_data);
    auto analytic = tmech::eval(tmech::dcontract(D, H));

    // Central finite difference of E_a along H.
    const double t = 1e-6;
    T2 Aplus = tmech::eval(A_val + t * H);
    T2 Aminus = tmech::eval(A_val - t * H);
    ev.set(A, data_from(Aplus));
    auto Ep_data = ev.apply(eig.basis(a));
    auto const &Ep = as_tmech<3, 2>(*Ep_data);
    ev.set(A, data_from(Aminus));
    auto Em_data = ev.apply(eig.basis(a));
    auto const &Em = as_tmech<3, 2>(*Em_data);
    auto fd = tmech::eval((Ep - Em) / (2.0 * t));

    EXPECT_TRUE(tmech::almost_equal(analytic, fd, 1e-6))
        << "∂E_" << a << "/∂A : H mismatch vs finite difference";
  }
}

TEST(TensorEval, SpectralEnergyStressAndTangent) {
  // End-to-end spectral chain via eigenvalues + dE/dA. For the isotropic
  // energy ψ = Σ λ_i² (= tr(A²)) built purely from eigenvalue nodes:
  //   stress  S = dψ/dA         = Σ 2λ_i E_i = 2A
  //   tangent C = dS/dA,  C : H = 2H   (for symmetric H)
  // Known closed forms, but the computation flows through the eigenvalue
  // derivative (dλ/dA = E_i) and the eigenprojection derivative (dE/dA).
  using T2 = tmech::tensor<double, 3, 2>;
  T2 A_val;
  A_val = {4.0, 1.0, 0.0, 1.0, 3.0, 0.0, 0.0, 0.0, 1.0};
  T2 H;
  H = {0.1, 0.2, 0.3, 0.2, 0.4, 0.1, 0.3, 0.1, 0.5};

  auto data_from = [](T2 const &t) {
    auto ptr = std::make_shared<tensor_data<double, 3, 2>>();
    auto *dst = ptr->raw_data();
    auto const *src = t.raw_data();
    for (std::size_t i = 0; i < 9; ++i)
      dst[i] = src[i];
    return ptr;
  };

  auto A = make_expression<tensor>("A", 3, 2);
  tensor_evaluator<double> ev;
  ev.set(A, data_from(A_val));
  eigen_decomposition eig(A);

  auto psi = eig.value(0) * eig.value(0) + eig.value(1) * eig.value(1) +
             eig.value(2) * eig.value(2);
  auto S = diff(psi, A); // stress, rank-2
  ASSERT_TRUE(S.is_valid());

  auto S_data = ev.apply(S);
  auto const &S_num = as_tmech<3, 2>(*S_data);
  EXPECT_TRUE(tmech::almost_equal(S_num, tmech::eval(2.0 * A_val), 1e-9))
      << "dψ/dA should be 2A";

  auto C = diff(S, A); // tangent, rank-4
  ASSERT_TRUE(C.is_valid());
  EXPECT_EQ(C.get().rank(), std::size_t{4});

  auto C_data = ev.apply(C);
  auto const &C_num = as_tmech<3, 4>(*C_data);
  auto CH = tmech::eval(tmech::dcontract(C_num, H));
  EXPECT_TRUE(tmech::almost_equal(CH, tmech::eval(2.0 * H), 1e-7))
      << "C : H should be 2H";
}

TEST(TensorEval, EigenprojectionDerivativeMinorSymmetric) {
  // ∂E_a/∂A must be minor-symmetric: E_a depends on A only through sym(A)
  // (the evaluator symmetrises), so a NON-symmetric increment H probes the
  // same response as sym(H). This test fails for the bare-otimesu form and
  // passes only for the minor-symmetric ⊙ form.
  using T2 = tmech::tensor<double, 3, 2>;
  T2 A_val;
  A_val = {4.0, 1.0, 0.0, 1.0, 3.0, 0.0, 0.0, 0.0, 1.0};
  T2 Hns; // deliberately NON-symmetric
  Hns = {0.1, 0.3, 0.2, 0.0, 0.4, 0.5, 0.6, 0.1, 0.2};

  auto data_from = [](T2 const &t) {
    auto ptr = std::make_shared<tensor_data<double, 3, 2>>();
    auto *dst = ptr->raw_data();
    auto const *src = t.raw_data();
    for (std::size_t i = 0; i < 9; ++i)
      dst[i] = src[i];
    return ptr;
  };

  auto A = make_expression<tensor>("A", 3, 2);
  tensor_evaluator<double> ev;
  eigen_decomposition eig(A);

  for (std::size_t a = 0; a < 3; ++a) {
    auto dEa = diff(eig.basis(a), A);
    ev.set(A, data_from(A_val));
    auto D_data = ev.apply(dEa);
    auto const &D = as_tmech<3, 4>(*D_data);
    auto analytic = tmech::eval(tmech::dcontract(D, Hns));

    // Central FD along the non-symmetric Hns; the evaluator symmetrises
    // the input internally, so this measures the response to sym(Hns).
    const double t = 1e-6;
    T2 Aplus = tmech::eval(A_val + t * Hns);
    T2 Aminus = tmech::eval(A_val - t * Hns);
    ev.set(A, data_from(Aplus));
    auto Ep_data = ev.apply(eig.basis(a));
    auto const &Ep = as_tmech<3, 2>(*Ep_data);
    ev.set(A, data_from(Aminus));
    auto Em_data = ev.apply(eig.basis(a));
    auto const &Em = as_tmech<3, 2>(*Em_data);
    auto fd = tmech::eval((Ep - Em) / (2.0 * t));

    EXPECT_TRUE(tmech::almost_equal(analytic, fd, 1e-6))
        << "∂E_" << a << "/∂A is not minor-symmetric (fails on non-sym H)";
  }
}

TEST(TensorEval, EigenprojectionDerivativeWrtScalar) {
  // dE_a/ds for A(s) = A0 + s·A1 — exercises the scalar-derivative path
  // (∂E_a/∂A : dA/ds), validated against a central FD in s.
  using T2 = tmech::tensor<double, 3, 2>;
  T2 A0, A1;
  A0 = {4.0, 1.0, 0.0, 1.0, 3.0, 0.0, 0.0, 0.0, 1.0};
  A1 = {0.0, 0.0, 0.5, 0.0, 0.0, 0.3, 0.5, 0.3, 0.0}; // symmetric
  const double s0 = 0.5;

  auto data_from = [](T2 const &t) {
    auto ptr = std::make_shared<tensor_data<double, 3, 2>>();
    auto *dst = ptr->raw_data();
    auto const *src = t.raw_data();
    for (std::size_t i = 0; i < 9; ++i)
      dst[i] = src[i];
    return ptr;
  };

  auto s = make_expression<scalar>("s");
  auto A0e = make_expression<tensor>("A0", 3, 2);
  auto A1e = make_expression<tensor>("A1", 3, 2);
  auto A = A0e + s * A1e; // scalar-dependent tensor
  eigen_decomposition eig(A);

  tensor_evaluator<double> ev;
  ev.set(A0e, data_from(A0));
  ev.set(A1e, data_from(A1));

  for (std::size_t a = 0; a < 3; ++a) {
    auto dEa_ds = diff(eig.basis(a), s); // rank-2
    ASSERT_TRUE(dEa_ds.is_valid());
    EXPECT_EQ(dEa_ds.get().rank(), std::size_t{2});

    ev.set_scalar(s, s0);
    auto D_data = ev.apply(dEa_ds);
    auto const &analytic = as_tmech<3, 2>(*D_data);

    const double t = 1e-6;
    ev.set_scalar(s, s0 + t);
    auto Ep_data = ev.apply(eig.basis(a));
    auto const &Ep = as_tmech<3, 2>(*Ep_data);
    ev.set_scalar(s, s0 - t);
    auto Em_data = ev.apply(eig.basis(a));
    auto const &Em = as_tmech<3, 2>(*Em_data);
    auto fd = tmech::eval((Ep - Em) / (2.0 * t));

    EXPECT_TRUE(tmech::almost_equal(tmech::eval(analytic), fd, 1e-6))
        << "dE_" << a << "/ds mismatch vs finite difference";
  }
}

} // namespace numsim::cas

#endif // TENSOREVALUATORTEST_H
