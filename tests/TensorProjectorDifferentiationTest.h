#ifndef TENSORPROJECTORDIFFERENTIATIONTEST_H
#define TENSORPROJECTORDIFFERENTIATIONTEST_H

#include <gtest/gtest.h>
#include <memory>

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/diff.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/tensor/projection_tensor.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor/visitors/tensor_evaluator.h>

namespace numsim::cas {

namespace {

template <std::size_t Dim, std::size_t Rank>
auto make_proj_test_data(std::initializer_list<double> values) {
  auto ptr = std::make_shared<tensor_data<double, Dim, Rank>>();
  auto *raw = ptr->raw_data();
  std::size_t i = 0;
  for (auto v : values)
    raw[i++] = v;
  return ptr;
}

template <std::size_t Dim, std::size_t Rank>
auto const &as_tmech_proj(tensor_data_base<double> const &data) {
  return static_cast<tensor_data<double, Dim, Rank> const &>(data).data();
}

constexpr double proj_tol = 1e-10;

} // namespace

// d(dev(X))/dX = P_devi
TEST(TensorProjDiff, DiffDevEqualsProjector) {
  auto X = make_expression<tensor>("X", 3, 2);
  auto d = diff(dev(X), X);

  tensor_evaluator<double> ev;
  // clang-format off
  ev.set(X, make_proj_test_data<3, 2>(
      {1.0, 2.0, 3.0,
       2.0, 5.0, 6.0,
       3.0, 6.0, 9.0}));
  // clang-format on

  auto result_diff = ev.apply(d);
  auto result_proj = ev.apply(P_devi(3));

  ASSERT_NE(result_diff, nullptr);
  ASSERT_NE(result_proj, nullptr);

  EXPECT_TRUE(tmech::almost_equal(as_tmech_proj<3, 4>(*result_diff),
                                  as_tmech_proj<3, 4>(*result_proj),
                                  proj_tol));
}

// d(sym(X))/dX = P_sym
TEST(TensorProjDiff, DiffSymEqualsProjector) {
  auto X = make_expression<tensor>("X", 3, 2);
  auto d = diff(sym(X), X);

  tensor_evaluator<double> ev;
  // clang-format off
  ev.set(X, make_proj_test_data<3, 2>(
      {1.0, 2.0, 3.0,
       2.0, 5.0, 6.0,
       3.0, 6.0, 9.0}));
  // clang-format on

  auto result_diff = ev.apply(d);
  auto result_proj = ev.apply(P_sym(3));

  ASSERT_NE(result_diff, nullptr);
  ASSERT_NE(result_proj, nullptr);
  EXPECT_TRUE(tmech::almost_equal(as_tmech_proj<3, 4>(*result_diff),
                                  as_tmech_proj<3, 4>(*result_proj),
                                  proj_tol));
}

// d(vol(X))/dX = P_vol
TEST(TensorProjDiff, DiffVolEqualsProjector) {
  auto X = make_expression<tensor>("X", 3, 2);
  auto d = diff(vol(X), X);

  tensor_evaluator<double> ev;
  // clang-format off
  ev.set(X, make_proj_test_data<3, 2>(
      {1.0, 2.0, 3.0,
       2.0, 5.0, 6.0,
       3.0, 6.0, 9.0}));
  // clang-format on

  auto result_diff = ev.apply(d);
  auto result_proj = ev.apply(P_vol(3));

  ASSERT_NE(result_diff, nullptr);
  ASSERT_NE(result_proj, nullptr);
  EXPECT_TRUE(tmech::almost_equal(as_tmech_proj<3, 4>(*result_diff),
                                  as_tmech_proj<3, 4>(*result_proj),
                                  proj_tol));
}

// d(skew(X))/dX = P_skew
TEST(TensorProjDiff, DiffSkewEqualsProjector) {
  auto X = make_expression<tensor>("X", 3, 2);
  auto d = diff(skew(X), X);

  tensor_evaluator<double> ev;
  // clang-format off
  ev.set(X, make_proj_test_data<3, 2>(
      {1.0, 2.0, 3.0,
       2.0, 5.0, 6.0,
       3.0, 6.0, 9.0}));
  // clang-format on

  auto result_diff = ev.apply(d);
  auto result_proj = ev.apply(P_skew(3));

  ASSERT_NE(result_diff, nullptr);
  ASSERT_NE(result_proj, nullptr);
  EXPECT_TRUE(tmech::almost_equal(as_tmech_proj<3, 4>(*result_diff),
                                  as_tmech_proj<3, 4>(*result_proj),
                                  proj_tol));
}

// d(dev(sym(X)))/dX = P_devi (construction simplifies dev(sym(X)) to dev(X))
TEST(TensorProjDiff, DiffDevSymEqualsDevProjector) {
  auto X = make_expression<tensor>("X", 3, 2);
  auto dev_sym_X = dev(sym(X));
  auto d = diff(dev_sym_X, X);

  tensor_evaluator<double> ev;
  // clang-format off
  ev.set(X, make_proj_test_data<3, 2>(
      {1.0, 2.0, 3.0,
       2.0, 5.0, 6.0,
       3.0, 6.0, 9.0}));
  // clang-format on

  auto result_diff = ev.apply(d);
  auto result_proj = ev.apply(P_devi(3));

  ASSERT_NE(result_diff, nullptr);
  ASSERT_NE(result_proj, nullptr);
  EXPECT_TRUE(tmech::almost_equal(as_tmech_proj<3, 4>(*result_diff),
                                  as_tmech_proj<3, 4>(*result_proj),
                                  proj_tol));
}

// d(vol(X) + dev(X))/dX = P_sym (construction simplifies vol+dev to sym)
TEST(TensorProjDiff, DiffVolPlusDevEqualsSymProjector) {
  auto X = make_expression<tensor>("X", 3, 2);
  auto vol_plus_dev = vol(X) + dev(X);
  auto d = diff(vol_plus_dev, X);

  tensor_evaluator<double> ev;
  // clang-format off
  ev.set(X, make_proj_test_data<3, 2>(
      {1.0, 2.0, 3.0,
       2.0, 5.0, 6.0,
       3.0, 6.0, 9.0}));
  // clang-format on

  auto result_diff = ev.apply(d);
  auto result_proj = ev.apply(P_sym(3));

  ASSERT_NE(result_diff, nullptr);
  ASSERT_NE(result_proj, nullptr);
  EXPECT_TRUE(tmech::almost_equal(as_tmech_proj<3, 4>(*result_diff),
                                  as_tmech_proj<3, 4>(*result_proj),
                                  proj_tol));
}

} // namespace numsim::cas

#endif // TENSORPROJECTORDIFFERENTIATIONTEST_H
