#ifndef TENSORALGEBRAASSUMETEST_H
#define TENSORALGEBRAASSUMETEST_H

#include <cmath>
#include <gtest/gtest.h>
#include <memory>
#include <numbers>

#include <numsim_cas/numsim_cas.h>
#include <numsim_cas/tensor/structural_propagation.h>
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
  // -1); TODO(#269) a chirality sub-tag could refine this.
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

// ─── T2S constant default annotations (#261, H1 from α-2 review) ────

TEST(TensorAlgebraConstants, TensorToScalarOneCarriesPositiveTags) {
  // The constant 1 IS mathematically positive, nonnegative, nonzero,
  // real, integer, rational. Annotated in the ctor so downstream
  // queries see it without a separate fold needing to insert them.
  // This closes the H1 inconsistency: det(orthogonal R) returns
  // tensor_to_scalar_one which now carries positive directly, matching
  // det(PD C)'s tensor_det with positive — semantically equivalent
  // results.
  auto one = make_expression<tensor_to_scalar_one>();
  auto const &a = one.data()->assumptions();
  EXPECT_TRUE(a.contains(positive{}));
  EXPECT_TRUE(a.contains(nonnegative{}));
  EXPECT_TRUE(a.contains(nonzero{}));
  EXPECT_TRUE(a.contains(real_tag{}));
  EXPECT_TRUE(a.contains(integer{}));
  EXPECT_TRUE(a.contains(rational{}));
}

TEST(TensorAlgebraConstants, TensorToScalarZeroCarriesNonnegativeNonpositive) {
  // The constant 0 is nonnegative AND nonpositive (both inequalities
  // are non-strict). NOT positive, NOT negative, NOT nonzero. Real,
  // integer, rational by trivial inclusion.
  auto zero = make_expression<tensor_to_scalar_zero>();
  auto const &a = zero.data()->assumptions();
  EXPECT_TRUE(a.contains(nonnegative{}));
  EXPECT_TRUE(a.contains(nonpositive{}));
  EXPECT_TRUE(a.contains(real_tag{}));
  EXPECT_TRUE(a.contains(integer{}));
  EXPECT_TRUE(a.contains(rational{}));
  // Negative cases:
  EXPECT_FALSE(a.contains(positive{}));
  EXPECT_FALSE(a.contains(negative{}));
  EXPECT_FALSE(a.contains(nonzero{}));
}

// ─── #246 α-2d: det(PD) > 0 scalar-assumption propagation ──────────

TEST(TensorAlgebraDetPropagation, DetPdIsPositive) {
  // det of a PD tensor is strictly positive — product of positive
  // eigenvalues. The propagation lives in det()'s generic return
  // path; folds for identity/zero/orthogonal/etc. don't go through
  // this annotation (those produce constants that are trivially
  // positive or zero by construction).
  auto C = std::get<0>(make_tensor_variable(std::tuple{"C", 3, 2}));
  assume_positive_definite(C);
  auto d = det(C);
  // Direct manager access — scalar_assume.h's is_positive is typed
  // for scalar_expression, not t2s; both inherit assumptions() from
  // the expression base class.
  auto const &a = d.data()->assumptions();
  EXPECT_TRUE(a.contains(positive{}));
  // Joint insertions per scalar_assume convention:
  EXPECT_TRUE(a.contains(nonnegative{}));
  EXPECT_TRUE(a.contains(nonzero{}));
  EXPECT_TRUE(a.contains(real_tag{}));
}

TEST(TensorAlgebraDetPropagation, DetPsdIsNonnegativeNotPositive) {
  // det of PSD is ≥ 0 — could be zero (singular case). Propagate
  // nonnegative but NOT positive / nonzero.
  auto H = std::get<0>(make_tensor_variable(std::tuple{"H", 3, 2}));
  assume_positive_semidefinite(H);
  auto d = det(H);
  auto const &a = d.data()->assumptions();
  EXPECT_TRUE(a.contains(nonnegative{}));
  EXPECT_TRUE(a.contains(real_tag{}));
  EXPECT_FALSE(a.contains(positive{}))
      << "PSD det may be zero; must not assert strictly positive";
  EXPECT_FALSE(a.contains(nonzero{}))
      << "PSD det may be zero; must not assert nonzero";
}

TEST(TensorAlgebraDetPropagation, DetUnannotatedHasNoPositivity) {
  // Negative case: det of an un-annotated tensor must NOT gain
  // positivity tags. Guards against over-eager propagation.
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  auto d = det(A);
  auto const &a = d.data()->assumptions();
  EXPECT_FALSE(a.contains(positive{}));
  EXPECT_FALSE(a.contains(nonnegative{}));
  EXPECT_FALSE(a.contains(nonzero{}));
}

TEST(TensorAlgebraDetPropagation, DetSkewDoesNotFirePropagation) {
  // Skew tensor is not PD/PSD — no propagation. Also det is in odd
  // dimensions actually zero (caught by the existing inv-singularity
  // guard, but that's separate). Just sanity-check the assumption
  // doesn't leak.
  auto W = std::get<0>(make_tensor_variable(std::tuple{"W", 3, 2}));
  assume_skew(W);
  auto d = det(W);
  auto const &a = d.data()->assumptions();
  EXPECT_FALSE(a.contains(positive{}));
  EXPECT_FALSE(a.contains(nonnegative{}));
}

TEST(TensorAlgebraDetPropagation, DetPdConstantFoldsBypassThePropagationPath) {
  // Sanity: det(identity) folds to tensor_to_scalar_one (a constant),
  // not a tensor_det node — so the annotation insertion path doesn't
  // run. The result is already known to be 1, so positivity is
  // implicit. Just verify the fold still fires unchanged.
  auto I = make_expression<identity_tensor>(std::size_t{3}, std::size_t{2});
  auto d = det(I);
  EXPECT_TRUE(is_same<tensor_to_scalar_one>(d));
}

// ─── Recursive-path gap documentation (#246 α-2d) ─────────────────
// The propagation block in det() fires only on the terminal
// tensor_det node. The recursive paths compose results via t2s
// mul/div/pow which do NOT yet propagate `positive` through the
// operation (#260, t2s op propagation through mul/div/pow).
//
// Status of the recursive paths:
//   - det(α·A) — gap open. Outer composition is pow(α,d) * det(A);
//     even though det(A) carries positive via the recursive call,
//     the t2s mul doesn't propagate. Test below pins the current
//     incomplete behavior.
//   - det(inv(C)) — gap CLOSED by PR #264, which projects PD directly
//     onto the composed 1/det(C) result at the is_same<tensor_inv>
//     fold site. The positive lock-in for that case lives in PR #264's
//     composition test suite (TensorAlgebraDetPropagation.
//     DetInvPdIsPositiveViaWrapperProjection) so we don't duplicate
//     here. Once #260 lands generically, the projection at the fold
//     site becomes redundant but remains correct.
//   - det(tensor_mul) — also gap-open via #260; not specifically
//     tested here (covered by composition-test PR's broader matrix).

