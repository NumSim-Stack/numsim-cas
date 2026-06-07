#ifndef TENSORINVDIFFSYMTEST_H
#define TENSORINVDIFFSYMTEST_H

#include "cas_test_helpers.h"
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <tmech/tmech.h>

#include <numsim_cas/core/diff.h>
#include <numsim_cas/numsim_cas.h>
#include <numsim_cas/tensor/tensor_assume.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor/visitors/tensor_evaluator.h>

// ─── β-1: Symmetric-aware rank-2 inv differentiation (#246 / #250 rank-2) ──
//
// The rank-2 branch of tensor_differentiation::operator()(tensor_inv)
// switches kernels based on is_symmetric() / is_skew() (which consult both
// the projector-space tag and the algebra-assumption manager):
//
//   - Symmetric  → minor-symmetric kernel (½(A⁻¹_im A⁻¹_nj + A⁻¹_in A⁻¹_mj))
//   - Skew       → minor-antisymmetric kernel
//                  (½(A⁻¹_im A⁻¹_nj − A⁻¹_in A⁻¹_mj))
//   - General    → unchanged Magnus kernel (A⁻¹_im A⁻¹_nj)
//
// These tests use tmech::num_diff_central with type-matching perturbations
// (sym for symmetric inputs, skew for skew inputs) as the ground truth.
//
// Lambdas inside num_diff_central use an intermediate `auto data = ev.apply(
// expr)` to keep the shared_ptr alive during the implicit copy that the
// lambda's auto-return-by-value performs. The helper's `auto const&` return
// is safe-but-brittle; the local binding makes the lifetime explicit.

namespace numsim::cas {

namespace {

template <std::size_t Dim, std::size_t Rank>
auto const &as_tmech_invdiff(tensor_data_base<double> const &data) {
  return static_cast<tensor_data<double, Dim, Rank> const &>(data).data();
}

} // namespace

// ── SymmetricRank2InvDiffMatchesNumeric ───────────────────────────────────
// Annotate X symmetric, eval d(inv(X))/dX symbolically, compare to
// numerical diff that perturbs only the symmetric part of X. Locks in the
// minor-symmetric kernel via tmech::num_diff_central.
TEST(TensorInvDiffSym, SymmetricRank2InvDiffMatchesNumeric) {
  auto X = make_expression<tensor>("X", std::size_t{3}, std::size_t{2});
  assume_symmetric(X);
  auto expr = inv(X);
  auto d = diff(expr, X);
  ASSERT_TRUE(d.is_valid()) << "Expected valid derivative for inv(X)";

  // Build a well-conditioned symmetric PD input: random sym matrix + 10·I
  // to keep eigenvalues bounded away from 0 for numerical stability of inv.
  std::mt19937 rng(2026);
  std::normal_distribution<double> dist(0.0, 1.0);
  tmech::tensor<double, 3, 2> X_t;
  for (std::size_t i = 0; i < 9; ++i)
    X_t.raw_data()[i] = dist(rng);
  X_t = tmech::eval(tmech::sym(X_t));
  for (std::size_t i = 0; i < 3; ++i)
    X_t(i, i) += 10.0;
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);

  auto result = ev.apply(d);
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->rank(), 4u);

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = tmech::eval(tmech::sym(x));
        auto data = ev.apply(expr); // keep shared_ptr alive past `.data()` ref
        return as_tmech_invdiff<3, 2>(*data);
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(
      tmech::almost_equal(as_tmech_invdiff<3, 4>(*result), numdiff, 1e-6))
      << "Symmetric inv-diff kernel must match num_diff_central with sym "
         "perturbations";
}

// ── SkewRank2Dim2InvDiffMatchesNumeric ────────────────────────────────────
// Smallest (and only currently testable) even dim. The evaluator's
// MaxDim=3 ceiling blocks a dim=4 numerical lock-in that would exercise
// all 6 free off-diagonal entries of a 4×4 skew (and catch sign-flip /
// transpose errors in T_swap that 2D can't surface). Tracked: extending
// MaxDim would let us add the dim=4 case here.
TEST(TensorInvDiffSym, SkewRank2Dim2InvDiffMatchesNumeric) {
  auto X = make_expression<tensor>("X", std::size_t{2}, std::size_t{2});
  assume_skew(X);
  auto expr = inv(X);
  auto d = diff(expr, X);
  ASSERT_TRUE(d.is_valid()) << "Expected valid derivative for inv(skew X)";

  tmech::tensor<double, 2, 2> X_t;
  X_t(0, 0) = 0.0;
  X_t(0, 1) = 2.5;
  X_t(1, 0) = -2.5;
  X_t(1, 1) = 0.0;
  auto X_ptr = std::make_shared<tensor_data<double, 2, 2>>(X_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);

  auto result = ev.apply(d);
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->rank(), 4u);

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = tmech::eval(tmech::skew(x));
        auto data = ev.apply(expr);
        return as_tmech_invdiff<2, 2>(*data);
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(
      tmech::almost_equal(as_tmech_invdiff<2, 4>(*result), numdiff, 1e-6))
      << "Skew inv-diff kernel must match num_diff_central with skew "
         "perturbations (dim=2)";
}

