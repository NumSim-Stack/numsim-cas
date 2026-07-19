#ifndef ISOTROPICTENSORFUNCTIONTEST_H
#define ISOTROPICTENSORFUNCTIONTEST_H

#include <cmath>
#include <gtest/gtest.h>
#include <memory>

#include <numsim_cas/core/diff.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_isotropic_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor/visitors/tensor_evaluator.h>

namespace numsim::cas {

namespace isofn_detail {

using T2 = tmech::tensor<double, 3, 2>;

inline std::shared_ptr<tensor_data<double, 3, 2>> data_from(T2 const &t) {
  auto ptr = std::make_shared<tensor_data<double, 3, 2>>();
  auto *dst = ptr->raw_data();
  auto const *src = t.raw_data();
  for (std::size_t i = 0; i < 9; ++i)
    dst[i] = src[i];
  return ptr;
}

template <std::size_t Dim, std::size_t Rank>
auto const &as_t(tensor_data_base<double> const &d) {
  return static_cast<tensor_data<double, Dim, Rank> const &>(d).data();
}

} // namespace isofn_detail

// ─── Value: diagonal exact ─────────────────────────────────────────────
TEST(IsotropicFn, DiagonalValues) {
  using namespace isofn_detail;
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  T2 A_val;
  A_val = {4.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 9.0};
  ev.set(A, data_from(A_val));

  auto s = ev.apply(sqrt(A));
  EXPECT_TRUE(tmech::almost_equal(as_t<3, 2>(*s), T2{2, 0, 0, 0, 1, 0, 0, 0, 3},
                                  1e-10));
  auto l = ev.apply(log(A));
  EXPECT_TRUE(tmech::almost_equal(
      as_t<3, 2>(*l), T2{std::log(4.0), 0, 0, 0, 0, 0, 0, 0, std::log(9.0)},
      1e-10));
  auto e = ev.apply(exp(A));
  EXPECT_TRUE(tmech::almost_equal(
      as_t<3, 2>(*e),
      T2{std::exp(4.0), 0, 0, 0, std::exp(1.0), 0, 0, 0, std::exp(9.0)}, 1e-9));
}

// ─── Value: round-trips (non-diagonal SPD) ─────────────────────────────
TEST(IsotropicFn, RoundTrips) {
  using namespace isofn_detail;
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  T2 A_val;
  A_val = {4.0, 1.0, 0.0, 1.0, 3.0, 0.0, 0.0, 0.0, 2.0};
  ev.set(A, data_from(A_val));

  auto explog = ev.apply(exp(log(A)));
  EXPECT_TRUE(tmech::almost_equal(as_t<3, 2>(*explog), A_val, 1e-9));
  auto S = sqrt(A);
  auto sq = ev.apply(inner_product(S, sequence{2}, S, sequence{1}));
  EXPECT_TRUE(tmech::almost_equal(as_t<3, 2>(*sq), A_val, 1e-9));
}

// ─── Tangent (distinct eigenvalues) matches finite differences ─────────
TEST(IsotropicFn, LogTangentDistinctMatchesFD) {
  using namespace isofn_detail;
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  T2 A_val;
  A_val = {4.0, 1.0, 0.0, 1.0, 3.0, 0.0, 0.0, 0.0, 2.0}; // eigs ~ {2,2.38,4.62}
  T2 H;
  H = {0.1, 0.2, 0.0, 0.2, 0.3, 0.1, 0.0, 0.1, 0.15};

  auto dL = diff(log(A), A);
  ASSERT_TRUE(dL.is_valid());
  EXPECT_EQ(dL.get().rank(), std::size_t{4});

  ev.set(A, data_from(A_val));
  auto D_data = ev.apply(dL);
  auto analytic = tmech::eval(tmech::dcontract(as_t<3, 4>(*D_data), H));

  const double t = 1e-6;
  ev.set(A, data_from(T2{tmech::eval(A_val + t * H)}));
  auto Lp = ev.apply(log(A));
  auto Lp_t = as_t<3, 2>(*Lp);
  ev.set(A, data_from(T2{tmech::eval(A_val - t * H)}));
  auto Lm = ev.apply(log(A));
  auto fd = tmech::eval((Lp_t - as_t<3, 2>(*Lm)) / (2.0 * t));
  EXPECT_TRUE(tmech::almost_equal(analytic, fd, 1e-6));
}

// ─── THE payoff: tangent stays finite & correct at COINCIDENT eigenvalues.
// A = [[3,1,1],[1,3,1],[1,1,3]] has eigenvalues {2, 2, 5} (2 doubled). log(A)
// is smooth, so d log(A)/dA is finite — but the naive 1/(λᵢ−λⱼ) form is nan
// here. Checked against a closed-form built from the invariant spectral
// projectors (λ=5 direction (1,1,1)/√3 ⇒ E₂ = ⅓·ones, P = I − E₂), so the
// reference is independent of the arbitrary eigenvector split in the
// degenerate subspace. A finite-difference reference is unreliable at exact
// degeneracy (log(A±tH) has an ill-conditioned E₀−E₁ component).
TEST(IsotropicFn, LogTangentCoincidentClosedForm) {
  using namespace isofn_detail;
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  T2 A_val;
  A_val = {3.0, 1.0, 1.0, 1.0, 3.0, 1.0, 1.0, 1.0, 3.0};
  T2 H;
  H = {0.1, 0.2, 0.05, 0.2, 0.3, 0.1, 0.05, 0.1, 0.15};

  auto dL = diff(log(A), A);
  ev.set(A, data_from(A_val));
  auto D_data = ev.apply(dL);
  auto analytic = tmech::eval(tmech::dcontract(as_t<3, 4>(*D_data), H));

  // Finite — the whole point of the divided-difference guard.
  for (std::size_t i = 0; i < 9; ++i)
    ASSERT_TRUE(std::isfinite(analytic.raw_data()[i])) << "component " << i;

  // Closed form: 0.5·PHP + dd·(P H E₂ + E₂ H P) + 0.2·E₂ H E₂,
  // with 0.5 = log'(2), 0.2 = log'(5), dd = (log2−log5)/(2−5).
  const double u = 1.0 / 3.0;
  T2 E2{u, u, u, u, u, u, u, u, u};
  T2 I = tmech::eval(tmech::eye<double, 3, 2>());
  T2 P = tmech::eval(I - E2);
  const double dd = (std::log(2.0) - std::log(5.0)) / (2.0 - 5.0);
  auto mm = [&](T2 const &X, T2 const &Y) { return tmech::eval(X * H * Y); };
  auto expected = tmech::eval(0.5 * mm(P, P) + dd * (mm(P, E2) + mm(E2, P)) +
                              0.2 * mm(E2, E2));
  EXPECT_TRUE(tmech::almost_equal(analytic, expected, 1e-10));
}

// ─── Fully coincident: d log(2I)/dA : H = H/2 (closed form) ─────────────
TEST(IsotropicFn, LogTangentFullyCoincident) {
  using namespace isofn_detail;
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  T2 A_val;
  A_val = {2.0, 0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0, 2.0}; // 2·I, eigs {2,2,2}
  T2 H;
  H = {0.1, 0.2, 0.05, 0.2, 0.3, 0.1, 0.05, 0.1, 0.15}; // symmetric

  auto dL = diff(log(A), A);
  ev.set(A, data_from(A_val));
  auto D_data = ev.apply(dL);
  auto analytic = tmech::eval(tmech::dcontract(as_t<3, 4>(*D_data), H));
  // log'(2) = 1/2, isotropic ⇒ tangent : H = (1/2) H for symmetric H.
  EXPECT_TRUE(tmech::almost_equal(analytic, tmech::eval(0.5 * H), 1e-10));
}

// ─── Printing ──────────────────────────────────────────────────────────
TEST(IsotropicFn, Printing) {
  auto A = make_expression<tensor>("A", 3, 2);
  EXPECT_EQ(to_string(log(A)), "log(A)");
  EXPECT_EQ(to_string(exp(A)), "exp(A)");
  EXPECT_EQ(to_string(sqrt(A)), "sqrt(A)");
}

} // namespace numsim::cas

#endif // ISOTROPICTENSORFUNCTIONTEST_H