TEST(TensorAlgebraDetPropagation, DetScalarMulPd_Issue260_Sentinel) {
  // SENTINEL for #260 (t2s mul/div/pow positivity propagation).
  //
  // Mathematically: det(α·C) = α^d · det(C), and for PD C the inner
  // det(C) gets `positive` via the terminal-tensor_det propagation
  // (α-2d). But α^d · positive is currently NOT folded to positive —
  // the t2s mul operator doesn't propagate `positive` through the
  // composition. So the outer expression has no positivity tag today.
  //
  // The EXPECT_FALSE here is INVERTED-on-purpose: this test passes
  // today because the gap is open, but a #260 implementation that
  // lands correct `positive * positive → positive` propagation will
  // make it fail. The failure is the desired signal — at that point
  // flip the expectation to EXPECT_TRUE and rename to ...IsPositive.
  // Sentinel framing reflected in the test name so a reader scanning
  // the test list knows it's a tracked-future-change marker rather
  // than a permanent contract.
  auto C = std::get<0>(make_tensor_variable(std::tuple{"C", 3, 2}));
  auto alpha = make_scalar_variable("alpha");
  assume_positive_definite(C);
  auto d = det(std::get<0>(alpha) * C);
  EXPECT_FALSE(d.data()->assumptions().contains(positive{}))
      << "Sentinel: flip to EXPECT_TRUE when #260 lands t2s mul positivity";
}