// ── GeneralRank2InvDiffUnchanged ──────────────────────────────────────────
// No annotation: the general Magnus kernel still fires. Lock-in regression
// test for the unannotated path so a future change to the branch ordering
// (e.g., always go through the sym branch) is caught.
TEST(TensorInvDiffSym, GeneralRank2InvDiffUnchanged) {
  auto X = make_expression<tensor>("X", std::size_t{3}, std::size_t{2});
  // No annotation — general tensor.
  auto expr = inv(X);
  auto d = diff(expr, X);
  ASSERT_TRUE(d.is_valid()) << "Expected valid derivative for inv(general X)";

  std::mt19937 rng(4242);
  std::normal_distribution<double> dist(0.0, 1.0);
  tmech::tensor<double, 3, 2> X_t;
  for (std::size_t i = 0; i < 9; ++i)
    X_t.raw_data()[i] = dist(rng);
  // Make diagonally dominant so the inverse is well-conditioned.
  for (std::size_t i = 0; i < 3; ++i)
    X_t(i, i) += 10.0;
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);

  auto result = ev.apply(d);
  ASSERT_NE(result, nullptr);

  // Numerical perturbation is unconstrained (no sym/skew projection).
  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = x;
        auto data = ev.apply(expr);
        return as_tmech_invdiff<3, 2>(*data);
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(
      tmech::almost_equal(as_tmech_invdiff<3, 4>(*result), numdiff, 1e-6))
      << "General inv-diff kernel (Magnus) must match num_diff_central with "
         "unconstrained perturbations — regression check on the un-annotated "
         "path";
}

// ── SkewClearedSpaceDispatchFallsThroughToMagnus ──────────────────────────
// Asymmetry between is_symmetric() and is_skew(): the former consults the
// algebra-assumption manager for PD/PSD ⇒ symmetric, the latter does NOT
// (skew has no algebra-manager backing). Consequence: clearing the space
// on a skew-annotated tensor drops the classification entirely, and the
// dispatch falls through to general Magnus.
//
// Verification uses a *structural* hash comparison between two diff results:
// one with `assume_skew` still annotated (skew kernel: wraps a
// tensor_scalar_mul of `half` × `tensor_add(T_base, tensor_negative(T_swap))`
// — n_ary_tree stores the negation via a wrapping `tensor_negative` node,
// not via an internal sign), one after `clear_space` (Magnus kernel: bare
// `T_base = otimes(...)`). The two AST shapes differ in NODE TYPES, not
// just coefficients, so the hash distinguishes them — the n_ary_tree
// coefficient-exclusion rule doesn't affect this comparison.
TEST(TensorInvDiffSym, SkewClearedSpaceDispatchFallsThroughToMagnus) {
  auto X = make_expression<tensor>("X", std::size_t{2}, std::size_t{2});
  assume_skew(X);
  ASSERT_TRUE(is_skew(X)) << "precondition: skew annotated via space";
  auto diff_with_skew = diff(inv(X), X);
  ASSERT_TRUE(diff_with_skew.is_valid());

  X.data()->clear_space();
  EXPECT_FALSE(is_skew(X))
      << "is_skew() drops to false because skew has no algebra-manager "
         "fallback (unlike is_symmetric which keeps PD/PSD)";
  auto diff_after_clear = diff(inv(X), X);
  ASSERT_TRUE(diff_after_clear.is_valid());

  EXPECT_NE(diff_with_skew.get().hash_value(),
            diff_after_clear.get().hash_value())
      << "After clearing space the dispatch must fall through to general "
         "Magnus (no swap term, no ½), yielding a structurally different "
         "AST from the skew-kernel result above";
}

