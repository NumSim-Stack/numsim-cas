#ifndef TENSORALGEBRAASSUMETEST_H
#define TENSORALGEBRAASSUMETEST_H

#include <gtest/gtest.h>

#include <numsim_cas/numsim_cas.h>
#include <numsim_cas/tensor/tensor_assume.h>

namespace numsim::cas {

// ─── #228: algebraic-property annotations (orthogonal / PD / PSD) ────

TEST(TensorAlgebraAssume, AssumeOrthogonalAndQuery) {
  auto R = std::get<0>(make_tensor_variable(std::tuple{"R", 3, 2}));
  EXPECT_FALSE(is_orthogonal(R)) << "default state: no annotation";
  assume_orthogonal(R);
  EXPECT_TRUE(is_orthogonal(R));
  // Orthogonal does NOT imply any other algebra kind.
  EXPECT_FALSE(is_positive_definite(R));
  EXPECT_FALSE(is_positive_semidefinite(R));
}

TEST(TensorAlgebraAssume, AssumePositiveDefiniteAndQuery) {
  auto C = std::get<0>(make_tensor_variable(std::tuple{"C", 3, 2}));
  EXPECT_FALSE(is_positive_definite(C));
  assume_positive_definite(C);
  EXPECT_TRUE(is_positive_definite(C));
  // PD => PSD by definition.
  EXPECT_TRUE(is_positive_semidefinite(C));
  // PD => symmetric (auto-propagated to the projector-space tag).
  EXPECT_TRUE(is_symmetric(C));
  EXPECT_FALSE(is_orthogonal(C));
}

TEST(TensorAlgebraAssume, AssumePositiveSemidefiniteAndQuery) {
  auto H = std::get<0>(make_tensor_variable(std::tuple{"H", 3, 2}));
  EXPECT_FALSE(is_positive_semidefinite(H));
  assume_positive_semidefinite(H);
  EXPECT_TRUE(is_positive_semidefinite(H));
  // PSD does NOT imply PD (the strict-vs-weak direction).
  EXPECT_FALSE(is_positive_definite(H));
  // PSD => symmetric (auto-propagated).
  EXPECT_TRUE(is_symmetric(H));
  EXPECT_FALSE(is_orthogonal(H));
}

TEST(TensorAlgebraAssume, OrthogonalDoesNotImplySymmetric) {
  // Rotations are generally not symmetric — annotating orthogonal must
  // leave the projector-space tag alone.
  auto R = std::get<0>(make_tensor_variable(std::tuple{"R", 3, 2}));
  assume_orthogonal(R);
  EXPECT_FALSE(is_symmetric(R));
  EXPECT_FALSE(is_skew(R));
  // The space tag was never set.
  EXPECT_FALSE(R.get().space().has_value());
}

TEST(TensorAlgebraAssume, AssumesOverwritePreviousAlgKind) {
  // Last assume wins (matches the projector-space assume_* convention).
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  assume_orthogonal(A);
  EXPECT_TRUE(is_orthogonal(A));
  assume_positive_definite(A);
  EXPECT_FALSE(is_orthogonal(A));
  EXPECT_TRUE(is_positive_definite(A));
}

TEST(TensorAlgebraAssume, AlgKindSharedAcrossHoldersOfSameNode) {
  // expression_holder is shared_ptr-based, so copies of a holder see the
  // same underlying tensor_expression. Sanity-check that an annotation set
  // through one holder is visible through another holder to the same node.
  auto C = std::get<0>(make_tensor_variable(std::tuple{"C", 3, 2}));
  auto C_alias = C; // shared_ptr aliasing
  assume_positive_definite(C);
  EXPECT_TRUE(is_positive_definite(C_alias));
  EXPECT_TRUE(is_symmetric(C_alias));
}

TEST(TensorAlgebraAssume, PdPreservesPriorVolumetricSubspace) {
  // Volumetric PD = positive multiple of identity is a real case (e.g.
  // isotropic pressure). assume_positive_definite must NOT downgrade an
  // already-specified Vol space to {Sym, AnyTrace}.
  auto P = std::get<0>(make_tensor_variable(std::tuple{"P", 3, 2}));
  assume_volumetric(P);
  EXPECT_TRUE(is_volumetric(P));
  assume_positive_definite(P);
  EXPECT_TRUE(is_positive_definite(P));
  EXPECT_TRUE(is_volumetric(P)) << "Vol subspace was destroyed by PD assume";
}

TEST(TensorAlgebraAssume, PsdPreservesPriorDeviatoricSubspace) {
  auto D = std::get<0>(make_tensor_variable(std::tuple{"D", 3, 2}));
  assume_deviatoric(D);
  EXPECT_TRUE(is_deviatoric(D));
  assume_positive_semidefinite(D);
  EXPECT_TRUE(is_positive_semidefinite(D));
  EXPECT_TRUE(is_deviatoric(D));
}

TEST(TensorAlgebraAssume, PdOverridesIncompatibleSpace) {
  // Skew is incompatible with PD (PD requires symmetric). assume_pd should
  // overwrite the skew tag.
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  assume_skew(A);
  EXPECT_TRUE(is_skew(A));
  assume_positive_definite(A);
  EXPECT_FALSE(is_skew(A));
  EXPECT_TRUE(is_symmetric(A));
}

TEST(TensorAlgebraAssume, IsSymmetricFollowsAlgKindThroughClearSpace) {
  // PD => symmetric independently of the projector-space tag. Clearing the
  // space should NOT lose the symmetric implication while alg_kind is PD.
  auto C = std::get<0>(make_tensor_variable(std::tuple{"C", 3, 2}));
  assume_positive_definite(C);
  EXPECT_TRUE(is_symmetric(C));
  C.data()->clear_space();
  EXPECT_FALSE(C.get().space().has_value());
  EXPECT_TRUE(is_symmetric(C))
      << "is_symmetric should follow alg_kind=PD through clear_space";
}

TEST(TensorAlgebraAssume, ClearAlgKindResetsToNone) {
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  assume_orthogonal(A);
  EXPECT_TRUE(is_orthogonal(A));
  A.data()->clear_algebra_kind();
  EXPECT_FALSE(is_orthogonal(A));
  EXPECT_FALSE(is_positive_definite(A));
  EXPECT_FALSE(is_positive_semidefinite(A));
}

TEST(TensorAlgebraAssume, AlgKindOrthogonalToProjectorSpace) {
  // The two annotation systems are independent: assume_skew() leaves the
  // algebra_kind alone, and assume_orthogonal() leaves the space alone.
  auto W = std::get<0>(make_tensor_variable(std::tuple{"W", 3, 2}));
  assume_skew(W);
  EXPECT_TRUE(is_skew(W));
  EXPECT_FALSE(is_orthogonal(W));
  assume_orthogonal(W);
  EXPECT_TRUE(is_orthogonal(W));
  // The earlier Skew space tag survives — orthogonal does not clear it.
  EXPECT_TRUE(is_skew(W));
}

} // namespace numsim::cas

#endif // TENSORALGEBRAASSUMETEST_H