TEST(TensorAlgebraDetPropagation, DetTransPdGetsPositiveViaSymmetricShortcut) {
  // det(trans(PD C)) — actually DOES get the positive annotation,
  // for a non-obvious reason: PD ⇒ symmetric, and trans()'s symmetric
  // short-circuit returns the operand unchanged (since A^T = A for
  // symmetric A). So trans(C) IS C, and the recursive det(C) fires
  // the propagation normally. The "trans loses PD" gap I worried
  // about doesn't bite here because trans never builds a wrapper in
  // the first place. Lock in this happy-path behavior — a future
  // change to trans()'s symmetric-shortcut (e.g. always wrap for
  // hash consistency) would catch this test.
  auto C = std::get<0>(make_tensor_variable(std::tuple{"C", 3, 2}));
  assume_positive_definite(C);
  auto d = det(trans(C));
  EXPECT_TRUE(d.data()->assumptions().contains(positive{}))
      << "PD ⇒ symmetric, so trans() returns C unchanged and the "
         "recursive det(C) fires the propagation";
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
  // trans(B) * R where B ≠ R: the inner of the transpose (B) doesn't
  // match the other operand (R), so is_trans_of(lhs=trans(B), rhs=R)
  // returns false. The fold requires both: (i) one operand IS the
  // transpose of the other, AND (ii) the non-transposed operand is
  // orthogonal. The orthogonal annotation on R alone isn't enough
  // because requirement (i) fails. Lock-in against widening the gate
  // to "either side orthogonal, either side a transpose of anything".
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
// ─── #246 α-2b: PD/PSD propagation through tensor_inv ──────────────

TEST(TensorAlgebraPropagation, InvPdIsAlsoPd) {
  // inv(PD) is PD by definition — positive eigenvalues stay positive
  // under inversion. The propagation lives in tensor_inv's constructor
  // so all paths that produce a tensor_inv (including direct calls,
  // not via the factory fold for orthogonal) see it.
  auto C = std::get<0>(make_tensor_variable(std::tuple{"C", 3, 2}));
  assume_positive_definite(C);
  auto invC = inv(C);
  // The factory doesn't fold inv(PD) (only inv(orthogonal) folds);
  // so invC is a tensor_inv. Sanity-check that path.
  ASSERT_TRUE(is_same<tensor_inv>(invC));
  EXPECT_TRUE(is_positive_definite(invC));
  // PD => PSD (joint insertion); also implies symmetric.
  EXPECT_TRUE(is_positive_semidefinite(invC));
  EXPECT_TRUE(is_symmetric(invC));
}

TEST(TensorAlgebraPropagation, InvPsdStaysPsd) {
  // inv(PSD) propagates PSD. (A non-singular PSD is actually PD, but
  // we don't know that symbolically — stay conservative; eval throws
  // if actually singular.) PSD does NOT imply PD.
  auto C = std::get<0>(make_tensor_variable(std::tuple{"C", 3, 2}));
  assume_positive_semidefinite(C);
  auto invC = inv(C);
  ASSERT_TRUE(is_same<tensor_inv>(invC));
  EXPECT_TRUE(is_positive_semidefinite(invC));
  EXPECT_FALSE(is_positive_definite(invC))
      << "PSD should not strengthen to PD through propagation";
  EXPECT_TRUE(is_symmetric(invC));
}

TEST(TensorAlgebraPropagation, InvUnannotatedHasNoPdAnnotation) {
  // Negative case: an un-annotated rank-2 tensor's inv must NOT
  // gain PD/PSD on its own. Guards against over-eager propagation.
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  auto invA = inv(A);
  ASSERT_TRUE(is_same<tensor_inv>(invA));
  EXPECT_FALSE(is_positive_definite(invA));
  EXPECT_FALSE(is_positive_semidefinite(invA));
}

TEST(TensorAlgebraPropagation, InvPdThroughClearSpaceStillSymmetric) {
  // Cross-mechanism: input C is annotated PD (which sets the Symmetric
  // space tag). Even if the user clears the space, the PD assumption
  // still implies symmetric in is_symmetric()'s logic. inv(C) then
  // propagates the PD assumption, which RE-SETS the Symmetric space
  // on the result (via detail::set_symmetric_unless_more_specific).
  auto C = std::get<0>(make_tensor_variable(std::tuple{"C", 3, 2}));
  assume_positive_definite(C);
  C.data()->clear_space();
  ASSERT_FALSE(C.get().space().has_value());
  // Despite cleared space on input, the inv() result has the space
  // re-derived from the PD propagation.
  auto invC = inv(C);
  EXPECT_TRUE(is_positive_definite(invC));
  EXPECT_TRUE(invC.get().space().has_value())
      << "PD propagation should re-derive the symmetric space tag";
  EXPECT_TRUE(is_symmetric(invC));
}

TEST(TensorAlgebraPropagation, InvPdRank4MinorMajorPropagates) {
  // Rank-4 PD (e.g., a positive-definite elasticity tangent). The
  // factory rank-gate accepts rank-4 with Minor/MinorMajor annotation,
  // and the tensor_inv ctor's algebra-propagation path fires regardless
  // of rank.
  auto C = std::get<0>(make_tensor_variable(std::tuple{"C", 3, 4}));
  assume_minor_major(C);
  assume_positive_definite(C);
  auto invC = inv(C);
  ASSERT_TRUE(is_same<tensor_inv>(invC));
  EXPECT_TRUE(is_positive_definite(invC));
  EXPECT_TRUE(is_positive_semidefinite(invC));
  // The minor-major space tag is preserved by the rank-4 branch of
  // tensor_inv's space-propagation, independently of the algebra
  // propagation.
  EXPECT_TRUE(is_minor_major(invC));
}

TEST(TensorAlgebraPropagation, AssumePdAfterSkewResolvesAtCallSite) {
  // Pre-condition for the ctor-defense test below: confirm that
  // assume_positive_definite OVERWRITES a prior Skew space at the
  // assume() call site (per #245). This means the typical path never
  // reaches tensor_inv's ctor with the contradictory state.
  auto W = std::get<0>(make_tensor_variable(std::tuple{"W", 3, 2}));
  assume_skew(W);
  assume_positive_definite(W);
  EXPECT_TRUE(is_symmetric(W));
  EXPECT_FALSE(is_skew(W));
  auto invW = inv(W);
  EXPECT_TRUE(is_positive_definite(invW));
  EXPECT_TRUE(is_symmetric(invW));
  EXPECT_FALSE(is_skew(invW));
}

TEST(TensorAlgebraPropagation, InvCtorDefendsAgainstSkewSpaceWithPdAlgebra) {
  // Exercise tensor_inv's ctor ordering on the pathological state that
  // the assume_* call sites prevent but a direct-manager caller can
  // construct: Skew on the space field AND positive_definite in the
  // algebra manager. The ctor's space block propagates Skew to the
  // result first; then the algebra block calls
  // set_symmetric_unless_more_specific which overwrites Skew with Sym
  // (Skew is not a recognised sym subspace). Lock that ordering in so
  // a refactor doesn't reverse it and produce an inv that claims to
  // be both PD and Skew.
  //
  // Dim 2 (even): the inv() factory's odd-dim skew-singularity guard
  // doesn't fire, so we actually reach the tensor_inv ctor.
  auto W = std::get<0>(make_tensor_variable(std::tuple{"W", 2, 2}));
  assume_skew(W);
  // Bypass assume_positive_definite (which would overwrite the Skew
  // space). Insert PD/PSD directly into the algebra manager, leaving
  // the Skew space intact. This is the state the ctor must handle.
  W.data()->tensor_algebra_assumptions().insert(positive_definite{});
  W.data()->tensor_algebra_assumptions().insert(positive_semidefinite{});
  ASSERT_TRUE(is_skew(W));
  ASSERT_TRUE(is_positive_definite(W));
  auto invW = inv(W);
  EXPECT_TRUE(is_positive_definite(invW));
  EXPECT_TRUE(is_symmetric(invW))
      << "ctor's algebra block must overwrite the propagated Skew with Sym";
  EXPECT_FALSE(is_skew(invW));
}

TEST(TensorAlgebraPropagation, InvPdComposesWithVolumetric) {
  // assume_volumetric(C) sets {Sym, Vol}; assume_positive_definite
  // preserves the Vol subspace via the more-specific rule from #245.
  // inv(C) propagates both: PD remains, and the more-specific Vol
  // space ALSO remains (because detail::set_symmetric_unless_more_specific
  // only widens to {Sym, AnyTrace} if no Sym subspace was set).
  auto P = std::get<0>(make_tensor_variable(std::tuple{"P", 3, 2}));
  assume_volumetric(P);
  assume_positive_definite(P);
  ASSERT_TRUE(is_volumetric(P));
  ASSERT_TRUE(is_positive_definite(P));
  auto invP = inv(P);
  EXPECT_TRUE(is_positive_definite(invP));
  // Vol IS preserved by tensor_inv's rank-2 space-propagation
  // (Volumetric perm in {Sym subspaces}; not Dev/Harm so kept).
  EXPECT_TRUE(is_volumetric(invP));
}

TEST(TensorAlgebraPropagation, InvInvPdReducesToCWithPdIntact) {
  // inv() factory's inv(inv(A)) → A short-circuit returns the inner
  // expression unchanged. Since C carried PD/PSD/Sym before, the
  // collapsed result trivially still has them — no separate
  // propagation step needed. Lock in the transitivity: a fold that
  // sheds annotations on collapse would silently weaken downstream
  // reasoning.
  auto C = std::get<0>(make_tensor_variable(std::tuple{"C", 3, 2}));
  assume_positive_definite(C);
  auto invInvC = inv(inv(C));
  EXPECT_EQ(invInvC, C) << "inv(inv(A)) should structurally collapse to A";
  EXPECT_TRUE(is_positive_definite(invInvC));
  EXPECT_TRUE(is_positive_semidefinite(invInvC));
  EXPECT_TRUE(is_symmetric(invInvC));
}

TEST(TensorAlgebraPropagation, AssumePdOnRank4WithNoSpaceLeavesSpaceUnset) {
  // Rank-gate on detail::set_symmetric_unless_more_specific: when the
  // input is rank-4 with no recognised sym subspace, the helper is a
  // no-op (writing a rank-2 Symmetric{} tag to a rank-4 tensor would
  // be a structural mismatch). The PD algebra annotation still goes
  // in. Lock in this defensive behavior: a regression that wrote
  // rank-2 Sym to rank-4 would silently produce malformed nodes.
  auto C = std::get<0>(make_tensor_variable(std::tuple{"C", 3, 4}));
  assume_positive_definite(C);
  EXPECT_TRUE(is_positive_definite(C));
  EXPECT_TRUE(is_positive_semidefinite(C));
  // Space stays unset — the user can opt in via assume_minor_major
  // or assume_major if they want a sym subspace at rank-4.
  EXPECT_FALSE(C.get().space().has_value())
      << "rank-4 PD with no prior space must NOT get a rank-2 Sym tag";
}

// ─── tensor_zero closed-form short-circuit (SymPy step 2) ────────────
// tensor_zero is the unique expression where Sym and Skew can BOTH be true:
// 0 = 0^T (symmetric) AND 0 = -0^T (skew). Every other expression has a
// single perm classification. This is safe to special-case in the helpers
// because tensor_zero never appears as a subterm — operators and factories
// collapse to a top-level zero, never wrap one as a child. See
// docs/sympy-assumption-redesign.md.

TEST(TensorAlgebraZeroAssumptions, ZeroIsSymmetricAtRank2) {
  auto Z = make_expression<tensor_zero>(std::size_t{3}, std::size_t{2});
  EXPECT_TRUE(is_symmetric(Z));
}

TEST(TensorAlgebraZeroAssumptions, ZeroIsSkewAtRank2) {
  auto Z = make_expression<tensor_zero>(std::size_t{3}, std::size_t{2});
  EXPECT_TRUE(is_skew(Z));
}

TEST(TensorAlgebraZeroAssumptions, ZeroIsBothSymmetricAndSkew) {
  // The simultaneity is the load-bearing fact for the SymPy redesign:
  // closed-form constants can satisfy multiple structural predicates
  // that would otherwise be mutually exclusive on the tensor_space variant.
  auto Z = make_expression<tensor_zero>(std::size_t{3}, std::size_t{2});
  EXPECT_TRUE(is_symmetric(Z) && is_skew(Z));
}

TEST(TensorAlgebraZeroAssumptions, ZeroIsSymmetricAtRank4) {
  // Rank-independent: 0 at any rank satisfies vacuous symmetry.
  auto Z = make_expression<tensor_zero>(std::size_t{3}, std::size_t{4});
  EXPECT_TRUE(is_symmetric(Z));
  EXPECT_TRUE(is_skew(Z));
}

TEST(TensorAlgebraZeroAssumptions, ZeroIsNotOtherAlgebraicProperties) {
  // Negative lock-ins on every helper that might develop a short-circuit:
  // zero has no space tag, so all the space-based helpers other than the
  // two explicitly short-circuited ones (is_symmetric, is_skew) must
  // return false. Zero is NOT orthogonal (0·0 ≠ I), NOT PD (0 is not
  // strictly positive), and we deliberately do NOT claim PSD here either
  // (PSD on zero is mathematically true but is_positive_semidefinite
  // queries the algebra-manager set, which doesn't include zero). If a
  // future helper develops its own short-circuit, this test will surface
  // the new behavior.
  auto Z = make_expression<tensor_zero>(std::size_t{3}, std::size_t{2});
  EXPECT_FALSE(is_volumetric(Z));
  EXPECT_FALSE(is_deviatoric(Z));
  EXPECT_FALSE(is_minor(Z));
  EXPECT_FALSE(is_minor_major(Z));
  EXPECT_FALSE(is_orthogonal(Z));
  EXPECT_FALSE(is_positive_definite(Z));
  EXPECT_FALSE(is_positive_semidefinite(Z));
}

// ─── identity_tensor closed-form pre-annotation (SymPy step 2) ────────
// Same pattern as tensor_to_scalar_zero/one: the constructor writes the
// structural classification into m_tensor_space. is_symmetric(I) etc. now
// answer true automatically. No visitor walk needed at this step.

TEST(TensorAlgebraIdentityAssumptions, Rank2IsSymmetric) {
  auto I = make_expression<identity_tensor>(std::size_t{3}, std::size_t{2});
  EXPECT_TRUE(is_symmetric(I));
}

TEST(TensorAlgebraIdentityAssumptions, Rank2IsNotSkew) {
  // I ≠ -I, so the identity is symmetric but NOT skew. Negative lock-in.
  auto I = make_expression<identity_tensor>(std::size_t{3}, std::size_t{2});
  EXPECT_FALSE(is_skew(I));
}

TEST(TensorAlgebraIdentityAssumptions, Rank4MinorIdentityIsMinorMajor) {
  // I_{ijkl} = δ_ik · δ_jl. Has minor symmetry (swap (i,j) or (k,l)) AND
  // major symmetry (swap (ij)↔(kl)).
  auto I = make_expression<identity_tensor>(std::size_t{3}, std::size_t{4});
  EXPECT_TRUE(is_minor_major(I));
}

TEST(TensorAlgebraIdentityAssumptions, Rank4MinorIdentityIsSymmetric) {
  // C1 lock-in: is_symmetric must also return true for the rank-4 minor
  // identity. Pre-fix the answer was false because classify_space mapped
  // MinorMajor → Other; the explicit MinorMajor branch in is_symmetric
  // fixes this. The rank-4 minor identity is the canonical fully-
  // symmetric rank-4 tensor.
  auto I = make_expression<identity_tensor>(std::size_t{3}, std::size_t{4});
  EXPECT_TRUE(is_symmetric(I));
}

TEST(TensorAlgebraIdentityAssumptions, Rank6IsUnclassified_OpenDecision) {
  // Doc open decision #3: higher-rank identity is left unset because the
  // variant has no general "all-pairs-minor" alternative. Lock in the
  // CURRENT behavior so a future change is explicit. Mathematically,
  // rank-6 minor identity δ_il·δ_jm·δ_kn does have full symmetry; if
  // 1.1 adds a rank-6 classification, this test should flip.
  auto I = make_expression<identity_tensor>(std::size_t{3}, std::size_t{6});
  EXPECT_FALSE(is_symmetric(I))
      << "rank-6 identity unclassified — see open decision in scoping doc";
  EXPECT_FALSE(is_minor_major(I));
}

TEST(TensorAlgebraIdentityAssumptions, Rank2IsNotOtherAlgebraicProperties) {
  // Negative-test matrix for rank-2 identity. I is symmetric, NOT skew,
  // NOT volumetric (vol(I) = (1/d)·tr(I)·I = I, so I has nonzero
  // volumetric part but is not ITSELF in the volumetric subspace as a
  // strict subspace classification — the codebase keeps the Symmetric
  // tag, not the VolumetricTag), NOT a rank-4 classification, NOT
  // orthogonal (orthogonal := R·Rᵀ = I, true for I but not annotated as
  // such today — locked in as the deliberate omission), NOT marked PD.
  auto I = make_expression<identity_tensor>(std::size_t{3}, std::size_t{2});
  EXPECT_FALSE(is_volumetric(I));
  EXPECT_FALSE(is_deviatoric(I));
  EXPECT_FALSE(is_minor(I));
  EXPECT_FALSE(is_major(I));
  EXPECT_FALSE(is_minor_major(I));
  EXPECT_FALSE(is_orthogonal(I));
  EXPECT_FALSE(is_positive_definite(I));
  EXPECT_FALSE(is_positive_semidefinite(I));
}

TEST(TensorAlgebraIdentityAssumptions, MovePreservesAnnotationRank2) {
  // Defaulted move ctor routes through tensor_expression's 1-arg move
  // (preserves m_tensor_space). The previous explicit-ctor + re-apply
  // pattern was a footgun; this test guards against a regression that
  // reintroduces the 3-arg base ctor path.
  identity_tensor src{std::size_t{3}, std::size_t{2}};
  identity_tensor moved{std::move(src)};
  ASSERT_TRUE(moved.space().has_value());
  EXPECT_TRUE(std::holds_alternative<Symmetric>(moved.space()->perm));
}

TEST(TensorAlgebraIdentityAssumptions, CopyPreservesAnnotationRank2) {
  identity_tensor src{std::size_t{3}, std::size_t{2}};
  identity_tensor copy{src};
  ASSERT_TRUE(copy.space().has_value());
  EXPECT_TRUE(std::holds_alternative<Symmetric>(copy.space()->perm));
}

TEST(TensorAlgebraIdentityAssumptions, MovePreservesAnnotationRank4) {
  // Rank-4 move ctor coverage (cpp-pro F7 gap): the rank-4 branch
  // produces MinorMajor, distinct from rank-2 Symmetric. Both must
  // survive move construction.
  identity_tensor src{std::size_t{3}, std::size_t{4}};
  identity_tensor moved{std::move(src)};
  ASSERT_TRUE(moved.space().has_value());
  EXPECT_TRUE(std::holds_alternative<MinorMajor>(moved.space()->perm));
}

TEST(TensorAlgebraIdentityAssumptions, HashIsIndependentOfSpaceAnnotation) {
  // Architect finding A5: m_tensor_space is NOT part of update_hash_value,
  // so two identity_tensor instances must hash equal regardless of their
  // space annotation. Verified by constructing two and forcing one to
  // carry a contradictory tag (Skew on rank-2 identity) — semantically
  // wrong but mechanically valid. clear_space() is a no-op on closed-form
  // constants so we use set_space to drive the difference.
  identity_tensor with_default_annotation{std::size_t{3}, std::size_t{2}};
  identity_tensor with_contradictory_annotation{std::size_t{3}, std::size_t{2}};
  with_contradictory_annotation.set_space({Skew{}, AnyTraceTag{}});
  EXPECT_EQ(with_default_annotation.hash_value(),
            with_contradictory_annotation.hash_value())
      << "m_tensor_space must not contribute to the content-addressed hash";
}

TEST(TensorAlgebraIdentityAssumptions,
     HashIsIndependentOfSpaceAnnotationRank4) {
  // QA Q4: rank-4 companion. If a future update_hash_value adds a
  // MinorMajor branch only, the rank-2 test above wouldn't catch it.
  identity_tensor with_default_annotation{std::size_t{3}, std::size_t{4}};
  identity_tensor with_contradictory_annotation{std::size_t{3}, std::size_t{4}};
  with_contradictory_annotation.set_space({Skew{}, AnyTraceTag{}});
  EXPECT_EQ(with_default_annotation.hash_value(),
            with_contradictory_annotation.hash_value());
}

TEST(TensorAlgebraIdentityAssumptions, CopyPreservesAnnotationRank4) {
  // QA Q6 companion to MovePreservesAnnotationRank4.
  identity_tensor src{std::size_t{3}, std::size_t{4}};
  identity_tensor copy{src};
  ASSERT_TRUE(copy.space().has_value());
  EXPECT_TRUE(std::holds_alternative<MinorMajor>(copy.space()->perm));
}

TEST(TensorAlgebraIdentityAssumptions, ClearSpaceIsNoOp) {
  // C1 lock-in: closed-form constants override clear_space() to a no-op
  // because the structural classification is intrinsic to the type. A
  // caller that clears via a tensor_expression* base pointer must NOT
  // leave the constant unclassified. Without this override the projector
  // would return a UB-deref'd value from space() in release builds.
  auto I = make_expression<identity_tensor>(std::size_t{3}, std::size_t{2});
  ASSERT_TRUE(is_symmetric(I));
  I.data()->clear_space();
  EXPECT_TRUE(is_symmetric(I))
      << "clear_space on closed-form identity must be a no-op";
}

TEST(TensorAlgebraIdentityAssumptions, AssumeSkewThrowsOnConstant) {
  // SymPy step 4 supersedes the previous "user assertion overwrites
  // pre-annotation" contract: identity_tensor is a closed-form constant,
  // not a Symbol, so assume_skew(I) is a category error. Constants are
  // classified by their type, not by user assertion. The contradictory
  // case (I ≠ -I^T) the old test described is now impossible to write.
  auto I = make_expression<identity_tensor>(std::size_t{3}, std::size_t{2});
  ASSERT_TRUE(is_symmetric(I));
  EXPECT_THROW(assume_skew(I), invalid_assumption_error);
  EXPECT_TRUE(is_symmetric(I))
      << "throw must not partially mutate state before failing";
}

// ─── tensor_projector closed-form pre-annotation (SymPy step 2) ───────

TEST(TensorAlgebraProjectorAssumptions, PSymIsSymmetric) {
  auto P = P_sym(std::size_t{3});
  EXPECT_TRUE(is_symmetric(P));
}

TEST(TensorAlgebraProjectorAssumptions, PSkewIsSkew) {
  auto P = P_skew(std::size_t{3});
  EXPECT_TRUE(is_skew(P));
}

TEST(TensorAlgebraProjectorAssumptions, PVolIsVolumetric) {
  auto P = P_vol(std::size_t{3});
  EXPECT_TRUE(is_volumetric(P));
  // Vol is a subspace of Sym, so symmetric must also hold.
  EXPECT_TRUE(is_symmetric(P));
}

TEST(TensorAlgebraProjectorAssumptions, PDevIsDeviatoric) {
  auto P = P_devi(std::size_t{3});
  EXPECT_TRUE(is_deviatoric(P));
  EXPECT_TRUE(is_symmetric(P));
}

TEST(TensorAlgebraProjectorAssumptions, PSymIsNotOtherSpaceClassifications) {
  auto P = P_sym(std::size_t{3});
  EXPECT_FALSE(is_skew(P));
  EXPECT_FALSE(is_volumetric(P));
  EXPECT_FALSE(is_deviatoric(P));
  EXPECT_FALSE(is_minor_major(P));
  EXPECT_FALSE(is_orthogonal(P));
  EXPECT_FALSE(is_positive_definite(P));
}

TEST(TensorAlgebraProjectorAssumptions, PVolIsNotSkewOrDeviatoric) {
  auto P = P_vol(std::size_t{3});
  EXPECT_FALSE(is_skew(P));
  EXPECT_FALSE(is_deviatoric(P));
  EXPECT_FALSE(is_minor_major(P));
  EXPECT_FALSE(is_orthogonal(P));
}

TEST(TensorAlgebraProjectorAssumptions, PSkewIsNotOtherSpaceClassifications) {
  // QA Q1: P_skew had only one positive cell pinned (is_skew). Any
  // regression that incorrectly tags P_skew as symmetric (e.g. a wrong
  // classify_space branch) was invisible. Pin every negative cell.
  auto P = P_skew(std::size_t{3});
  EXPECT_FALSE(is_symmetric(P));
  EXPECT_FALSE(is_volumetric(P));
  EXPECT_FALSE(is_deviatoric(P));
  EXPECT_FALSE(is_minor_major(P));
  EXPECT_FALSE(is_orthogonal(P));
  EXPECT_FALSE(is_positive_definite(P));
}

TEST(TensorAlgebraProjectorAssumptions, PDevIsNotOtherSpaceClassifications) {
  // QA Q1: P_dev had 2/10 cells pinned. Companion negative matrix.
  auto P = P_devi(std::size_t{3});
  EXPECT_FALSE(is_skew(P));
  EXPECT_FALSE(is_volumetric(P));
  EXPECT_FALSE(is_minor_major(P));
  EXPECT_FALSE(is_orthogonal(P));
  EXPECT_FALSE(is_positive_definite(P));
}

TEST(TensorAlgebraProjectorAssumptions, AssumeSymmetricOnPSkewThrows) {
  // SymPy step 4: projectors are closed-form constants, not Symbols.
  // Was previously locked in as an overwrite contract; under SymPy that
  // contract is a category error.
  auto P = P_skew(std::size_t{3});
  ASSERT_TRUE(is_skew(P));
  EXPECT_THROW(assume_symmetric(P), invalid_assumption_error);
  EXPECT_TRUE(is_skew(P)) << "throw must not mutate state";
}

TEST(TensorAlgebraProjectorAssumptions, ClearSpaceIsNoOp) {
  // C1 companion: closed-form constant override applies to projectors too.
  // The release-mode UB was specifically because tensor_projector::space()
  // dereferences m_tensor_space.value() — if clear_space() had emptied
  // the optional, the next space() call would UB-deref.
  auto P = P_vol(std::size_t{3});
  ASSERT_TRUE(is_volumetric(P));
  P.data()->clear_space();
  EXPECT_TRUE(is_volumetric(P)) << "clear_space on projector must be a no-op";
}

TEST(TensorAlgebraProjectorAssumptions, AssumeSkewOnPVolThrows) {
  // SymPy step 4 supersedes the previous overwrite contract: P_vol is
  // a closed-form constant and rejects user assertion. The contradictory
  // case the old test described (Skew on a Vol projector) is now
  // impossible to express.
  auto P = P_vol(std::size_t{3});
  ASSERT_TRUE(is_volumetric(P));
  EXPECT_THROW(assume_skew(P), invalid_assumption_error);
  EXPECT_TRUE(is_volumetric(P)) << "throw must not mutate state";
}

// ─── Compound regression guards on pre-annotated identity ────────────
// Compound propagation for identity_tensor already works via the
// existing per-wrapper space-propagation in n_ary_tree (binary add) and
// tensor_scalar_mul. Lock in that the step-2 pre-annotation composes
// correctly with those existing folds — a step-3 visitor migration must
// not silently regress these.

TEST(TensorAlgebraCompoundRegression, IsSymmetricOfIdentityPlusIdentity) {
  auto I = make_expression<identity_tensor>(std::size_t{3}, std::size_t{2});
  auto I2 = make_expression<identity_tensor>(std::size_t{3}, std::size_t{2});
  // I + I propagates Sym through binary_op space-propagation. Already
  // works today via the n_ary_tree merge of two Sym tagged children.
  EXPECT_TRUE(is_symmetric(I + I2));
}

TEST(TensorAlgebraCompoundRegression, IsSymmetricOfTransOfIdentity) {
  auto I = make_expression<identity_tensor>(std::size_t{3}, std::size_t{2});
  // trans(I) collapses to I via trans()'s Sym-space short-circuit — I is
  // now Sym-tagged so the fold fires. Regression guard: a future trans
  // change that misses the Sym check would here return false.
  EXPECT_TRUE(is_symmetric(trans(I)));
}

TEST(TensorAlgebraCompoundRegression, IsSymmetricOfScalarTimesIdentity) {
  auto I = make_expression<identity_tensor>(std::size_t{3}, std::size_t{2});
  auto [alpha] = make_scalar_variable("alpha");
  // α·I — tensor_scalar_mul wrapper propagates m_rhs's space onto the
  // result. Already works today; locks in composition with the step-2
  // identity pre-annotation.
  EXPECT_TRUE(is_symmetric(alpha * I));
}

TEST(TensorAlgebraCompoundRegression,
     IsSymmetricOfVolPlusDevEqualsSymProjector) {
  // QA Q9: round-trip through projector algebra. P_vol + P_dev → P_sym
  // mathematically; binary_op's Vol + Dev join (or the projector
  // simplifier folding to P_sym) must surface as is_symmetric == true.
  // The architecturally interesting case: a future regression that loses
  // space-tag propagation on this exact compound would silently break
  // continuum-mechanics projector decompositions.
  auto Pv = P_vol(std::size_t{3});
  auto Pd = P_devi(std::size_t{3});
  EXPECT_TRUE(is_symmetric(Pv + Pd))
      << "P_vol + P_dev should resolve to a symmetric projector";
}

// ─── Step 3a: structural_propagation::preserve_unary lock-ins ─────────
// Two compound wrappers (tensor_negative and tensor_scalar_mul) now share
// a single helper for the pass-through pattern. Tests below exercise the
// helper through each caller separately, catching divergence between the
// two migration sites.

TEST(TensorAlgebraStructuralPropagation, PreserveUnaryCopiesChildSpace) {
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  assume_symmetric(A);
  // tensor_negative as consumer — its ctor calls preserve_unary(this, child).
  auto neg_A = -A;
  ASSERT_TRUE(neg_A.get().space().has_value());
  EXPECT_TRUE(std::holds_alternative<Symmetric>(neg_A.get().space()->perm));
}

TEST(TensorAlgebraStructuralPropagation, PreserveUnaryNoopWhenChildHasNoSpace) {
  // Negative case: child with no space tag must NOT cause the result to
  // get a wrong default. preserve_unary is no-op when the optional is empty.
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  auto neg_A = -A;
  EXPECT_FALSE(neg_A.get().space().has_value());
}

TEST(TensorAlgebraStructuralPropagation, ScalarMulPreservesRhsSpace) {
  // Different caller (tensor_scalar_mul) — independent behavioral lock-in.
  // Note: this tests OBSERVABLE behavior at the caller site, not that the
  // helper is shared with tensor_negative. A correct re-inline at either
  // call site would still pass; the test catches a regression, not a
  // refactor.
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  auto [alpha] = make_scalar_variable("alpha");
  assume_skew(A);
  auto scaled = alpha * A;
  ASSERT_TRUE(scaled.get().space().has_value());
  EXPECT_TRUE(std::holds_alternative<Skew>(scaled.get().space()->perm));
}

// ─── Step 4: tensor assume_* throws on non-Symbols ─────────────────
// SymPy-style strictness — only Symbols (named tensors) accept user
// assertions. Compounds, constants, and wrappers throw
// invalid_assumption_error. The compounds case is the most important
// (it was the silent-noop footgun); the constant/wrapper cases protect
// against accidental misuse of the API.

TEST(TensorAlgebraStrictAssume, AssumeSymmetricOnCompoundThrows) {
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  auto B = std::get<0>(make_tensor_variable(std::tuple{"B", 3, 2}));
  auto sum = A + B;
  EXPECT_THROW(assume_symmetric(sum), invalid_assumption_error);
}

TEST(TensorAlgebraStrictAssume, AssumeSkewOnCompoundThrows) {
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  auto neg = -A;
  EXPECT_THROW(assume_skew(neg), invalid_assumption_error);
}

TEST(TensorAlgebraStrictAssume, AssumePositiveDefiniteOnCompoundThrows) {
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  auto B = std::get<0>(make_tensor_variable(std::tuple{"B", 3, 2}));
  EXPECT_THROW(assume_positive_definite(A + B), invalid_assumption_error);
}

TEST(TensorAlgebraStrictAssume, AssumeOrthogonalOnCompoundThrows) {
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  auto B = std::get<0>(make_tensor_variable(std::tuple{"B", 3, 2}));
  EXPECT_THROW(assume_orthogonal(A + B), invalid_assumption_error);
}

TEST(TensorAlgebraStrictAssume, AssumeSymmetricOnIdentityConstantThrows) {
  auto I = make_expression<identity_tensor>(std::size_t{3}, std::size_t{2});
  EXPECT_THROW(assume_symmetric(I), invalid_assumption_error);
}

TEST(TensorAlgebraStrictAssume, AssumeSymmetricOnZeroConstantThrows) {
  auto Z = make_expression<tensor_zero>(std::size_t{3}, std::size_t{2});
  EXPECT_THROW(assume_symmetric(Z), invalid_assumption_error);
}

TEST(TensorAlgebraStrictAssume, AssumeMinorMajorOnCompoundThrows) {
  // QA Q1: assume_minor_major is rank-4-specific (every other helper is
  // rank-2). If a future change adds a separate code path for rank-4
  // and forgets the guard, the shared-guard rank-2 tests miss it.
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 4}));
  auto B = std::get<0>(make_tensor_variable(std::tuple{"B", 3, 4}));
  EXPECT_THROW(assume_minor_major(A + B), invalid_assumption_error);
}

