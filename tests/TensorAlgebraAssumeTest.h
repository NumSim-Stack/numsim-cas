#ifndef TENSORALGEBRAASSUMETEST_H
#define TENSORALGEBRAASSUMETEST_H

#include <cmath>
#include <gtest/gtest.h>
#include <memory>
#include <numbers>

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

TEST(TensorAlgebraFold, TransOrthogonalIsOrthogonal) {
  // Algebra fact: trans of orthogonal is orthogonal. The propagation
  // lives in trans() itself so callers that go through trans() directly
  // (not via the inv() fold) also see it. This is the primary
  // propagation test; InvOrthogonalFoldsToTrans below verifies the fold
  // composes with it.
  auto R = std::get<0>(make_tensor_variable(std::tuple{"R", 3, 2}));
  assume_orthogonal(R);
  auto rT = trans(R);
  EXPECT_TRUE(is_orthogonal(rT)) << "trans(orthogonal) should be orthogonal";
}

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

TEST(TensorAlgebraFold, InvSkewOrthogonalOddDimThrowsContradiction) {
  // User asserts BOTH skew and orthogonal on an odd-dim tensor —
  // logical contradiction (skew odd-dim ⇒ det = 0 by the (-1)^n det
  // theorem; orthogonal ⇒ det = ±1). The skew-singularity guard
  // (rank-2, odd-dim, contains_skew_factor) is ordered BEFORE the
  // orthogonal fold so the contradiction is rejected loudly rather
  // than silently folded to trans(R).
  //
  // The 2D case is legitimate (2D rotation by π/2 IS both skew and
  // orthogonal: R=[[0,-1],[1,0]], R^T=-R, R^T R = I). That case is
  // covered by InvSkewOrthogonal2DStillFolds below.
  auto R3 = make_expression<tensor>("R3", std::size_t{3}, std::size_t{2});
  assume_skew(R3);
  assume_orthogonal(R3);
  try {
    [[maybe_unused]] auto r = inv(R3);
    FAIL() << "Expected throw for skew+orthogonal at odd dim "
              "(logical contradiction)";
  } catch (invalid_expression_error const &e) {
    EXPECT_NE(std::string(e.what()).find("skew"), std::string::npos)
        << "error message should mention skew singularity";
  }
}

TEST(TensorAlgebraFold, InvSkewOrthogonal2DStillFolds) {
  // Even-dim skew-and-orthogonal IS mathematically valid (e.g. 2D
  // rotation by π/2 satisfies both). Skew guard doesn't fire (even
  // dim) so the orthogonal fold runs: inv(R) → trans(R) = -R.
  auto R2 = make_expression<tensor>("R2", std::size_t{2}, std::size_t{2});
  assume_skew(R2);
  assume_orthogonal(R2);
  auto r = inv(R2);
  // Even-dim skew + orthogonal is legit; fold to trans which for
  // skew returns -R (trans()'s skew short-circuit). So inv = -R.
  EXPECT_TRUE(is_same<tensor_negative>(r))
      << "2D skew-orthogonal inv folds to -R via trans short-circuit";
  // (-R) is itself orthogonal: (-R)^T (-R) = R^T R = I. trans()'s skew
  // branch inserts the orthogonal annotation onto the negated result;
  // lock that in so a refactor of the short-circuit doesn't drop it.
  EXPECT_TRUE(is_orthogonal(r))
      << "negated result should still carry the orthogonal annotation";
}

