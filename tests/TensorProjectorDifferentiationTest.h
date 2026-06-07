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
                                  as_tmech_proj<3, 4>(*result_proj), proj_tol));
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
                                  as_tmech_proj<3, 4>(*result_proj), proj_tol));
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
                                  as_tmech_proj<3, 4>(*result_proj), proj_tol));
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
                                  as_tmech_proj<3, 4>(*result_proj), proj_tol));
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
                                  as_tmech_proj<3, 4>(*result_proj), proj_tol));
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
                                  as_tmech_proj<3, 4>(*result_proj), proj_tol));
}

// d(dev(inv(X)))/dX vs numerical differentiation
TEST(TensorProjDiff, DiffDevOfInvVsNumerical) {
  auto X = make_expression<tensor>("X", 3, 2);
  auto expr = dev(inv(X));
  auto d = diff(expr, X);

  // clang-format off
  tmech::tensor<double, 3, 2> X_t{5.0, 0.3, 0.1,
                                   0.3, 6.0, 0.2,
                                   0.1, 0.2, 7.0};
  // clang-format on
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);

  auto result_sym = ev.apply(d);
  ASSERT_NE(result_sym, nullptr);

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = tmech::eval(tmech::sym(x));
        return as_tmech_proj<3, 2>(*ev.apply(expr));
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(
      tmech::almost_equal(as_tmech_proj<3, 4>(*result_sym), numdiff, 1e-6))
      << "d(dev(inv(X)))/dX: symbolic != numerical";
}

// d(P_dev:{3,4} inv(X):{1,2})/dX — raw inner_product form
TEST(TensorProjDiff, DiffRawProjOfInvVsNumerical) {
  auto X = make_expression<tensor>("X", 3, 2);
  auto expr = inner_product(P_devi(3), sequence{3, 4}, inv(X), sequence{1, 2});
  auto d = diff(expr, X);

  // clang-format off
  tmech::tensor<double, 3, 2> X_t{5.0, 0.3, 0.1,
                                   0.3, 6.0, 0.2,
                                   0.1, 0.2, 7.0};
  // clang-format on
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);

  auto result_sym = ev.apply(d);
  ASSERT_NE(result_sym, nullptr);

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = tmech::eval(tmech::sym(x));
        return as_tmech_proj<3, 2>(*ev.apply(expr));
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(
      tmech::almost_equal(as_tmech_proj<3, 4>(*result_sym), numdiff, 1e-6))
      << "d(P_dev:inv(X))/dX raw: symbolic != numerical";
}

// d(inv(X))/dX alone (baseline check)
TEST(TensorProjDiff, DiffInvAlone) {
  auto X = make_expression<tensor>("X", 3, 2);
  auto expr = inv(X);
  auto d = diff(expr, X);

  // clang-format off
  tmech::tensor<double, 3, 2> X_t{5.0, 0.3, 0.1,
                                   0.3, 6.0, 0.2,
                                   0.1, 0.2, 7.0};
  // clang-format on
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);

  auto result_sym = ev.apply(d);
  ASSERT_NE(result_sym, nullptr);

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = x;
        return as_tmech_proj<3, 2>(*ev.apply(expr));
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(
      tmech::almost_equal(as_tmech_proj<3, 4>(*result_sym), numdiff, 1e-6))
      << "d(inv(X))/dX: symbolic != numerical";
}

// d(P_sym:{3,4} X:{1,2})/dX — simplest projector case with raw inner_product
TEST(TensorProjDiff, DiffRawProjOfXIdentity) {
  auto X = make_expression<tensor>("X", 3, 2);
  auto expr = inner_product(P_sym(3), sequence{3, 4}, X, sequence{1, 2});
  auto d = diff(expr, X);

  // clang-format off
  tmech::tensor<double, 3, 2> X_t{5.0, 0.3, 0.1,
                                   0.3, 6.0, 0.2,
                                   0.1, 0.2, 7.0};
  // clang-format on
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);

  auto result_sym = ev.apply(d);
  ASSERT_NE(result_sym, nullptr);

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = x;
        return as_tmech_proj<3, 2>(*ev.apply(expr));
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(
      tmech::almost_equal(as_tmech_proj<3, 4>(*result_sym), numdiff, 1e-6))
      << "d(P_sym:X)/dX raw: symbolic != numerical";
}

// d(P_sym:{3,4} (X*X):{1,2})/dX — projector of tensor_mul
TEST(TensorProjDiff, DiffProjOfTensorMul) {
  auto X = make_expression<tensor>("X", 3, 2);
  auto expr = inner_product(P_sym(3), sequence{3, 4}, X * X, sequence{1, 2});
  auto d = diff(expr, X);

  // clang-format off
  tmech::tensor<double, 3, 2> X_t{5.0, 0.3, 0.1,
                                   0.3, 6.0, 0.2,
                                   0.1, 0.2, 7.0};
  // clang-format on
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);

  auto result_sym = ev.apply(d);
  ASSERT_NE(result_sym, nullptr);

  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = x;
        return as_tmech_proj<3, 2>(*ev.apply(expr));
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(
      tmech::almost_equal(as_tmech_proj<3, 4>(*result_sym), numdiff, 1e-6))
      << "d(P_sym:(X*X))/dX: symbolic != numerical";
}