// ── VolumetricDispatchEndToEndDiffersFromUnannotated ──────────────────────
// `assume_volumetric(X)` sets {perm=Symmetric, trace=VolumetricTag}.
// is_symmetric(X) returns true (Vol is a Sym subspace per classify_space),
// so the dispatch routes through the sym kernel.
//
// The hash inequality below catches a Vol-dispatch regression but the
// signal is end-to-end: diff(inv(vol_X)) differs from diff(inv(unann_Y))
// in TWO ways — (a) the leaf base case diff(X,X) returns P_vol for vol-X
// vs identity_tensor for unannotated Y (per tensor_differentiation.h:73-92's
// classify_space switch), AND (b) the inv-diff kernel choice (sym vs
// Magnus). Either regressing would break the hash inequality, so this
// test is a coverage net for "annotation is respected end-to-end" rather
// than an isolated "sym kernel fired" lock-in. The kernel-isolation
// version lives in PdAlgebraDispatchIsolatesSymKernel below.
TEST(TensorInvDiffSym, VolumetricDispatchEndToEndDiffersFromUnannotated) {
  auto X = make_expression<tensor>("X", std::size_t{3}, std::size_t{2});
  assume_volumetric(X);
  ASSERT_TRUE(is_symmetric(X)) << "Vol implies symmetric";
  auto diff_with_vol = diff(inv(X), X);
  ASSERT_TRUE(diff_with_vol.is_valid());

  auto Y = make_expression<tensor>("X", std::size_t{3}, std::size_t{2});
  // Y has the same name as X so leaf hashes match. Annotation is the
  // only meaningful difference between the two inputs.
  ASSERT_FALSE(is_symmetric(Y));
  auto diff_unannotated = diff(inv(Y), Y);
  ASSERT_TRUE(diff_unannotated.is_valid());

  EXPECT_NE(diff_with_vol.get().hash_value(),
            diff_unannotated.get().hash_value())
      << "Vol-annotated input must produce a different diff AST than the "
         "unannotated case (via leaf base case P_vol AND/OR sym kernel)";
}

// ── PdAlgebraDispatchIsolatesSymKernel ────────────────────────────────────
// Kernel-isolation lock-in: uses `assume_positive_definite(X); clear_space`
// so X has PD in the algebra-manager but no space tag. Two consequences:
//   1. is_symmetric(X) returns true (via the algebra-manager path) →
//      dispatch picks the sym kernel.
//   2. diff(X, X) at the leaf base case sees empty space → returns
//      identity_tensor (NOT a projector), same as the unannotated case.
// So the ONLY structural difference between this diff and an unannotated
// diff is the kernel choice. Hash inequality therefore pins the kernel
// dispatch in isolation, unlike the end-to-end Volumetric test above.
TEST(TensorInvDiffSym, PdAlgebraDispatchIsolatesSymKernel) {
  auto X = make_expression<tensor>("X", std::size_t{3}, std::size_t{2});
  assume_positive_definite(X);
  X.data()->clear_space();
  ASSERT_TRUE(is_symmetric(X))
      << "precondition: PD-without-space still classifies as symmetric "
         "via the algebra-manager path";
  ASSERT_FALSE(X.get().space().has_value())
      << "precondition: leaf base case will return identity_tensor (no "
         "projector dispatch), matching the unannotated path";
  auto diff_with_pd = diff(inv(X), X);
  ASSERT_TRUE(diff_with_pd.is_valid());

  auto Y = make_expression<tensor>("X", std::size_t{3}, std::size_t{2});
  ASSERT_FALSE(is_symmetric(Y));
  auto diff_unannotated = diff(inv(Y), Y);
  ASSERT_TRUE(diff_unannotated.is_valid());

  EXPECT_NE(diff_with_pd.get().hash_value(),
            diff_unannotated.get().hash_value())
      << "With identical leaf base cases (both return identity_tensor), "
         "the hash inequality MUST come from the kernel choice (sym vs "
         "Magnus). A future change that breaks the algebra-manager → "
         "sym-kernel dispatch would be caught here.";
}