TEST(TensorAlgebraStrictAssume, AssumeOnSymbolSucceeds) {
  // Positive case: the same call that throws on a compound succeeds on
  // a Symbol. Guards against an over-aggressive guard that throws on
  // legitimate inputs.
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  EXPECT_NO_THROW(assume_symmetric(A));
  EXPECT_TRUE(is_symmetric(A));
}

TEST(TensorAlgebraStructuralPropagation, PreserveUnaryOverwritesExistingSpace) {
  // QA review #3: the helper has no guard against overwriting. Today's
  // callers happen to construct `out` with empty m_tensor_space, but
  // nothing in the helper enforces that precondition. Lock in the
  // overwrite contract — the child's space wins — so a future caller
  // that passes a pre-tagged expression sees defined behavior instead
  // of relying on uninvestigated emergent behavior.
  auto child = std::get<0>(make_tensor_variable(std::tuple{"C", 3, 2}));
  assume_symmetric(child);
  // tensor_negative wraps child; then we manually pre-tag with Skew
  // (a state real wrappers wouldn't reach, but the helper must handle).
  tensor_negative out{child};
  out.set_space({Skew{}, AnyTraceTag{}});
  ASSERT_TRUE(std::holds_alternative<Skew>(out.space()->perm));
  structural_propagation::preserve_unary(out, child.get());
  EXPECT_TRUE(std::holds_alternative<Symmetric>(out.space()->perm))
      << "preserve_unary contract: child's space overwrites prior tag";
}

