#ifndef TENSORTOSCALARDIFFERENTIATIONTEST_H
#define TENSORTOSCALARDIFFERENTIATIONTEST_H

#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "numsim_cas/operators.h"
#include "numsim_cas/tensor/tensor_functions_fwd.h"
#include "numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_differentiation.h"

namespace numsim::cas {

template <class T> expression_holder<scalar_expression<T>> C(T v) {
  return make_expression<scalar_constant<T>>(v);
}

// ---- Adjust this one function to your tensor-variable type ----
template <class T>
expression_holder<tensor_expression<T>>
MakeTensorVar(char const *name, std::size_t dim, std::size_t rank) {
  // Common alternatives you might have:
  //   return make_expression<tensor<T>>(name, dim, rank);
  //   return make_expression<tensor_variable<T>>(name, dim, rank);
  //   return make_expression<tensor_symbol<T>>(name, dim, rank);
  return make_expression<tensor<T>>(name, dim, rank); // <-- adjust if needed
}

template <class T>
expression_holder<tensor_expression<T>> Z(std::size_t dim, std::size_t rank) {
  return make_expression<tensor_zero<T>>(dim, rank);
}

template <class T> expression_holder<tensor_expression<T>> I(std::size_t dim) {
  return make_expression<kronecker_delta<T>>(dim);
}

class TensorToScalarDifferentiationTest : public ::testing::Test {
protected:
  using T = double;
  static constexpr std::size_t dim = 3;
  static constexpr std::size_t rank = 2;

  using t_expr = expression_holder<tensor_expression<T>>;
  using t2s_expr = expression_holder<tensor_to_scalar_expression<T>>;
  using s_expr = expression_holder<scalar_expression<T>>;

  t_expr Y = MakeTensorVar<T>("Y", dim, rank);
  t_expr X = MakeTensorVar<T>("X", dim, rank); // independent tensor symbol
  t2s_expr trY = numsim::cas::trace(Y);
  t2s_expr trX = numsim::cas::trace(X);
  t2s_expr nY = numsim::cas::norm(Y);
  t2s_expr detY = numsim::cas::det(Y);

