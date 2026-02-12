#ifndef TENSORTOSCALAREXPRESSIONTEST_H
#define TENSORTOSCALAREXPRESSIONTEST_H

#include "cas_test_helpers.h"
#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"

namespace {

using TestDims = ::testing::Types<std::integral_constant<std::size_t, 1>,
                                  std::integral_constant<std::size_t, 2>,
                                  std::integral_constant<std::size_t, 3>>;

} // namespace

template <typename DimTag>
class TensorToScalarExpressionTest : public ::testing::Test {
protected:
  static constexpr std::size_t Dim = DimTag::value;

  using tensor_t =
      numsim::cas::expression_holder<numsim::cas::tensor_expression>;
  using scalar_t =
      numsim::cas::expression_holder<numsim::cas::scalar_expression>;

  TensorToScalarExpressionTest() {
    std::tie(X, Y, Z) = numsim::cas::make_tensor_variable(
        std::tuple{"X", Dim, 2}, std::tuple{"Y", Dim, 2},
        std::tuple{"Z", Dim, 2});

    std::tie(A, B, C) = numsim::cas::make_tensor_variable(
        std::tuple{"A", Dim, 4}, std::tuple{"B", Dim, 4},
        std::tuple{"C", Dim, 4});

    std::tie(x, y, z) = numsim::cas::make_scalar_variable("x", "y", "z");
    std::tie(a, b, c) = numsim::cas::make_scalar_variable("a", "b", "c");

    // IMPORTANT: do NOT "using std::pow;" in these tests.
    // Keep constants built through CAS so operator overloads stay in CAS-land.
    std::tie(_1, _2, _3) = numsim::cas::make_scalar_constant(1, 2, 3);

    _zero = scalar_t{numsim::cas::get_scalar_zero()};
    _one = scalar_t{numsim::cas::get_scalar_one()};

    _Zero = tensor_t{
        numsim::cas::make_expression<numsim::cas::tensor_zero>(Dim, 2)};
    _One = tensor_t{
        numsim::cas::make_expression<numsim::cas::kronecker_delta>(Dim)};
  }

  // tensors
  tensor_t X, Y, Z;
  tensor_t A, B, C;

  // scalars
  scalar_t x, y, z;
  scalar_t a, b, c;

  // scalar constants / special
  scalar_t _1, _2, _3;
  scalar_t _zero, _one;

  // tensor special
  tensor_t _Zero, _One;
};

TYPED_TEST_SUITE(TensorToScalarExpressionTest, TestDims);

// ---------- Basics: print and simple algebra ----------
TYPED_TEST(TensorToScalarExpressionTest,
           TensorToScalar_Basics_PrintAndAlgebra) {
  auto &X = this->X;

  auto X_tr = numsim::cas::trace(X);
  auto X_norm = numsim::cas::norm(X);
  auto X_det = numsim::cas::det(X);

  EXPECT_PRINT(X_tr, "tr(X)");
  EXPECT_PRINT(X_norm, "norm(X)");
  EXPECT_PRINT(X_det, "det(X)");
  std::cout << X_tr + X_tr << std::endl;
  std::cout << X_norm + X_norm << std::endl;
  std::cout << X_det + X_det << std::endl;
  std::cout << X_tr + X_norm + X_det << std::endl;

  EXPECT_PRINT(X_tr + X_tr, "2*tr(X)");
  EXPECT_PRINT(X_tr * X_tr, "pow(tr(X),2)");
  EXPECT_PRINT(X_tr / X_tr, "tr(X)/tr(X)");

  EXPECT_PRINT(X_norm * X_norm, "pow(norm(X),2)");
  EXPECT_PRINT(X_det * X_det, "pow(det(X),2)");
}

// ---------- Mixing with scalar symbols ----------
TYPED_TEST(TensorToScalarExpressionTest,
           TensorToScalar_WithScalars_OrderingAndPowers) {
  auto &X = this->X;
  auto &x = this->x;

  auto X_tr = numsim::cas::trace(X);

  std::cout << (x + X_tr) + x << std::endl;

  EXPECT_SAME_PRINT(x + X_tr, X_tr + x);
  EXPECT_SAME_PRINT((x + X_tr) + x, 2 * x + X_tr);
  EXPECT_SAME_PRINT(x + (X_tr + x), 2 * x + X_tr);
  EXPECT_SAME_PRINT((x + X_tr) + X_tr, x + 2 * X_tr);
  EXPECT_SAME_PRINT(X_tr + (X_tr + x), x + 2 * X_tr);
  EXPECT_SAME_PRINT((X_tr + x) + (X_tr + x), 2 * x + 2 * X_tr);

  EXPECT_SAME_PRINT(x * X_tr, X_tr * x);
  EXPECT_PRINT((x * X_tr) * x, "pow(x,2)*tr(X)");
  EXPECT_PRINT(x * (X_tr * x), "pow(x,2)*tr(X)");

  // EXPECT_SAME_PRINT((X_tr * x) * (X_tr * x),
  //                   numsim::cas::pow(x, this->_2) * numsim::cas::pow(X_tr,
  //                   this->_2));
}

// ---------- Division chains with tensor-to-scalar nodes ----------
TYPED_TEST(TensorToScalarExpressionTest,
           TensorToScalar_Division_ChainsAndSimplify) {
  auto &Y = this->Y;
  auto &_2 = this->_2;
  auto &_3 = this->_3;

  using numsim::cas::norm;
  using numsim::cas::trace;

  auto trY = trace(Y);
  auto nY = norm(Y);

  auto scalar_norm = (_2 / nY);
  auto norm_scalar = nY / _2;

  // EXPECT_PRINT(numsim::cas::diff(trace(Y), Y), "I");
  // EXPECT_PRINT(numsim::cas::diff(trace(Y * Y), Y), "2*I");
  // EXPECT_PRINT(numsim::cas::diff(numsim::cas::dot(Y), Y), "2*Y");

  EXPECT_PRINT((_3 / trY), "3/tr(Y)");
  EXPECT_SAME_PRINT((_3 / trY) / (_2 / nY), (_3 * nY) / (_2 * trY));
  EXPECT_SAME_PRINT(norm_scalar / (_3 / trY), (nY * trY) / (_3 * _2));
  EXPECT_SAME_PRINT(norm_scalar / _3, nY / (_2 * _3));
  EXPECT_SAME_PRINT(scalar_norm / trY, _2 / (nY * trY));
}

// ---------- Mixed products ----------
TYPED_TEST(TensorToScalarExpressionTest,
           TensorToScalar_MixedProductsAndPowers) {
  auto &X = this->X;
  auto &x = this->x;

  auto t = numsim::cas::trace(X);
  EXPECT_PRINT(x * t * t, "x*pow(tr(X),2)");
}

#endif // TENSORTOSCALAREXPRESSIONTEST_H
