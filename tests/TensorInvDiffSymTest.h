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

// ── SkewRank2OddDimRejectedAtFactory ──────────────────────────────────────
// Contract check, not a diff test: the inv() factory itself rejects
// skew + odd-dim before the diff visitor ever sees it. Lock in the gate so
// a future refactor that removes the factory check would be caught here
// rather than producing a misleading inv-diff error.
TEST(TensorInvDiffSym, SkewRank2OddDimRejectedAtFactory) {
  auto X = make_expression<tensor>("X", std::size_t{3}, std::size_t{2});
  assume_skew(X);
  EXPECT_THROW(inv(X), invalid_expression_error)
      << "inv() factory must reject odd-dim skew (singular); diff visitor "
         "should never see this case";
}

// NOTE — Vol/Dev coverage lives elsewhere:
//
// `assume_volumetric(X)` / `assume_deviatoric(X)` set
// {perm=Symmetric, trace=VolumetricTag/DeviatoricTag}, which `is_symmetric()`
// recognizes as Sym subspaces. The dispatch correctly routes them through
// the sym kernel. We don't add a standalone numerical lock-in here because:
//   1. The numerical-diff approach can't match: symbolic diff w.r.t. a
//      vol-annotated X returns the P_vol-restricted Jacobian (see
//      tensor_differentiation.h's tensor_self-diff path which returns
//      P_vol for Vol inputs), but a numerical sym-perturbation explores
//      the larger Sym subspace — the two contract differently by design.
//   2. The dispatch IS covered indirectly by InvDevProjectorDiff in
//      tests/TensorDifferentiationTest.h (uses inv(dev(X)) where dev(X)'s
//      space is {Sym, Dev}), which exercises my new branch via the same
//      is_symmetric() path. That test's 1e-6 pass confirms Vol/Dev
//      dispatch works.
//
// If a future contributor wants a direct Vol/Dev lock-in for this kernel,
// the right approach is structural (e.g., hash-compare diff(inv(Vol_X), X)
// against a reference AST) rather than numerical.

// ── PdAnnotationWithClearedSpaceStillRoutesToSymKernel ────────────────────
// Soundness regression for the dispatch logic: assume_positive_definite
// writes {Sym, AnyTrace} to space AND inserts positive_definite into the
// algebra-assumption manager. If a user then calls clear_space(), the
// space tag is gone but the PD annotation remains. is_symmetric() picks
// the PD-implies-symmetric path; raw holds_alternative<Symmetric>(perm)
// would not. This test pins that the dispatch uses the former, so a
// future change reverting to raw variant checks is caught.
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