// Manual P:dB/dX via separate evaluation, check auto and manual agree
TEST(TensorProjDiff, ManualProjTimesDerivInv) {
  auto X = make_expression<tensor>("X", 3, 2);
  auto inv_expr = inv(X);
  auto dinv = diff(inv_expr, X);

  auto manual_expr =
      inner_product(P_devi(3), sequence{3, 4}, dinv, sequence{1, 2});

  auto auto_expr = dev(inv(X));
  auto auto_diff = diff(auto_expr, X);

  // clang-format off
  tmech::tensor<double, 3, 2> X_t{5.0, 0.3, 0.1,
                                   0.3, 6.0, 0.2,
                                   0.1, 0.2, 7.0};
  // clang-format on
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);

  auto result_manual = ev.apply(manual_expr);
  auto result_auto = ev.apply(auto_diff);

  ASSERT_NE(result_manual, nullptr);
  ASSERT_NE(result_auto, nullptr);

  // Check if manual and auto give same result
  EXPECT_TRUE(tmech::almost_equal(as_tmech_proj<3, 4>(*result_manual),
                                  as_tmech_proj<3, 4>(*result_auto), 1e-10))
      << "manual P:dinv != auto d(dev(inv))/dX";

  // Check manual against numerical
  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = tmech::eval(tmech::sym(x));
        return as_tmech_proj<3, 2>(*ev.apply(auto_expr));
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(
      tmech::almost_equal(as_tmech_proj<3, 4>(*result_manual), numdiff, 1e-6))
      << "manual P:dinv vs numerical";
}

// Raw manual contraction bypassing CAS inner_product evaluator
TEST(TensorProjDiff, RawManualContraction) {
  auto X = make_expression<tensor>("X", 3, 2);
  auto inv_expr = inv(X);
  auto dinv = diff(inv_expr, X);

  // clang-format off
  tmech::tensor<double, 3, 2> X_t{5.0, 0.3, 0.1,
                                   0.3, 6.0, 0.2,
                                   0.1, 0.2, 7.0};
  // clang-format on
  auto X_ptr = std::make_shared<tensor_data<double, 3, 2>>(X_t);

  tensor_evaluator<double> ev;
  ev.set(X, X_ptr);

  auto p_data = ev.apply(P_devi(3));
  auto d_data = ev.apply(dinv);

  ASSERT_NE(p_data, nullptr);
  ASSERT_NE(d_data, nullptr);
  ASSERT_EQ(p_data->rank(), 4u);
  ASSERT_EQ(d_data->rank(), 4u);

  // Use raw_data pointers for manual contraction (0-based flat indexing)
  auto const *P_raw = p_data->raw_data();
  auto const *D_raw = d_data->raw_data();

  tmech::tensor<double, 3, 4> manual_result;
  auto *M_raw = manual_result.raw_data();
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      for (int k = 0; k < 3; ++k)
        for (int l = 0; l < 3; ++l) {
          double sum = 0;
          for (int m = 0; m < 3; ++m)
            for (int n = 0; n < 3; ++n)
              sum += P_raw[i * 27 + j * 9 + m * 3 + n] *
                     D_raw[m * 27 + n * 9 + k * 3 + l];
          M_raw[i * 27 + j * 9 + k * 3 + l] = sum;
        }

  // Compare manual contraction against numerical differentiation
  auto dev_inv_expr = dev(inv(X));
  auto numdiff = tmech::num_diff_central<tmech::sequence<1, 2, 3, 4>>(
      [&](auto const &x) {
        X_ptr->data() = tmech::eval(tmech::sym(x));
        return as_tmech_proj<3, 2>(*ev.apply(dev_inv_expr));
      },
      X_t);
  X_ptr->data() = X_t;

  EXPECT_TRUE(tmech::almost_equal(manual_result, numdiff, 1e-6))
      << "Manual P:dinv contraction vs numerical";

  // Also evaluate via CAS inner_product and check against manual
  auto cas_expr =
      inner_product(P_devi(3), sequence{3, 4}, dinv, sequence{1, 2});
  auto cas_data = ev.apply(cas_expr);
  ASSERT_NE(cas_data, nullptr);

  EXPECT_TRUE(
      tmech::almost_equal(as_tmech_proj<3, 4>(*cas_data), manual_result, 1e-10))
      << "CAS inner_product vs manual contraction";
}

} // namespace numsim::cas

#endif // TENSORPROJECTORDIFFERENTIATIONTEST_H