  s_expr _2 = C<T>(2.0);
  s_expr _3 = C<T>(3.0);
};

//
// -----------------------------------------------------------------------------
// 1) Each node "for itself"
// -----------------------------------------------------------------------------
TEST_F(TensorToScalarDifferentiationTest, Node_Trace) {
  tensor_to_scalar_differentiation<T> dY(Y);
  auto d = dY.apply(trY);
  EXPECT_SAME_PRINT(d, I<T>(dim));
}

TEST_F(TensorToScalarDifferentiationTest, Node_Trace_NonDependencyGivesZero) {
  tensor_to_scalar_differentiation<T> dY(Y);
  auto d = dY.apply(trX);
  EXPECT_SAME_PRINT(d, Z<T>(dim, rank));
}

TEST_F(TensorToScalarDifferentiationTest, Node_Dot) {
  tensor_to_scalar_differentiation<T> dY(Y);
  auto f = dot(Y);
  auto d = dY.apply(f);
  EXPECT_SAME_PRINT(d, _2 * Y);
}

TEST_F(TensorToScalarDifferentiationTest, Node_Norm) {
  tensor_to_scalar_differentiation<T> dY(Y);
  auto d = dY.apply(nY);
  EXPECT_SAME_PRINT(d, Y / nY);
}

TEST_F(TensorToScalarDifferentiationTest, Node_Det) {
  tensor_to_scalar_differentiation<T> dY(Y);
  auto d = dY.apply(detY);
  EXPECT_SAME_PRINT(d, detY * inv(trans(Y)));
}

TEST_F(TensorToScalarDifferentiationTest, Node_Negative) {
  tensor_to_scalar_differentiation<T> dY(Y);
  auto f = -trY;
  auto d = dY.apply(f);
  EXPECT_SAME_PRINT(d, -I<T>(dim));
}

TEST_F(TensorToScalarDifferentiationTest, Node_Add) {
  tensor_to_scalar_differentiation<T> dY(Y);
  auto f = trY + nY;
  auto d = dY.apply(f);
  EXPECT_SAME_PRINT(d, I<T>(dim) + (Y / nY));
}

TEST_F(TensorToScalarDifferentiationTest, Node_Mul) {
  tensor_to_scalar_differentiation<T> dY(Y);
  auto f = trY * nY;
  auto d = dY.apply(f);

  // d(tr*norm) = (dtr)*norm + tr*(dnorm)
  auto expected = (I<T>(dim) * nY) + (trY * (Y / nY));
  EXPECT_SAME_PRINT(d, expected);
}

TEST_F(TensorToScalarDifferentiationTest, Node_Div) {
  tensor_to_scalar_differentiation<T> dY(Y);
  auto f = trY / nY;
  auto d = dY.apply(f);

  // d(g/h) = (g'*h - g*h') / h^2
  auto expected = ((I<T>(dim) * nY) - (trY * (Y / nY))) / (nY * nY);
  EXPECT_SAME_PRINT(d, expected);
}

TEST_F(TensorToScalarDifferentiationTest, Node_WithScalarMul) {
  tensor_to_scalar_differentiation<T> dY(Y);
  auto f = _3 * trY; // tensor_to_scalar_with_scalar_mul
  auto d = dY.apply(f);
  EXPECT_SAME_PRINT(d, _3 * I<T>(dim));
}

TEST_F(TensorToScalarDifferentiationTest, Node_WithScalarAdd) {
  tensor_to_scalar_differentiation<T> dY(Y);
  auto f = _3 + trY; // tensor_to_scalar_with_scalar_add
  auto d = dY.apply(f);
  EXPECT_SAME_PRINT(d, I<T>(dim));
}

TEST_F(TensorToScalarDifferentiationTest, Node_WithScalarDiv) {
  tensor_to_scalar_differentiation<T> dY(Y);
  auto f = trY / _3; // tensor_to_scalar_with_scalar_div
  auto d = dY.apply(f);
  EXPECT_SAME_PRINT(d, I<T>(dim) / _3);
}

TEST_F(TensorToScalarDifferentiationTest, Node_ScalarDivTensorToScalar) {
  tensor_to_scalar_differentiation<T> dY(Y);
  auto f = _3 / trY; // scalar_with_tensor_to_scalar_div
  auto d = dY.apply(f);
  auto expr = trace(Y) - 1;
  std::cout << expr << std::endl;
  // EXPECT_SAME_PRINT(d, -_3 * I<T>(dim) / (trY * trY));
  // EXPECT_SAME_PRINT(d,
  // pow(trace(Y),trace(Y)-1)*(trace(Y)*I<T>(dim)+trace(Y)*log(trace(Y))*I<T>(dim)));
}

TEST_F(TensorToScalarDifferentiationTest, Node_InnerProductToScalar) {
  tensor_to_scalar_differentiation<T> dY(Y);

  // f = <Y, Y> = Y: Y  (scalar)
  auto f = dot_product(Y, sequence{1, 2}, Y, sequence{1, 2});
  auto d = dY.apply(f);
  // Y_ij*Y_ij
  EXPECT_SAME_PRINT(d, _2 * Y);
}

TEST_F(TensorToScalarDifferentiationTest, Node_Log) {
  tensor_to_scalar_differentiation<T> dY(Y);
  auto f = log(trY);
  auto d = dY.apply(f);
  EXPECT_SAME_PRINT(d, I<T>(dim) / trY);
}

TEST_F(TensorToScalarDifferentiationTest, Node_OneIsZeroDerivative) {
  tensor_to_scalar_differentiation<T> dY(Y);
  auto one = make_expression<tensor_to_scalar_one<T>>();
  auto d = dY.apply(one);
  EXPECT_SAME_PRINT(d, Z<T>(dim, rank));
}

TEST_F(TensorToScalarDifferentiationTest, Node_ZeroIsZeroDerivative) {
  tensor_to_scalar_differentiation<T> dY(Y);
  auto zero = make_expression<tensor_to_scalar_zero<T>>();
  auto d = dY.apply(zero);
  EXPECT_SAME_PRINT(d, Z<T>(dim, rank));
}

TEST_F(TensorToScalarDifferentiationTest, Node_Pow_GeneralExponentDependsOnX) {
  tensor_to_scalar_differentiation<T> dY(Y);

  // f = tr(Y) ^ tr(Y)
  auto f = pow(trY, trY); // tensor_to_scalar_pow
  auto d = dY.apply(f);

  // df = f * (h' * log(g) + h * g'/g) with g=h=tr(Y), g'=h'=I
  // => df = f * (I*log(trY) + I)
  // auto expected = pow(trY, trY) * (I<T>(dim) * log(trY) + I<T>(dim));
  auto expected =
      pow(trY, -1 + trY) * (trY * I<T>(dim) + trY * log(trY) * I<T>(dim));
  EXPECT_SAME_PRINT(d, expected);
}

TEST_F(TensorToScalarDifferentiationTest, Node_Pow_WithScalarExponent) {
  tensor_to_scalar_differentiation<T> dY(Y);

  // f = tr(Y) ^ 3
  auto f = pow(trY, _3); // tensor_to_scalar_pow_with_scalar_exponent
  auto d = dY.apply(f);

  // df = 3 * tr(Y)^(2) * I
  auto expected = _3 * pow(trY, _2) * I<T>(dim);
  EXPECT_SAME_PRINT(d, expected);
}

//
// -----------------------------------------------------------------------------
// 2) Chain rule checks (nested expressions)
// -----------------------------------------------------------------------------
TEST_F(TensorToScalarDifferentiationTest, Chain_LogOfNorm) {
  tensor_to_scalar_differentiation<T> dY(Y);

  // f = log(norm(Y))
  auto f = log(nY);
  auto d = dY.apply(f);

  // df = (1/norm(Y)) * d(norm(Y)) = (1/nY) * (Y/nY) = (Y/nY)/nY
  auto expected = (Y / nY) / nY;
  EXPECT_SAME_PRINT(d, expected);
}

TEST_F(TensorToScalarDifferentiationTest, Chain_LogOfDet) {
  tensor_to_scalar_differentiation<T> dY(Y);

  // f = log(det(Y))
  auto f = log(detY);
  auto d = dY.apply(f);

  // df = d(det)/det = (det*inv(trans(Y)))/det  (may or may not simplify
  // further)
  auto expected = (detY * inv(trans(Y))) / detY;
  EXPECT_SAME_PRINT(d, expected);
}

TEST_F(TensorToScalarDifferentiationTest, Chain_LogOfShiftedTrace) {
  tensor_to_scalar_differentiation<T> dY(Y);

  // f = log(tr(Y) + 2)
  auto f = log(trY + _2);
  auto d = dY.apply(f);

  // df = I / (tr(Y)+2)
  auto expected = I<T>(dim) / (trY + _2);
  EXPECT_SAME_PRINT(d, expected);
}

TEST_F(TensorToScalarDifferentiationTest, Chain_ProductWithLog) {
  tensor_to_scalar_differentiation<T> dY(Y);

  // f = det(Y) * log(tr(Y))
  auto f = detY * log(trY);
  auto d = dY.apply(f);

  // df = det' * log(tr) + det * (I/tr)
  auto expected = (detY * inv(trans(Y))) * log(trY) + detY * (I<T>(dim) / trY);
  EXPECT_SAME_PRINT(d, expected);
}

//
// -----------------------------------------------------------------------------
// 3) Combination tests (mix of rules)
// -----------------------------------------------------------------------------
TEST_F(TensorToScalarDifferentiationTest, Combo_MixedProductQuotient) {
  tensor_to_scalar_differentiation<T> dY(Y);

  // f = (tr(Y) + 2) * (norm(Y)/3) / det(Y)
  auto a = trY + _2;
  auto b = nY / _3;
  auto g = a * b;
  auto h = detY;
  auto f = g / h;

  auto d = dY.apply(f);

  // a' = I
  // b' = (dnorm)/3 = (Y/norm)/3
  // g' = a'*b + a*b'
  auto gp = (I<T>(dim) * b) + (a * ((Y / nY) / _3));

  // h' = det(Y)*inv(trans(Y))
  auto hp = detY * inv(trans(Y));

  // f' = (g'*h - g*h') / h^2
  auto expected = (gp * h - g * hp) / (h * h);

  EXPECT_SAME_PRINT(d, expected);
}

TEST_F(TensorToScalarDifferentiationTest, Combo_NonDependencyInsideExpression) {
  tensor_to_scalar_differentiation<T> dY(Y);

  // f = tr(X) * norm(Y)  (tr(X) independent of Y)
  auto f = trX * nY;
  auto d = dY.apply(f);

  // df = tr(X) * d(norm(Y))
  auto expected = trX * (Y / nY);
  EXPECT_SAME_PRINT(d, expected);
}

// Regression-style combo: ensures apply() does not leak results between calls.
TEST_F(TensorToScalarDifferentiationTest,
       Combo_ApplyMustResetStateBetweenCalls) {
  tensor_to_scalar_differentiation<T> dY(Y);

  auto d1 = dY.apply(trY);
  EXPECT_SAME_PRINT(d1, I<T>(dim));

  // Next call is independent of Y => must be zero
  auto d2 = dY.apply(trX);
  EXPECT_SAME_PRINT(d2, Z<T>(dim, rank));
}

} // namespace numsim::cas

#endif // TENSORTOSCALARDIFFERENTIATIONTEST_H