// LIMITATIONS NOTE — dim=4 sign-flip detection is not currently testable.
// TODO(#266): when the inner_product_wrapper hash gap closes, the deleted
// SkewRank2Dim4StructuralLockIn test (removed in commit ad4d824) can be
// revived; see the issue for the kernel signature mismatch.
//
// A naive hash-comparison structural lock-in at dim=4 cannot distinguish
// the sym kernel (T_base + T_swap)·½ from the skew kernel
// (T_base − T_swap)·½ for THREE independent reasons:
//
//   1. n_ary_tree's hash EXCLUDES coefficients (documented design in
//      CLAUDE.md / n_ary_tree.h), so the sign of T_swap (via tensor_negative
//      wrapping) doesn't flow into the add-node's hash beyond the child
//      node's own hash, which is the same for +T_swap and -T_swap-wrapped-
//      in-negative... actually subtle: tensor_negative IS a distinct node,
//      so this one is partly covered. The deeper issue is below.
//   2. tensor_scalar_mul's hash, when LHS is a scalar_constant, COLLAPSES
//      to the RHS hash (intentional design rule for `c·T == T` hash
//      equality — see operators/scalar/tensor_scalar_mul.h:31-39). So
//      `(T_base + T_swap) * half` hashes identically to `T_base + T_swap`,
//      and `(T_base - T_swap) * half` hashes identically to `T_base -
//      T_swap`. The ½ factor is invisible.
//   3. inner_product_wrapper does NOT hash its contraction indices
//      (issue #266) — so a swap of {3,4}/{1,2} → {1,2}/{3,4} on the
//      outer contraction wouldn't be caught either.
//
// And the evaluator's MaxDim=3 ceiling blocks numerical validation at
// dim=4.
//
// What we DO catch:
//   - sym vs general dispatch (different node hierarchy via the
//     tensor_add/tensor_scalar_mul wrappers) — locked in by
//     SkewClearedSpaceDispatchFallsThroughToMagnus,
//     VolumetricDispatchEndToEndDiffersFromUnannotated, and
//     PdAlgebraDispatchIsolatesSymKernel.
//   - sym kernel correctness at dim=3 — SymmetricRank2InvDiffMatchesNumeric.
//   - skew kernel sign correctness at dim=2 —
//     SkewRank2Dim2InvDiffMatchesNumeric (only 1 free off-diagonal entry —
//     not a thorough check; the kernel works there but doesn't exercise
//     the full minor-antisymmetric structure).
//
// What we DON'T catch:
//   - Sign-flip in T_swap at dim ≥ 4 (sym ↔ skew kernel swap).
//   - Transpose errors in T_swap's index sequences at dim ≥ 4.
//   - A ½-coefficient regression at any dim (per reason 2 above).
//
// Until #266 lands (and ideally also a hash extension to capture
// scalar_constant coefficients in tensor_scalar_mul), the math has to be
// trusted from the derivation in tensor_differentiation.cpp's doc comment.

// ── PdAnnotationWithClearedSpaceStillRoutesToSymKernel ────────────────────
// Soundness regression for the dispatch logic: assume_positive_definite
// writes {Sym, AnyTrace} to space AND inserts positive_definite into the
// algebra-assumption manager. If a user then calls clear_space(), the
// space tag is gone but the PD annotation remains. is_symmetric() picks
// the PD-implies-symmetric path; raw holds_alternative<Symmetric>(perm)
// would not. This test pins that the dispatch uses the former, so a
// future change reverting to raw variant checks is caught.
//
// IMPORTANT TIMING: clear_space() is called BEFORE inv()/diff(). The
// dispatch is queried lazily at diff() time, so the cleared space is
// what's seen. If a future refactor caches the kernel choice at inv()
// construction time, this test would silently start exercising the
// wrong path (it would get the sym kernel from the still-Sym-annotated
// X at inv()-time, not from the post-clear query). The lazy-dispatch
// assumption is load-bearing for what this test actually pins.
TEST(TensorInvDiffSym, PdAnnotationWithClearedSpaceStillRoutesToSymKernel) {
  auto X = make_expression<tensor>("X", std::size_t{3}, std::size_t{2});
  assume_positive_definite(X);
  X.data()->clear_space();
  ASSERT_FALSE(X.get().space().has_value())
      << "precondition: space cleared, only algebra-manager PD remains";
  ASSERT_TRUE(is_symmetric(X))
      << "precondition: PD ⇒ symmetric via algebra-manager path";

  auto expr = inv(X);
  auto d = diff(expr, X);
  ASSERT_TRUE(d.is_valid());

  // Build a PD input numerically.
  std::mt19937 rng(9001);
  std::normal_distribution<double> dist(0.0, 1.0);
  tmech::tensor<double, 3, 2> X_t;
  for (std::size_t i = 0; i < 9; ++i)
    X_t.raw_data()[i] = dist(rng);
  X_t = tmech::eval(tmech::sym(X_t));
  for (std::size_t i = 0; i < 3; ++i)
    X_t(i, i) += 10.0;
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);

  auto result = ev.apply(d);
  ASSERT_NE(result, nullptr);

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = tmech::eval(tmech::sym(x));
        auto data = ev.apply(expr);
        return as_tmech_invdiff<3, 2>(*data);
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(
      tmech::almost_equal(as_tmech_invdiff<3, 4>(*result), numdiff, 1e-6))
      << "PD-with-cleared-space must still route through sym kernel via "
         "is_symmetric()'s algebra-manager check";
}

} // namespace numsim::cas

#endif // TENSORINVDIFFSYMTEST_H