TEST(TensorAlgebraFold, InvScalarMulOrthogonalRecursesCorrectly) {
  // inv(α·R) → inv(R)/α = trans(R)/α. Locks in the inv() factory
  // ordering: the orthogonal-fold check (gated on rank-2) sits BEFORE
  // the scalar_mul fold but must NOT fire on α·R, because
  // tensor_scalar_mul does NOT propagate the orthogonal algebra
  // annotation (verified in operators/scalar/tensor_scalar_mul.h —
  // only `space` is forwarded). If a future change to scalar_mul
  // wrongly propagated orthogonal, the fold would silently collapse
  // α·R → trans(R) instead of preserving the 1/α factor.
  auto R = std::get<0>(make_tensor_variable(std::tuple{"R", 3, 2}));
  assume_orthogonal(R);
  auto alpha = std::get<0>(make_scalar_variable("alpha"));
  auto r = inv(alpha * R);
  // POSITIVE structural check (not negative). In the bug case where
  // is_orthogonal(α·R) wrongly returned true, the orthogonal fold
  // would build permute_indices_wrapper(α·R, {2,1}) — that's
  // neither identity_tensor nor tensor_inv, so absence-only
  // assertions would silently miss the bug. Assert presence of the
  // 1/α scalar factor: a tensor_scalar_mul wrapping trans(R).
  ASSERT_TRUE(is_same<tensor_scalar_mul>(r))
      << "expected (1/α)·trans(R); bare trans/permute means the orth "
         "fold wrongly fired on α·R without preserving the scalar factor";
  // Belt and braces: the inner tensor of the scalar_mul is trans(R),
  // i.e. a permute_indices_wrapper.
  auto const &sm = r.template get<tensor_scalar_mul>();
  EXPECT_TRUE(is_same<permute_indices_wrapper>(sm.expr_rhs()))
      << "inner of the scalar_mul should be trans(R)";
}

TEST(TensorAlgebraFold, DetScalarMulOrthogonalIsNotOne) {
  // det(α·R) = α^d · det(R) = α^d for orthogonal R. The result MUST
  // NOT collapse to 1 — that would be the bug case where the
  // orthogonal fold fired on α·R despite the scalar factor.
  // Companion to InvScalarMulOrthogonalRecursesCorrectly above.
  auto R = std::get<0>(make_tensor_variable(std::tuple{"R", 3, 2}));
  assume_orthogonal(R);
  auto alpha = std::get<0>(make_scalar_variable("alpha"));
  auto d = det(alpha * R);
  EXPECT_FALSE(is_same<tensor_to_scalar_one>(d))
      << "det(α·R) is α^d, not 1 — orthogonal fold must not fire on α·R";
}

