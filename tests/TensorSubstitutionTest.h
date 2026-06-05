#ifndef TENSORSUBSTITUTIONTEST_H
#define TENSORSUBSTITUTIONTEST_H

#pragma once

#include "cas_test_helpers.h"
#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"

#include <numsim_cas/core/substitute.h>
#include <numsim_cas/tensor/tensor_assume.h>
#include <numsim_cas/tensor/visitors/tensor_substitution.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_substitution.h>

namespace {

using TensorSubTestDims =
    ::testing::Types<std::integral_constant<std::size_t, 1>,
                     std::integral_constant<std::size_t, 2>,
                     std::integral_constant<std::size_t, 3>>;

} // namespace

template <typename DimTag>
class TensorSubstitutionTest : public ::testing::Test {
protected:
  static constexpr std::size_t Dim = DimTag::value;

  using tensor_t =
      numsim::cas::expression_holder<numsim::cas::tensor_expression>;
  using scalar_t =
      numsim::cas::expression_holder<numsim::cas::scalar_expression>;
  using t2s_t =
      numsim::cas::expression_holder<numsim::cas::tensor_to_scalar_expression>;

  TensorSubstitutionTest() {
    std::tie(X, Y, Z) = numsim::cas::make_tensor_variable(
        std::tuple{"X", Dim, 2}, std::tuple{"Y", Dim, 2},
        std::tuple{"Z", Dim, 2});

    std::tie(x, y, z) = numsim::cas::make_scalar_variable("x", "y", "z");
  }

  tensor_t X, Y, Z;
  scalar_t x, y, z;
};

TYPED_TEST_SUITE(TensorSubstitutionTest, TensorSubTestDims);

// tensor -> tensor substitution: subs(X+Y, X, Z) -> Z+Y
TYPED_TEST(TensorSubstitutionTest, TensorForTensor) {
  auto &X = this->X;
  auto &Y = this->Y;
  auto &Z = this->Z;

  auto expr = X + Y;
  auto result = numsim::cas::substitute(expr, X, Z);
  auto expected = Z + Y;
  EXPECT_TRUE(result == expected)
      << "Expected " << testcas::S(expected) << ", got: " << testcas::S(result);
}

// scalar -> scalar in tensor: subs(x*X, x, y) -> y*X
TYPED_TEST(TensorSubstitutionTest, ScalarInTensor) {
  auto &X = this->X;
  auto &x = this->x;
  auto &y = this->y;

  auto expr = x * X;
  auto result = numsim::cas::substitute(expr, x, y);
  auto expected = y * X;
  EXPECT_TRUE(result == expected)
      << "Expected " << testcas::S(expected) << ", got: " << testcas::S(result);
}

// t2s -> t2s in tensor: subs(trace(X)*Y, trace(X), det(X)) -> det(X)*Y
TYPED_TEST(TensorSubstitutionTest, T2sInTensor) {
  auto &X = this->X;
  auto &Y = this->Y;

  auto trX = numsim::cas::trace(X);
  auto detX = numsim::cas::det(X);

  auto expr = numsim::cas::make_expression<
      numsim::cas::tensor_to_scalar_with_tensor_mul>(Y, trX);
  auto result = numsim::cas::substitute(expr, trX, detX);
  auto expected = numsim::cas::make_expression<
      numsim::cas::tensor_to_scalar_with_tensor_mul>(Y, detX);
  EXPECT_TRUE(result == expected)
      << "Expected " << testcas::S(expected) << ", got: " << testcas::S(result);
}

// Cross-domain depth: scalar sub reaches through t2s then tensor
// subs(trace(x*X)*Y, x, z)
TYPED_TEST(TensorSubstitutionTest, CrossDomainDepth) {
  auto &X = this->X;
  auto &Y = this->Y;
  auto &x = this->x;
  auto &z = this->z;

  auto inner = x * X;
  auto trInner = numsim::cas::trace(inner);
  auto expr = numsim::cas::make_expression<
      numsim::cas::tensor_to_scalar_with_tensor_mul>(Y, trInner);

  auto result = numsim::cas::substitute(expr, x, z);

  auto expectedInner = z * X;
  auto expectedTr = numsim::cas::trace(expectedInner);
  auto expected = numsim::cas::make_expression<
      numsim::cas::tensor_to_scalar_with_tensor_mul>(Y, expectedTr);
  EXPECT_TRUE(result == expected)
      << "Expected " << testcas::S(expected) << ", got: " << testcas::S(result);
}

// ─── β-4: outer-annotation preservation across substitution ──────────────

