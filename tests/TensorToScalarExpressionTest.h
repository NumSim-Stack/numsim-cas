#ifndef TENSORTOSCALAREXPRESSIONTEST_H
#define TENSORTOSCALAREXPRESSIONTEST_H

#include "cas_test_helpers.h"
#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"
#include <cmath>

using std::pow;

// A tiny carrier for scalar type + dimension
namespace testcas::types {
template <typename T, std::size_t D> struct TD {
  using scalar = T;
  static constexpr std::size_t dim = D;
};
} // namespace testcas::types

// Make the fixture accept exactly one TypeParam
template <class P>
using TF = testcas::TensorFixture<typename P::scalar, P::dim>;

// Build the type list with the carrier
using TensorTypes =
    ::testing::Types<testcas::types::TD<float, 1>, testcas::types::TD<float, 2>,
                     testcas::types::TD<float, 3>, testcas::types::TD<int, 1>,
                     testcas::types::TD<int, 2>, testcas::types::TD<int, 3>,
                     testcas::types::TD<double, 1>,
                     testcas::types::TD<double, 2>,
                     testcas::types::TD<double, 3>>;

// Register the suite
TYPED_TEST_SUITE(TF, TensorTypes);

// ---------- Basics: print and simple algebra ----------
TYPED_TEST(TF, TensorToScalar_Basics_PrintAndAlgebra) {
  auto &X = this->X;

  // Shorthands
  auto X_tr = numsim::cas::trace(X);
  auto X_norm = numsim::cas::norm(X);
  auto X_det = numsim::cas::det(X);

  // Basic print (adjust "tr"→"trace" locally if your printer uses that token)
  EXPECT_PRINT(X_tr, "tr(X)");
  EXPECT_PRINT(X_norm, "norm(X)");
  EXPECT_PRINT(X_det, "det(X)");

  // Self algebra
  EXPECT_PRINT(X_tr + X_tr, "2*tr(X)");
  EXPECT_PRINT(X_tr * X_tr, "pow(tr(X),2)");
  EXPECT_PRINT(X_tr / X_tr, "tr(X)/tr(X)");

  EXPECT_PRINT(X_norm * X_norm, "pow(norm(X),2)");
  EXPECT_PRINT(X_det * X_det, "pow(det(X),2)");
}

// ---------- Mixing with scalar symbols ----------
TYPED_TEST(TF, TensorToScalar_WithScalars_OrderingAndPowers) {
  auto &X = this->X;
  auto &x = this->x;

  auto X_tr = numsim::cas::trace(X);

  // Commutativity/canonicalization without depending on a specific order
  EXPECT_SAME_PRINT(x + X_tr, X_tr + x);
  EXPECT_SAME_PRINT((x + X_tr) + x, 2 * x + X_tr);
  EXPECT_SAME_PRINT(x + (X_tr + x), 2 * x + X_tr);
  EXPECT_SAME_PRINT((x + X_tr) + X_tr, x + 2 * X_tr);
  EXPECT_SAME_PRINT(X_tr + (X_tr + x), x + 2 * X_tr);
  EXPECT_SAME_PRINT((X_tr + x) + (X_tr + x), 2 * x + 2 * X_tr);

  // Products + power bumps
  EXPECT_SAME_PRINT(x * X_tr, X_tr * x);
  EXPECT_PRINT((x * X_tr) * x, "pow(x,2)*tr(X)");
  EXPECT_PRINT(x * (X_tr * x), "pow(x,2)*tr(X)");

  // Product squared; compare to constructed pow for robustness
  EXPECT_SAME_PRINT((X_tr * x) * (X_tr * x),
                    pow(x, this->_2) * pow(X_tr, this->_2));
}

// ---------- Division chains with tensor-to-scalar nodes ----------
TYPED_TEST(TF, TensorToScalar_Division_ChainsAndSimplify) {
  auto &Y = this->Y; // second tensor for variety
  auto &_2 = this->_2;
  auto &_3 = this->_3;

  using numsim::cas::norm;
  using numsim::cas::trace;

  auto trY = trace(Y);
  auto nY = norm(Y);

  // Helpers closely matching the sample from your snippet
  auto scalar_tr = (_3 / trY);  // 3/tr(Y)
  auto scalar_norm = (_2 / nY); // 2/norm(Y)
  auto tr_scalar = trY / _3;    // tr(Y)/3
  auto norm_scalar = nY / _2;   // norm(Y)/2

  EXPECT_PRINT(numsim::cas::diff(trace(Y), Y), "I");
  EXPECT_PRINT(numsim::cas::diff(trace(Y * Y), Y), "2*I");
  EXPECT_PRINT(numsim::cas::diff(numsim::cas::dot(Y), Y), "2*Y");

  // Sanity: string form of 3/tr(Y)
  EXPECT_PRINT((_3 / trY), "3/tr(Y)");

  // (3/tr)/(2/norm)  ==  (3*norm)/(2*tr)
  EXPECT_SAME_PRINT((_3 / trY) / (_2 / nY), (_3 * nY) / (_2 * trY));

  // (tr/3)/(norm/2) == (2*tr)/(3*norm)
  EXPECT_SAME_PRINT(tr_scalar / norm_scalar, (_2 * trY) / (_3 * nY));

  // (3/tr)/(norm/2) == (3*2)/(tr*norm)
  EXPECT_SAME_PRINT((_3 / trY) / norm_scalar, (_3 * _2) / (trY * nY));

  // (norm/2)/(3/tr) == (norm*tr)/6
  EXPECT_SAME_PRINT(norm_scalar / (_3 / trY), (nY * trY) / (_3 * _2));

  // norm/2/3 == norm/6
  EXPECT_SAME_PRINT(norm_scalar / _3, nY / (_2 * _3));

  // (norm/2)/tr == norm/(2*tr)
  EXPECT_SAME_PRINT(norm_scalar / trY, nY / (_2 * trY));

  // (2/norm)/3 == 2/(3*norm)
  // EXPECT_SAME_PRINT(scalar_norm / _3, _2 / (_3 * nY));

  // (2/norm)/tr == 2/(norm*tr)
  EXPECT_SAME_PRINT(scalar_norm / trY, _2 / (nY * trY));
}

// ---------- Mixed products: scalar * (tensor_to_scalar)^k ----------
TYPED_TEST(TF, TensorToScalar_MixedProductsAndPowers) {
  auto &X = this->X;
  auto &x = this->x;
  // auto &_2 = this->_2;

  using numsim::cas::trace;

  auto t = trace(X);

  EXPECT_PRINT(x * t * t, "x*pow(tr(X),2)");
  // EXPECT_SAME_PRINT(pow(t, _2) * x, x * pow(t, _2));
  // EXPECT_SAME_PRINT(pow(x * t, _2),
  //                   pow(x, _2) *
  //                       pow(t, _2));
}

#endif // TENSORTOSCALAREXPRESSIONTEST_H
