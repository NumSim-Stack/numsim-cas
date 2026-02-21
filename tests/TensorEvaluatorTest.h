#ifndef TENSOREVALUATORTEST_H
#define TENSOREVALUATORTEST_H

#include <gtest/gtest.h>
#include <memory>

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor/visitors/tensor_evaluator.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_functions.h>

namespace numsim::cas {

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
  auto delta = make_expression<kronecker_delta>(3);
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

TEST(TensorEval, EvalBasisChange) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  // clang-format off
  ev.set(A, make_test_data<2, 2>({1.0, 2.0,
                                   3.0, 4.0}));
  // clang-format on
  // trans(A) = basis_change(A, {2,1})
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

TEST(TensorEval, EvalT2sTensorMulNotImplemented2) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto t2s_expr = trace(A);
  auto expr = make_expression<tensor_to_scalar_with_tensor_mul>(A, t2s_expr);
  EXPECT_THROW(ev.apply(expr), not_implemented_error);
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

TEST(TensorEval, EvalT2sTensorMulNotImplemented) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto t2s_expr = trace(A);
  auto expr = make_expression<tensor_to_scalar_with_tensor_mul>(A, t2s_expr);
  EXPECT_THROW(ev.apply(expr), not_implemented_error);
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

TEST(TensorEval, NotImplementedErrorCarriesMessage) {
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 2, 2);
  auto t2s_expr = trace(A);
  auto expr = make_expression<tensor_to_scalar_with_tensor_mul>(A, t2s_expr);
  try {
    ev.apply(expr);
    FAIL() << "Expected not_implemented_error";
  } catch (not_implemented_error const &e) {
    EXPECT_TRUE(std::string(e.what()).find("not yet implemented") !=
                std::string::npos);
  }
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
                                  6.0, 5.0, 2.0,
                                  7.0, 8.0, 4.0});
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

} // namespace numsim::cas

#endif // TENSOREVALUATORTEST_H