TEST(TensorAlgebraFold, InvOrthogonalEvaluatesCorrectly) {
  // End-to-end: inv(R) folded to trans(R) must evaluate to the actual
  // matrix inverse for a numerically-orthogonal rotation. Build a 3D
  // rotation about z by π/6 and verify inv(R) equals R^T componentwise.
  auto R = std::get<0>(make_tensor_variable(std::tuple{"R", 3, 2}));
  assume_orthogonal(R);
  const double c = std::cos(std::numbers::pi / 6.0);
  const double s = std::sin(std::numbers::pi / 6.0);
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

// ─── #246 α-2c: trans(orthogonal) · orthogonal → I ──────────────────

TEST(TensorAlgebraMulFold, TransOrthogonalTimesOrthogonalIsIdentity) {
  // R^T · R = I for orthogonal R (the defining relation). Fires the
  // construction-time fold so the AST collapses to identity_tensor
  // without needing evaluator support.
  auto R = std::get<0>(make_tensor_variable(std::tuple{"R", 3, 2}));
  assume_orthogonal(R);
  auto result = trans(R) * R;
  EXPECT_TRUE(is_same<identity_tensor>(result))
      << "trans(orthogonal R) * R should fold to identity_tensor";
  EXPECT_EQ(result.get().rank(), 2u);
  EXPECT_EQ(result.get().dim(), 3u);
}

TEST(TensorAlgebraMulFold, OrthogonalTimesTransOrthogonalIsIdentity) {
  // R · R^T = I — the other ordering. Symmetric to the above.
  auto R = std::get<0>(make_tensor_variable(std::tuple{"R", 3, 2}));
  assume_orthogonal(R);
  auto result = R * trans(R);
  EXPECT_TRUE(is_same<identity_tensor>(result))
      << "R * trans(orthogonal R) should fold to identity_tensor";
}

TEST(TensorAlgebraMulFold, UnannotatedTransTimesItselfDoesNotFold) {
  // Negative case: trans(A) * A does NOT fold to identity when A
  // lacks the orthogonal annotation. The generic mul simplifier
  // handles it.
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  auto result = trans(A) * A;
  EXPECT_FALSE(is_same<identity_tensor>(result))
      << "trans(unannotated) * unannotated must NOT fold to identity";
}

TEST(TensorAlgebraMulFold, OrthogonalTimesUnrelatedOrthogonalDoesNotFold) {
  // R · S where both are orthogonal but unrelated does NOT fold to I
  // (it equals some other orthogonal tensor, not the identity).
  // The detection requires one operand to be the transpose of the
  // other — same-symbol check, not just orthogonality on both.
  auto R = std::get<0>(make_tensor_variable(std::tuple{"R", 3, 2}));
  auto S = std::get<0>(make_tensor_variable(std::tuple{"S", 3, 2}));
  assume_orthogonal(R);
  assume_orthogonal(S);
  auto result = R * S;
  EXPECT_FALSE(is_same<identity_tensor>(result))
      << "two unrelated orthogonals must not collapse to identity";
}

TEST(TensorAlgebraMulFold, OrthogonalSelfMultiplyDoesNotFold) {
  // R · R for orthogonal R is NOT identity (unless R = ±I, which the
  // annotation doesn't promise). The detection requires the transpose
  // pattern. Negative case to ensure no over-eager match.
  auto R = std::get<0>(make_tensor_variable(std::tuple{"R", 3, 2}));
  assume_orthogonal(R);
  auto result = R * R;
  EXPECT_FALSE(is_same<identity_tensor>(result))
      << "R * R must not fold (would only equal I if R = ±I)";
}

TEST(TensorAlgebraMulFold, TransOrthogonalTimesTransOrthogonalDoesNotFold) {
  // R^T · R^T = (R · R)^T which is NOT identity unless R = ±I (and
  // the annotation doesn't promise that). The detection requires the
  // inner of one trans to match the OTHER operand — here both
  // operands are trans(R), so neither's inner (= R) matches the other
  // (= trans(R)). Locks in the symmetric negative case.
  auto R = std::get<0>(make_tensor_variable(std::tuple{"R", 3, 2}));
  assume_orthogonal(R);
  auto result = trans(R) * trans(R);
  EXPECT_FALSE(is_same<identity_tensor>(result))
      << "trans(R) * trans(R) must not fold to identity";
}

TEST(TensorAlgebraMulFold, TransUnannotatedTimesOrthogonalDoesNotFold) {
  // trans(B) * R: B is un-annotated. The detection looks at the inner
  // of the transpose, but is_trans_of returns false (B != R). Even if
  // it did match, the orthogonal check on the non-trans side would
  // need to look at R — which IS orthogonal. But the test name fits
  // the gating logic: the relationship must be transpose-of-self.
  auto B = std::get<0>(make_tensor_variable(std::tuple{"B", 3, 2}));
  auto R = std::get<0>(make_tensor_variable(std::tuple{"R", 3, 2}));
  assume_orthogonal(R);
  auto result = trans(B) * R;
  EXPECT_FALSE(is_same<identity_tensor>(result))
      << "trans(B) * R with B ≠ R must not fold";
}

TEST(TensorAlgebraMulFold, IdentityResultMatchesNumerically) {
  // End-to-end: build a 3D rotation about z by π/3, annotate
  // orthogonal, compute trans(R) * R via the fold, verify the
  // numerical result equals the rank-2 identity.
  auto R = std::get<0>(make_tensor_variable(std::tuple{"R", 3, 2}));
  assume_orthogonal(R);
  const double c = std::cos(std::numbers::pi / 3.0);
  const double s = std::sin(std::numbers::pi / 3.0);
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
  // The fold collapses trans(R)*R to identity_tensor — eval bypasses
  // any actual matrix multiplication.
  auto folded = trans(R) * R;
  ASSERT_TRUE(is_same<identity_tensor>(folded));
  auto result_data = ev.apply(folded);
  ASSERT_NE(result_data, nullptr);
  auto const &result =
      static_cast<tensor_data<double, 3, 2> const &>(*result_data).data();
  EXPECT_NEAR(result(0, 0), 1.0, 1e-12);
  EXPECT_NEAR(result(1, 1), 1.0, 1e-12);
  EXPECT_NEAR(result(2, 2), 1.0, 1e-12);
  EXPECT_NEAR(result(0, 1), 0.0, 1e-12);
  EXPECT_NEAR(result(1, 0), 0.0, 1e-12);
}

} // namespace numsim::cas

#endif // TENSORALGEBRAASSUMETEST_H