// ─── Step 5: expression_holder::assumption() variadic API (tensor) ───

TEST(TensorAlgebraAssumption, SingleStructuralFactSucceeds) {
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  A.assumption(Symmetric{});
  EXPECT_TRUE(is_symmetric(A));
}

TEST(TensorAlgebraAssumption, SingleAlgebraicFactSucceeds) {
  auto C = std::get<0>(make_tensor_variable(std::tuple{"C", 3, 2}));
  C.assumption(positive_definite{});
  EXPECT_TRUE(is_positive_definite(C));
  EXPECT_TRUE(is_positive_semidefinite(C)); // PD implies PSD
  EXPECT_TRUE(is_symmetric(C));             // PD implies Sym
}

TEST(TensorAlgebraAssumption, MultiFactCombinesStructuralAndAlgebraic) {
  auto C = std::get<0>(make_tensor_variable(std::tuple{"C", 3, 2}));
  C.assumption(Symmetric{}, positive_definite{});
  EXPECT_TRUE(is_symmetric(C));
  EXPECT_TRUE(is_positive_definite(C));
}

TEST(TensorAlgebraAssumption, ChainableReturnsSelf) {
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  A.assumption(Symmetric{}).assumption(positive_definite{});
  EXPECT_TRUE(is_symmetric(A));
  EXPECT_TRUE(is_positive_definite(A));
}

