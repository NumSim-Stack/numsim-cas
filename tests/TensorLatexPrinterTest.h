#ifndef TENSORLATEXPRINTERTEST_H
#define TENSORLATEXPRINTERTEST_H

#include "cas_test_helpers.h"
#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"

#include <cmath>
#include <cstddef>
#include <string>
#include <tuple>
#include <type_traits>

namespace {
using TensorLatexTestDims =
    ::testing::Types<std::integral_constant<std::size_t, 2>,
                     std::integral_constant<std::size_t, 3>>;
} // namespace

template <typename DimTag>
class TensorLatexPrinterTest : public ::testing::Test {
protected:
  static constexpr std::size_t Dim = DimTag::value;

  using tensor_t =
      numsim::cas::expression_holder<numsim::cas::tensor_expression>;
  using scalar_t =
      numsim::cas::expression_holder<numsim::cas::scalar_expression>;

  TensorLatexPrinterTest() {
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
  scalar_t _zero;
  scalar_t _one;

  tensor_t _Zero;
  tensor_t _One;
};

TYPED_TEST_SUITE(TensorLatexPrinterTest, TensorLatexTestDims);

#define EXPECT_TENSOR_LATEX(expr, expected)                                    \
  EXPECT_EQ(numsim::cas::to_latex((expr)), std::string(expected))

#define EXPECT_TENSOR_LATEX_CFG(expr, cfg, expected)                           \
  EXPECT_EQ(numsim::cas::to_latex((expr), (cfg)), std::string(expected))

// --- Basic tensor names ---
TYPED_TEST(TensorLatexPrinterTest, LATEX_TensorNames) {
  EXPECT_TENSOR_LATEX(this->X, "\\boldsymbol{X}");
  EXPECT_TENSOR_LATEX(this->A, "\\mathbb{A}");
}

// --- Kronecker delta ---
TYPED_TEST(TensorLatexPrinterTest, LATEX_KroneckerDelta) {
  EXPECT_TENSOR_LATEX(this->_One, "\\boldsymbol{I}");
}

// --- Zero tensor ---
TYPED_TEST(TensorLatexPrinterTest, LATEX_ZeroTensor) {
  EXPECT_TENSOR_LATEX(this->_Zero, "0");
}

// --- Addition ---
TYPED_TEST(TensorLatexPrinterTest, LATEX_Addition) {
  auto &X = this->X;
  auto &Y = this->Y;
  EXPECT_TENSOR_LATEX(X + Y, "\\boldsymbol{X}+\\boldsymbol{Y}");
}

// --- Negation ---
TYPED_TEST(TensorLatexPrinterTest, LATEX_Negation) {
  auto &X = this->X;
  EXPECT_TENSOR_LATEX(-X, "-\\boldsymbol{X}");
}

// --- Scalar multiplication ---
TYPED_TEST(TensorLatexPrinterTest, LATEX_ScalarMul) {
  auto &X = this->X;
  auto &x = this->x;
  EXPECT_TENSOR_LATEX(x * X, "x \\cdot \\boldsymbol{X}");
}

// --- Scalar division ---
TYPED_TEST(TensorLatexPrinterTest, LATEX_ScalarDiv) {
  auto &X = this->X;
  auto &x = this->x;
  EXPECT_TENSOR_LATEX(X / x, "\\frac{\\boldsymbol{X}}{x}");
}

// --- Inverse ---
TYPED_TEST(TensorLatexPrinterTest, LATEX_Inverse) {
  auto &X = this->X;
  EXPECT_TENSOR_LATEX(numsim::cas::inv(X), "{\\boldsymbol{X}}^{-1}");
}

// --- Transpose ---
TYPED_TEST(TensorLatexPrinterTest, LATEX_Transpose) {
  auto &X = this->X;
  EXPECT_TENSOR_LATEX(numsim::cas::trans(X), "{\\boldsymbol{X}}^{\\mathrm{T}}");
}

// --- Double contraction ---
TYPED_TEST(TensorLatexPrinterTest, LATEX_DoubleContraction) {
  auto &X = this->X;
  auto &Y = this->Y;
  EXPECT_TENSOR_LATEX(numsim::cas::inner_product(X, numsim::cas::sequence{1, 2},
                                                 Y,
                                                 numsim::cas::sequence{1, 2}),
                      "\\boldsymbol{X} : \\boldsymbol{Y}");
}

// --- Projector functions ---
TYPED_TEST(TensorLatexPrinterTest, LATEX_ProjectorFunctions) {
  auto &X = this->X;
  EXPECT_TENSOR_LATEX(numsim::cas::dev(X),
                      "\\operatorname{dev}\\left(\\boldsymbol{X}\\right)");
  EXPECT_TENSOR_LATEX(numsim::cas::sym(X),
                      "\\operatorname{sym}\\left(\\boldsymbol{X}\\right)");
  EXPECT_TENSOR_LATEX(numsim::cas::vol(X),
                      "\\operatorname{vol}\\left(\\boldsymbol{X}\\right)");
  EXPECT_TENSOR_LATEX(numsim::cas::skew(X),
                      "\\operatorname{skew}\\left(\\boldsymbol{X}\\right)");
}

// --- Custom config ---
TYPED_TEST(TensorLatexPrinterTest, LATEX_CustomConfig) {
  auto &X = this->X;
  numsim::cas::latex_config cfg = numsim::cas::latex_config::default_config();
  cfg.tensor_fonts[2] = "\\mathbf";
  EXPECT_TENSOR_LATEX_CFG(X, cfg, "\\mathbf{X}");
}

#undef EXPECT_TENSOR_LATEX
#undef EXPECT_TENSOR_LATEX_CFG

#endif // TENSORLATEXPRINTERTEST_H