// Outer algebra annotation (PD) on a compound input survives substitution.
// Without the rebuild-visitor propagation, the fresh tensor_add node
// produced by `substitute(X+Y, X, Z)` would have an empty
// algebra-assumption manager — the user-asserted PD would be lost.
TYPED_TEST(TensorSubstitutionTest, OuterPdAnnotationSurvivesSubstitution) {
  using numsim::cas::assume_positive_definite;
  using numsim::cas::is_positive_definite;
  auto &X = this->X;
  auto &Y = this->Y;
  auto &Z = this->Z;

  auto C = X + Y;
  assume_positive_definite(C);
  ASSERT_TRUE(is_positive_definite(C));

  auto result = numsim::cas::substitute(C, X, Z);
  EXPECT_TRUE(is_positive_definite(result))
      << "PD annotation on the outer compound input must survive rebuild";
  // PSD ⇒ PD per the assume_* convention.
  EXPECT_TRUE(numsim::cas::is_positive_semidefinite(result));
}

// Same shape, orthogonal instead of PD. Pure algebra-manager propagation;
// no space-tag side effects.
TYPED_TEST(TensorSubstitutionTest, OuterOrthogonalSurvivesSubstitution) {
  using numsim::cas::assume_orthogonal;
  using numsim::cas::is_orthogonal;
  auto &X = this->X;
  auto &Y = this->Y;
  auto &Z = this->Z;

  auto C = X + Y;
  assume_orthogonal(C);
  ASSERT_TRUE(is_orthogonal(C));

  auto result = numsim::cas::substitute(C, X, Z);
  EXPECT_TRUE(is_orthogonal(result))
      << "Orthogonal annotation on the outer compound must survive rebuild";
}

// Space tag on the outer compound survives — the rebuild's tensor_add ctor
// would propagate space only if children agreed, but we annotated the
// PARENT directly. Without propagate_outer_annotations the new add would
// have empty space.
TYPED_TEST(TensorSubstitutionTest, OuterSpaceTagSurvivesSubstitution) {
  using numsim::cas::assume_symmetric;
  using numsim::cas::is_symmetric;
  auto &X = this->X;
  auto &Y = this->Y;
  auto &Z = this->Z;

  auto C = X + Y;
  assume_symmetric(C);
  ASSERT_TRUE(is_symmetric(C));

  auto result = numsim::cas::substitute(C, X, Z);
  EXPECT_TRUE(is_symmetric(result))
      << "Symmetric space on the outer compound must survive rebuild";
}

// Ctor-inferred space wins over input space on conflict — don't overwrite
// what a wrapper's ctor learned from structural analysis with a stale
// user-asserted annotation.
//
// Setup: inv(X) where X is PD. Annotate the WHOLE expression `inv(X)` as
// `assume_skew` (deliberately contradictory — PD ⇒ symmetric, not skew).
// After substitute(_, X, Z) where Z is also PD, the new tensor_inv ctor
// inherits Sym from Z (via α-2b propagation). propagate_outer_annotations
// must NOT overwrite that Sym with the user's contradictory Skew.
TYPED_TEST(TensorSubstitutionTest, CtorInferredSpaceWinsOverInputSpace) {
  using numsim::cas::assume_positive_definite;
  using numsim::cas::assume_skew;
  using numsim::cas::is_symmetric;
  auto &X = this->X;
  auto &Z = this->Z;

  assume_positive_definite(X);
  assume_positive_definite(Z);
  auto C = numsim::cas::inv(X);
  ASSERT_TRUE(is_symmetric(C)) << "PD ⇒ symmetric ctor propagation";
  // Now corrupt the outer with a contradictory annotation by directly
  // setting Skew on the space (bypassing assume_skew's algebra-manager
  // overwrite). This is the pathological case the helper must handle.
  C.data()->set_space({numsim::cas::Skew{}, numsim::cas::AnyTraceTag{}});

  auto result = numsim::cas::substitute(C, X, Z);
  // The rebuilt inv(Z) gets Sym from its ctor (Z is PD). The input's
  // Skew must NOT win because the output already has a space tag —
  // ctor-inferred wins.
  EXPECT_TRUE(is_symmetric(result))
      << "Ctor-inferred Sym must not be overwritten by input's Skew";
}

// Identity substitution (subs(X, X, X)) returns m_new directly via
// tensor_substitution::apply's fast path — no rebuild, annotations on
// X are trivially preserved through shared_ptr aliasing. Lock-in: this
// path should NOT regress to going through propagate_outer_annotations
// (which would no-op anyway since m_result.data() == expr.data()).
TYPED_TEST(TensorSubstitutionTest,
           IdentitySubstitutionPreservesAllAnnotations) {
  using numsim::cas::assume_positive_definite;
  using numsim::cas::is_positive_definite;
  auto &X = this->X;

  assume_positive_definite(X);
  auto result = numsim::cas::substitute(X, X, X);
  EXPECT_TRUE(is_positive_definite(result));
  EXPECT_EQ(result.data(), X.data())
      << "identity substitution should return the same shared_ptr instance";
}

#endif // TENSORSUBSTITUTIONTEST_H
