#ifndef TENSORALGEBRAASSUMETEST_H
#define TENSORALGEBRAASSUMETEST_H

#include <cmath>
#include <gtest/gtest.h>
#include <memory>

#include <numsim_cas/numsim_cas.h>
#include <numsim_cas/tensor/tensor_assume.h>
#include <numsim_cas/tensor/visitors/tensor_evaluator.h>

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

TEST(TensorAlgebraAssume, AssumesAccumulate) {
  // The manager is set-based (mirrors numeric_assumption_manager), so
  // multiple annotations coexist — assuming PD after orthogonal does NOT
  // remove orthogonal. (Orthogonal-and-PD together is rare in practice
  // but the API shouldn't silently throw away information.)
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  assume_orthogonal(A);
  EXPECT_TRUE(is_orthogonal(A));
  assume_positive_definite(A);
  EXPECT_TRUE(is_orthogonal(A)) << "set-based manager should retain orthogonal";
  EXPECT_TRUE(is_positive_definite(A));
  EXPECT_TRUE(is_positive_semidefinite(A));
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

TEST(TensorAlgebraAssume,
     IsSymmetricFollowsAlgebraAssumptionThroughClearSpace) {
  // PD => symmetric independently of the projector-space tag. Clearing the
  // space should NOT lose the symmetric implication while the PD
  // assumption is still in the set.
  auto C = std::get<0>(make_tensor_variable(std::tuple{"C", 3, 2}));
  assume_positive_definite(C);
  EXPECT_TRUE(is_symmetric(C));
  C.data()->clear_space();
  EXPECT_FALSE(C.get().space().has_value());
  EXPECT_TRUE(is_symmetric(C))
      << "is_symmetric should follow PD through clear_space";
}

TEST(TensorAlgebraAssume, ClearingManagerRemovesAllAlgebraAssumptions) {
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  assume_orthogonal(A);
  assume_positive_definite(A);
  EXPECT_TRUE(is_orthogonal(A));
  EXPECT_TRUE(is_positive_definite(A));
  A.data()->tensor_algebra_assumptions().clear();
  EXPECT_FALSE(is_orthogonal(A));
  EXPECT_FALSE(is_positive_definite(A));
  EXPECT_FALSE(is_positive_semidefinite(A));
}

TEST(TensorAlgebraAssume, EraseSingleAssumption) {
  // erase() on the manager removes a specific tag without affecting others.
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  assume_positive_definite(A);
  EXPECT_TRUE(is_positive_definite(A));
  EXPECT_TRUE(is_positive_semidefinite(A));
  // Erase just the stronger property. PSD should remain.
  A.data()->tensor_algebra_assumptions().erase(positive_definite{});
  EXPECT_FALSE(is_positive_definite(A));
  EXPECT_TRUE(is_positive_semidefinite(A));
}

TEST(TensorAlgebraAssume, RemoveAssumptionFreeFunction) {
  // remove_assumption(expr, tag) matches scalar_assume.h's API shape.
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  assume_orthogonal(A);
  EXPECT_TRUE(is_orthogonal(A));
  remove_assumption(A, orthogonal{});
  EXPECT_FALSE(is_orthogonal(A));
}

TEST(TensorAlgebraAssume, RemoveDoesNotUndoCrossMechanismImplications) {
  // assume_positive_definite sets {Symmetric, AnyTrace} on the space.
  // Removing positive_definite{} from the manager leaves the space tag
  // alone — the user's "A is symmetric" assertion stands independently.
  auto C = std::get<0>(make_tensor_variable(std::tuple{"C", 3, 2}));
  assume_positive_definite(C);
  EXPECT_TRUE(C.get().space().has_value());
  EXPECT_TRUE(is_symmetric(C));
  remove_assumption(C, positive_definite{});
  remove_assumption(C, positive_semidefinite{});
  EXPECT_FALSE(is_positive_definite(C));
  EXPECT_FALSE(is_positive_semidefinite(C));
  // Space tag persists; is_symmetric returns true via the space.
  EXPECT_TRUE(C.get().space().has_value());
  EXPECT_TRUE(is_symmetric(C));
}

TEST(TensorAlgebraAssume, AlgebraAssumptionOrthogonalToProjectorSpace) {
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

// ─── #246 algebra-rule folds (α-2a): inv / det for orthogonal ──────────

TEST(TensorAlgebraFold, InvOrthogonalFoldsToTrans) {
  // inv(R) = trans(R) when R is annotated orthogonal — closes the
  // dominant simplification for rotation tensors. The result must also
  // carry the orthogonal annotation so downstream queries / further
  // folds see it.
  auto R = std::get<0>(make_tensor_variable(std::tuple{"R", 3, 2}));
  assume_orthogonal(R);
  auto r = inv(R);
  // Structural form is the transpose, NOT a tensor_inv node.
  EXPECT_FALSE(is_same<tensor_inv>(r))
      << "inv(orthogonal) should fold, not build a tensor_inv node";
  EXPECT_TRUE(is_same<permute_indices_wrapper>(r))
      << "expected trans(R) = permute_indices_wrapper";
  // Annotation propagation: inv(orthogonal) is still orthogonal.
  EXPECT_TRUE(is_orthogonal(r))
      << "result should carry orthogonal annotation for downstream folds";
}

TEST(TensorAlgebraFold, InvOrthogonalDoesNotFireAtRank4) {
  // The fold requires rank == 2 (trans() is rank-2 only and orthogonality
  // at rank-4 isn't standardly defined). A rank-4 tensor with orthogonal
  // annotation should still build a tensor_inv (routed to invf at eval).
  auto A4 = make_expression<tensor>("A4", std::size_t{3}, std::size_t{4});
  assume_orthogonal(A4);
  auto r = inv(A4);
  EXPECT_TRUE(is_same<tensor_inv>(r))
      << "rank-4 orthogonal should NOT trigger the trans fold";
}

TEST(TensorAlgebraFold, InvUnannotatedDoesNotFireFold) {
  // Negative case: without the orthogonal annotation the fold must NOT
  // fire — guards against accidental over-eager folding.
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  auto r = inv(A);
  EXPECT_TRUE(is_same<tensor_inv>(r))
      << "inv(unannotated rank-2) should build tensor_inv as before";
}

TEST(TensorAlgebraFold, DetOrthogonalFoldsToOne) {
  // det(R) = 1 for proper rotations (the dominant continuum-mechanics
  // case). The annotation doesn't distinguish improper rotations (det =
  // -1); a future chirality sub-tag could refine this.
  auto R = std::get<0>(make_tensor_variable(std::tuple{"R", 3, 2}));
  assume_orthogonal(R);
  auto d = det(R);
  EXPECT_TRUE(is_same<tensor_to_scalar_one>(d))
      << "det(orthogonal) should fold to tensor_to_scalar_one";
}

TEST(TensorAlgebraFold, DetUnannotatedDoesNotFireFold) {
  // Negative case: det of an un-annotated tensor must build a tensor_det
  // node — fold gated correctly.
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  auto d = det(A);
  EXPECT_TRUE(is_same<tensor_det>(d))
      << "det(unannotated) should build tensor_det as before";
}

TEST(TensorAlgebraFold, InvOrthogonalEvaluatesCorrectly) {
  // End-to-end: inv(R) folded to trans(R) must evaluate to the actual
  // matrix inverse for a numerically-orthogonal rotation. Build a 3D
  // rotation about z by π/6 and verify inv(R) equals R^T componentwise.
  auto R = std::get<0>(make_tensor_variable(std::tuple{"R", 3, 2}));
  assume_orthogonal(R);
  const double c = std::cos(M_PI / 6.0);
  const double s = std::sin(M_PI / 6.0);
  auto R_data = std::make_shared<tensor_data<double, 3, 2>>();
  auto *raw = R_data->raw_data();
  raw[0] = c;
  raw[1] = -s;
  raw[2] = 0;
  raw[3] = s;
  raw[4] = c;
  raw[5] = 0;
  raw[6] = 0;
  raw[7] = 0;
  raw[8] = 1;
  tensor_evaluator<double> ev;
  ev.set(R, std::static_pointer_cast<tensor_data_base<double>>(R_data));
  auto inv_R_data = ev.apply(inv(R));
  ASSERT_NE(inv_R_data, nullptr);
  // For a rotation, transpose IS the inverse: R^T(0,1) = R(1,0) = s.
  auto const &inv_R =
      static_cast<tensor_data<double, 3, 2> const &>(*inv_R_data).data();
  EXPECT_NEAR(inv_R(0, 1), s, 1e-12);
  EXPECT_NEAR(inv_R(1, 0), -s, 1e-12);
  EXPECT_NEAR(inv_R(2, 2), 1.0, 1e-12);
}

} // namespace numsim::cas

#endif // TENSORALGEBRAASSUMETEST_H
