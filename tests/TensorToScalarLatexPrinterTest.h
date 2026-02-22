#ifndef TENSORTOSCALARLATEXPRINTERTEST_H
#define TENSORTOSCALARLATEXPRINTERTEST_H

#include "cas_test_helpers.h"
#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"

#include <cstddef>
#include <string>
#include <tuple>
#include <type_traits>

namespace {
using T2SLatexTestDims =
    ::testing::Types<std::integral_constant<std::size_t, 2>,
                     std::integral_constant<std::size_t, 3>>;
} // namespace

template <typename DimTag>
class TensorToScalarLatexPrinterTest : public ::testing::Test {
protected:
  static constexpr std::size_t Dim = DimTag::value;

  using tensor_t =
      numsim::cas::expression_holder<numsim::cas::tensor_expression>;
  using scalar_t =
      numsim::cas::expression_holder<numsim::cas::scalar_expression>;

  TensorToScalarLatexPrinterTest() {
    std::tie(X, Y, Z) = numsim::cas::make_tensor_variable(
        std::tuple{"X", Dim, 2}, std::tuple{"Y", Dim, 2},
        std::tuple{"Z", Dim, 2});

    std::tie(A, B, C) = numsim::cas::make_tensor_variable(
        std::tuple{"A", Dim, 4}, std::tuple{"B", Dim, 4},
        std::tuple{"C", Dim, 4});

    std::tie(x, y, z) = numsim::cas::make_scalar_variable("x", "y", "z");
    std::tie(a, b, c) = numsim::cas::make_scalar_variable("a", "b", "c");

    std::tie(_1, _2, _3) = numsim::cas::make_scalar_constant(1, 2, 3);

    _zero = scalar_t{numsim::cas::get_scalar_zero()};
    _one = scalar_t{numsim::cas::get_scalar_one()};

    _Zero = tensor_t{
        numsim::cas::make_expression<numsim::cas::tensor_zero>(Dim, 2)};
    _One = tensor_t{
        numsim::cas::make_expression<numsim::cas::kronecker_delta>(Dim)};
  }

  tensor_t X, Y, Z;
  tensor_t A, B, C;

  scalar_t x, y, z;
  scalar_t a, b, c;

  scalar_t _1, _2, _3;
  scalar_t _zero, _one;

  tensor_t _Zero, _One;
};

TYPED_TEST_SUITE(TensorToScalarLatexPrinterTest, T2SLatexTestDims);

#define EXPECT_T2S_LATEX(expr, expected)                                       \
  EXPECT_EQ(numsim::cas::to_latex((expr)), std::string(expected))

// --- Trace ---
TYPED_TEST(TensorToScalarLatexPrinterTest, LATEX_Trace) {
  auto &X = this->X;
  EXPECT_T2S_LATEX(numsim::cas::trace(X),
                   "\\operatorname{tr}\\left(\\boldsymbol{X}\\right)");
}

// --- Determinant ---
TYPED_TEST(TensorToScalarLatexPrinterTest, LATEX_Det) {
  auto &X = this->X;
  EXPECT_T2S_LATEX(numsim::cas::det(X), "\\det\\left(\\boldsymbol{X}\\right)");
}

// --- Norm ---
TYPED_TEST(TensorToScalarLatexPrinterTest, LATEX_Norm) {
  auto &X = this->X;
  EXPECT_T2S_LATEX(numsim::cas::norm(X), "\\left\\|\\boldsymbol{X}\\right\\|");
}

// --- Double contraction to scalar ---
TYPED_TEST(TensorToScalarLatexPrinterTest, LATEX_DoubleContraction) {
  auto &X = this->X;
  auto &Y = this->Y;
  EXPECT_T2S_LATEX(numsim::cas::dot_product(X, numsim::cas::sequence{1, 2}, Y,
                                            numsim::cas::sequence{1, 2}),
                   "\\boldsymbol{X} : \\boldsymbol{Y}");
}

// --- Sqrt ---
TYPED_TEST(TensorToScalarLatexPrinterTest, LATEX_Sqrt) {
  auto &X = this->X;
  auto tr_x = numsim::cas::trace(X);
  EXPECT_T2S_LATEX(numsim::cas::sqrt(tr_x),
                   "\\sqrt{\\operatorname{tr}\\left(\\boldsymbol{X}\\right)}");
}

// --- Constants ---
TYPED_TEST(TensorToScalarLatexPrinterTest, LATEX_Constants) {
  auto t2s_zero =
      numsim::cas::expression_holder<numsim::cas::tensor_to_scalar_expression>{
          numsim::cas::make_expression<numsim::cas::tensor_to_scalar_zero>()};
  auto t2s_one =
      numsim::cas::expression_holder<numsim::cas::tensor_to_scalar_expression>{
          numsim::cas::make_expression<numsim::cas::tensor_to_scalar_one>()};
  EXPECT_T2S_LATEX(t2s_zero, "0");
  EXPECT_T2S_LATEX(t2s_one, "1");
}

#undef EXPECT_T2S_LATEX

#endif // TENSORTOSCALARLATEXPRINTERTEST_H