TEST(TensorAlgebraAssumption, ZeroFactsIsNoOp) {
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  EXPECT_NO_THROW(A.assumption());
  EXPECT_FALSE(A.get().space().has_value())
      << "0-fact assumption() must not assert anything";
}

TEST(TensorAlgebraAssumption, OnCompoundThrows) {
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  auto B = std::get<0>(make_tensor_variable(std::tuple{"B", 3, 2}));
  auto sum = A + B;
  EXPECT_THROW(sum.assumption(Symmetric{}), invalid_assumption_error);
}

TEST(TensorAlgebraAssumption, OnClosedFormConstantThrows) {
  auto I = make_expression<identity_tensor>(std::size_t{3}, std::size_t{2});
  EXPECT_THROW(I.assumption(Skew{}), invalid_assumption_error);
}

TEST(TensorAlgebraAssumption, OrthogonalDoesNotImplySymmetric) {
  // QA: orthogonal is the only algebraic fact WITHOUT the symmetric
  // cross-mechanism implication (PD/PSD both imply Sym via
  // set_symmetric_unless_more_specific). A future refactor that
  // accidentally routes orthogonal through that helper would silently
  // change semantics. Pin the distinction.
  auto Q = std::get<0>(make_tensor_variable(std::tuple{"Q", 3, 2}));
  Q.assumption(orthogonal{});
  EXPECT_TRUE(is_orthogonal(Q));
  EXPECT_FALSE(is_symmetric(Q))
      << "orthogonal must NOT imply Sym (R^T R = I doesn't make R symmetric)";
  EXPECT_FALSE(is_positive_definite(Q));
}

