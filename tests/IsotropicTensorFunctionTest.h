#ifndef ISOTROPICTENSORFUNCTIONTEST_H
#define ISOTROPICTENSORFUNCTIONTEST_H

#include <cmath>
#include <gtest/gtest.h>
#include <memory>

#include <numsim_cas/core/diff.h>
#include <numsim_cas/eigen_decomposition.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_isotropic_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor/visitors/tensor_evaluator.h>

namespace numsim::cas {

namespace iso_detail {

constexpr double iso_tol = 1e-10;

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

} // namespace iso_detail

// ─── Value: diagonal is exact ──────────────────────────────────────────
TEST(IsotropicFn, DiagonalValues) {
  using namespace iso_detail;
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  T2 A_val;
  A_val = {4.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 9.0};
  ev.set(A, data_from(A_val));

  auto s = ev.apply(sqrt(A));
  EXPECT_TRUE(tmech::almost_equal(
      as_t<3, 2>(*s), T2{2.0, 0, 0, 0, 1.0, 0, 0, 0, 3.0}, iso_tol));

  auto l = ev.apply(log(A));
  EXPECT_TRUE(tmech::almost_equal(
      as_t<3, 2>(*l), T2{std::log(4.0), 0, 0, 0, 0.0, 0, 0, 0, std::log(9.0)},
      iso_tol));

  auto e = ev.apply(exp(A));
  EXPECT_TRUE(tmech::almost_equal(
      as_t<3, 2>(*e),
      T2{std::exp(4.0), 0, 0, 0, std::exp(1.0), 0, 0, 0, std::exp(9.0)},
      iso_tol));
}

// ─── Value: round-trips on a non-diagonal SPD tensor ───────────────────
TEST(IsotropicFn, RoundTripsNonDiagonal) {
  using namespace iso_detail;
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  T2 A_val; // SPD, eigenvalues {2, 2.382, 4.618}
  A_val = {4.0, 1.0, 0.0, 1.0, 3.0, 0.0, 0.0, 0.0, 2.0};
  ev.set(A, data_from(A_val));

  // exp(log(A)) == A
  auto explog = ev.apply(exp(log(A)));
  EXPECT_TRUE(tmech::almost_equal(as_t<3, 2>(*explog), A_val, 1e-9));

  // sqrt(A) * sqrt(A) == A  (matrix product of the isotropic square root)
  auto S = sqrt(A);
  auto sq = ev.apply(inner_product(S, sequence{2}, S, sequence{1}));
  EXPECT_TRUE(tmech::almost_equal(as_t<3, 2>(*sq), A_val, 1e-9));
}

// ─── Tangent for free: d log(A)/dA matches finite differences ──────────
TEST(IsotropicFn, LogTangentMatchesFiniteDiff) {
  using namespace iso_detail;
  tensor_evaluator<double> ev;
  auto A = make_expression<tensor>("A", 3, 2);
  T2 A_val;
  A_val = {4.0, 1.0, 0.0, 1.0, 3.0, 0.0, 0.0, 0.0, 2.0}; // SPD
  T2 H;
  H = {0.1, 0.2, 0.0, 0.2, 0.3, 0.1, 0.0, 0.1, 0.15}; // symmetric direction

  auto dL = diff(log(A), A); // rank-4, produced purely by the generic
                             // chain rule — no isotropic-function diff code
  ASSERT_TRUE(dL.is_valid());
  EXPECT_EQ(dL.get().rank(), std::size_t{4});

  ev.set(A, data_from(A_val));
  auto D_data = ev.apply(dL);
  auto const &D = as_t<3, 4>(*D_data);
  auto analytic = tmech::eval(tmech::dcontract(D, H));

  const double t = 1e-6;
  ev.set(A, data_from(T2{tmech::eval(A_val + t * H)}));
  auto Lp_data = ev.apply(log(A));
  auto const &Lp = as_t<3, 2>(*Lp_data);
  ev.set(A, data_from(T2{tmech::eval(A_val - t * H)}));
  auto Lm_data = ev.apply(log(A));
  auto const &Lm = as_t<3, 2>(*Lm_data);
  auto fd = tmech::eval((Lp - Lm) / (2.0 * t));

  EXPECT_TRUE(tmech::almost_equal(analytic, fd, 1e-6))
      << "d log(A)/dA : H disagrees with finite difference";
}

} // namespace numsim::cas

#endif // ISOTROPICTENSORFUNCTIONTEST_H
