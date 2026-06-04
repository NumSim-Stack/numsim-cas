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
// switches kernels based on the input's space() annotation:
//
//   - Symmetric  → minor-symmetric kernel (½(A⁻¹_im A⁻¹_nj + A⁻¹_in A⁻¹_mj))
//   - Skew       → minor-antisymmetric kernel (½(A⁻¹_im A⁻¹_nj − A⁻¹_in
//   A⁻¹_mj))
//   - General    → unchanged Magnus kernel (A⁻¹_im A⁻¹_nj)
//
// These tests use tmech::num_diff_central with type-matching perturbations
// (sym for symmetric inputs, skew for skew inputs) as the ground truth.

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
// minor-symmetric kernel.
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
        return as_tmech_invdiff<3, 2>(*ev.apply(expr));
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(
      tmech::almost_equal(as_tmech_invdiff<3, 4>(*result), numdiff, 1e-6))
      << "Symmetric inv-diff kernel must match num_diff_central with sym "
         "perturbations";
}

// ── SkewRank2EvenDimInvDiffMatchesNumeric ─────────────────────────────────
// Annotate X skew (dim 2, even — odd-dim skew is singular and rejected by
// the inv() factory). Eval and compare to numerical diff perturbing only
// the skew part.
TEST(TensorInvDiffSym, SkewRank2EvenDimInvDiffMatchesNumeric) {
  auto X = make_expression<tensor>("X", std::size_t{2}, std::size_t{2});
  assume_skew(X);
  auto expr = inv(X);
  auto d = diff(expr, X);
  ASSERT_TRUE(d.is_valid()) << "Expected valid derivative for inv(skew X)";

  // 2D skew tensor X = [[0, a], [-a, 0]] with a ≠ 0 is invertible
  // (det = a²). Build with a away from zero for numerical stability.
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
        // Perturb only the skew part; symmetric component is "frozen" at 0
        // to respect the assume_skew annotation.
        X_ptr->data() = tmech::eval(tmech::skew(x));
        return as_tmech_invdiff<2, 2>(*ev.apply(expr));
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(
      tmech::almost_equal(as_tmech_invdiff<2, 4>(*result), numdiff, 1e-6))
      << "Skew inv-diff kernel must match num_diff_central with skew "
         "perturbations";
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
        return as_tmech_invdiff<3, 2>(*ev.apply(expr));
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
  EXPECT_THROW(
      { [[maybe_unused]] auto e = inv(X); }, invalid_expression_error)
      << "inv() factory must reject odd-dim skew (singular); diff visitor "
         "should never see this case";
}

} // namespace numsim::cas

#endif // TENSORINVDIFFSYMTEST_H