TEST(TensorAlgebraAssumption, SkewThenPDLastWriterWinsLeftToRight) {
  // QA: pin the documented left-to-right ordering by constructing a case
  // where order matters. assume(Skew{}, positive_definite{}):
  //   1. assume_skew sets space = {Skew, AnyTrace}
  //   2. assume_positive_definite calls set_symmetric_unless_more_specific
  //      which sees classify_space(Skew) — not in the Sym/Vol/Dev/Minor/
  //      MinorMajor guard — and OVERWRITES with {Symmetric, AnyTrace}.
  // Final state under left-to-right: Sym + PD (Skew lost).
  // Right-to-left would give: PD then Skew, with Skew the final space tag.
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  A.assumption(Skew{}, positive_definite{});
  EXPECT_TRUE(is_positive_definite(A));
  EXPECT_TRUE(is_symmetric(A))
      << "PD's set_symmetric_unless_more_specific overwrites the Skew tag";
  EXPECT_FALSE(is_skew(A))
      << "left-to-right contract: Skew was overwritten by PD's chain";
}

TEST(TensorAlgebraAssumption, ChainableReturnsSelfByIdentity) {
  // QA: address-of identity check, same as the scalar counterpart.
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  auto &ref = A.assumption(Symmetric{});
  EXPECT_EQ(&ref, &A);
}

