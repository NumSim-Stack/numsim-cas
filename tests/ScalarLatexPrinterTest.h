#ifndef SCALARLATEXPRINTERTEST_H
#define SCALARLATEXPRINTERTEST_H

#include "cas_test_helpers.h"
#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"

#include <cmath>
#include <string>
#include <tuple>

#define EXPECT_LATEX(expr, expected)                                           \
  EXPECT_EQ(numsim::cas::to_latex((expr)), std::string(expected))

struct ScalarLatexFixture : ::testing::Test {
  using scalar_expr =
      numsim::cas::expression_holder<numsim::cas::scalar_expression>;

  scalar_expr x, y, z;
  scalar_expr a, b, c;
  scalar_expr _1, _2, _3;

  scalar_expr _zero{numsim::cas::get_scalar_zero()};
  scalar_expr _one{numsim::cas::get_scalar_one()};

  ScalarLatexFixture() {
    std::tie(x, y, z) = numsim::cas::make_scalar_variable("x", "y", "z");
    std::tie(a, b, c) = numsim::cas::make_scalar_variable("a", "b", "c");
    std::tie(_1, _2, _3) = numsim::cas::make_scalar_constant(1, 2, 3);
  }
};

using std::abs;
using std::acos;
using std::asin;
using std::atan;
using std::cos;
using std::exp;
using std::log;
using std::pow;
using std::sin;
using std::sqrt;
using std::tan;

// --- Fundamentals ---
TEST_F(ScalarLatexFixture, LATEX_Fundamentals) {
  EXPECT_LATEX(x, "x");
  EXPECT_LATEX(_zero, "0");
  EXPECT_LATEX(_one, "1");
  EXPECT_LATEX(_1, "1");
}

// --- Addition ---
TEST_F(ScalarLatexFixture, LATEX_Addition) {
  EXPECT_LATEX(x + y, "x+y");
  EXPECT_LATEX(x + y + z, "x+y+z");
}

// --- Negation ---
TEST_F(ScalarLatexFixture, LATEX_Negation) {
  EXPECT_LATEX(-x, "-x");
  EXPECT_LATEX(x - y, "x-y");
}

// --- Multiplication ---
TEST_F(ScalarLatexFixture, LATEX_Multiplication) {
  EXPECT_LATEX(x * y, "x \\cdot y");
  EXPECT_LATEX(_2 * x, "2 \\cdot x");
}

// --- Division via fraction ---
TEST_F(ScalarLatexFixture, LATEX_Division) {
  EXPECT_LATEX(x / y, "\\frac{x}{y}");
  // 1/x simplifies to pow(x,-1) = {x}^{-1}
  EXPECT_LATEX(_1 / x, "{x}^{-1}");
  EXPECT_LATEX(x / (y * z), "\\frac{x}{y \\cdot z}");
}

// --- Power ---
TEST_F(ScalarLatexFixture, LATEX_Power) {
  EXPECT_LATEX(pow(x, _2), "{x}^{2}");
  EXPECT_LATEX(pow(x + y, _2), "{\\left(x+y\\right)}^{2}");
}

// --- Trig functions ---
TEST_F(ScalarLatexFixture, LATEX_TrigFunctions) {
  EXPECT_LATEX(sin(x), "\\sin\\left(x\\right)");
  EXPECT_LATEX(cos(x), "\\cos\\left(x\\right)");
  EXPECT_LATEX(tan(x), "\\tan\\left(x\\right)");
  EXPECT_LATEX(asin(x), "\\arcsin\\left(x\\right)");
  EXPECT_LATEX(acos(x), "\\arccos\\left(x\\right)");
  EXPECT_LATEX(atan(x), "\\arctan\\left(x\\right)");
}

// --- Math functions ---
TEST_F(ScalarLatexFixture, LATEX_MathFunctions) {
  EXPECT_LATEX(log(x), "\\ln\\left(x\\right)");
  EXPECT_LATEX(exp(x), "\\exp\\left(x\\right)");
  EXPECT_LATEX(sqrt(x), "\\sqrt{x}");
  EXPECT_LATEX(abs(x), "\\left|x\\right|");
  EXPECT_LATEX(numsim::cas::sign(x), "\\operatorname{sgn}\\left(x\\right)");
}

// --- Rational constants ---
TEST_F(ScalarLatexFixture, LATEX_RationalConstants) {
  auto r = numsim::cas::make_scalar_constant(numsim::cas::rational_t{3, 4});
  EXPECT_LATEX(r, "\\frac{3}{4}");
}

#undef EXPECT_LATEX

#endif // SCALARLATEXPRINTERTEST_H