// ─── Step 6: tensor side of the cross-domain consistency sweep ──────

TEST(TensorAlgebraStep6, TensorAssumeUniformGuardSampling) {
  // Sample all 10 tensor assume_* helpers on a compound. Each must
  // throw via require_symbol. Uniformity lock-in against a future
  // helper that forgets the guard.
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  auto B = std::get<0>(make_tensor_variable(std::tuple{"B", 3, 2}));
  auto sum = A + B;
  EXPECT_THROW(assume_symmetric(sum), invalid_assumption_error);
  EXPECT_THROW(assume_skew(sum), invalid_assumption_error);
  EXPECT_THROW(assume_volumetric(sum), invalid_assumption_error);
  EXPECT_THROW(assume_deviatoric(sum), invalid_assumption_error);
  EXPECT_THROW(assume_minor(sum), invalid_assumption_error);
  EXPECT_THROW(assume_major(sum), invalid_assumption_error);
  EXPECT_THROW(assume_minor_major(sum), invalid_assumption_error);
  EXPECT_THROW(assume_orthogonal(sum), invalid_assumption_error);
  EXPECT_THROW(assume_positive_definite(sum), invalid_assumption_error);
  EXPECT_THROW(assume_positive_semidefinite(sum), invalid_assumption_error);
}

// ─── Step 6: tensor compound propagation through Symbols ─────────────
// Parallel to the scalar PropagateAddBothPositive family — verify that
// space-propagation through n_ary_tree / tensor_scalar_mul / tensor_negative
// works for SYMBOL operands, not just constants (step-2's
// IsSymmetricOfIdentityPlusIdentity et al. covered constants only).

TEST(TensorAlgebraStep6, SymPlusSymIsSymmetric_Symbols) {
  // QA-flagged gap: tensor parallel to PropagateAddBothPositive. Sum of
  // two Sym Symbols inherits Sym via n_ary_tree's binary_op space join.
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  auto B = std::get<0>(make_tensor_variable(std::tuple{"B", 3, 2}));
  assume_symmetric(A);
  assume_symmetric(B);
  EXPECT_TRUE(is_symmetric(A + B));
}

TEST(TensorAlgebraStep6, SkewPlusSkewIsSkew_Symbols) {
  // Mirror for the Skew classification — separate variant alternative,
  // distinct space-join path.
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  auto B = std::get<0>(make_tensor_variable(std::tuple{"B", 3, 2}));
  assume_skew(A);
  assume_skew(B);
  EXPECT_TRUE(is_skew(A + B));
}

TEST(TensorAlgebraStep6, ScalarTimesSymIsSymmetric_Symbol) {
  // tensor_scalar_mul preserves rhs's space (step-3's preserve_unary
  // helper).
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  auto [alpha] = make_scalar_variable("alpha");
  assume_symmetric(A);
  EXPECT_TRUE(is_symmetric(alpha * A));
}

TEST(TensorAlgebraStep6, NegOfSymIsSymmetric_Symbol) {
  // tensor_negative preserves the child's space (same preserve_unary
  // helper). Sym Symbol → -Sym still Sym.
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  assume_symmetric(A);
  EXPECT_TRUE(is_symmetric(-A));
}

TEST(TensorAlgebraStep6, SymPlusUnannotatedIsNotSymmetric) {
  // Negative-case lock-in: the propagation requires BOTH operands to be
  // Sym. Without that the result has no space tag.
  auto A = std::get<0>(make_tensor_variable(std::tuple{"A", 3, 2}));
  auto B = std::get<0>(make_tensor_variable(std::tuple{"B", 3, 2}));
  assume_symmetric(A);
  // B is left unannotated
  EXPECT_FALSE(is_symmetric(A + B))
      << "n_ary_tree space-join requires all children to carry the tag";
}

TEST(TensorAlgebraStep6, ClosedFormConstantQueryConsistency) {
  // All closed-form constants answer is_* queries consistently — either
  // via helper short-circuit (tensor_zero) or ctor pre-annotation
  // (identity_tensor, tensor_projector). Pin a representative answer
  // from each category to lock in the consistency invariant.
  auto Z = make_expression<tensor_zero>(std::size_t{3}, std::size_t{2});
  auto I = make_expression<identity_tensor>(std::size_t{3}, std::size_t{2});
  auto P = P_vol(std::size_t{3});

  EXPECT_TRUE(is_symmetric(Z)) << "zero via helper short-circuit";
  EXPECT_TRUE(is_symmetric(I)) << "identity via ctor pre-annotation";
  EXPECT_TRUE(is_symmetric(P)) << "projector via ctor pre-annotation";

  // Negative consistency: all three reject orthogonal/PD without explicit
  // assertion (they're closed-form constants, not annotated as such).
  EXPECT_FALSE(is_orthogonal(Z));
  EXPECT_FALSE(is_orthogonal(I));
  EXPECT_FALSE(is_orthogonal(P));
}

} // namespace numsim::cas

#endif // TENSORALGEBRAASSUMETEST_H
