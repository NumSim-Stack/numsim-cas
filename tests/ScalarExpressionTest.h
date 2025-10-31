#ifndef SCALAREXPRESSIONTEST_H
#define SCALAREXPRESSIONTEST_H

#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"

#include <tuple>
#include <cmath>

// Assertions
#define EXPECT_PRINT(expr, expected) \
EXPECT_EQ(std::to_string((expr)), std::string(expected))

#define EXPECT_SAME_PRINT(lhs, rhs) \
EXPECT_EQ(std::to_string((lhs)), ::S((rhs)))

// ---------- Scalar fixture ----------
template <typename T>
struct ScalarFixture : ::testing::Test {
  using value_type = T;
  using scalar_expr = numsim::cas::expression_holder<numsim::cas::scalar_expression<T>>;

         // Variables & constants
  scalar_expr x, y, z;
  scalar_expr a, b, c, d;
  scalar_expr _1, _2, _3;
  scalar_expr _zero{numsim::cas::get_scalar_zero<T>()};
  scalar_expr _one {numsim::cas::get_scalar_one<T>()};

  ScalarFixture() {
    std::tie(x, y, z)   = numsim::cas::make_scalar_variable<T>("x","y","z");
    std::tie(a, b, c, d)= numsim::cas::make_scalar_variable<T>("a","b","c","d");
    std::tie(_1, _2, _3)= numsim::cas::make_scalar_constant<T>(1,2,3);
  }
};


// Bring numeric overloads; ADL will pick CAS overloads for expressions
using std::pow; using std::sin; using std::cos; using std::tan; using std::exp; using std::log; using std::sqrt;

using ScalarTypes = ::testing::Types<double, float, int /*, std::complex<double>*/>;
TYPED_TEST_SUITE(ScalarFixture, ScalarTypes);

/*
Nomenclature & conventions
- PRINT_*: checks printed canonical forms (ordering, collection, formatting)
- ADD_*, MUL_*: addition/multiplication-specific merges
- DIFF_*: differentiation rules and simplifications
- We compare strings via EXPECT_PRINT(expr, "literal") unless equality up to printing is desired → EXPECT_SAME_PRINT(lhs, rhs).
*/

//
// PRINT_FUNDAMENTALS — zeros, ones, identity/annihilator laws
//
TYPED_TEST(ScalarFixture, PRINT_Fundamentals) {
  auto &x=this->x,&y=this->y,&z=this->z;
  auto &_1=this->_1,&_2=this->_2,&_3=this->_3;
  auto &zero=this->_zero,&one=this->_one;

  EXPECT_PRINT(_1, "1");
  EXPECT_PRINT(_1 + _2 + _3, "6");

         // additive identity
  EXPECT_PRINT(zero + x, "x");
  EXPECT_PRINT(x + zero, "x");
  EXPECT_PRINT(zero + (x + y + z), "x+y+z");
  EXPECT_PRINT((x + y + z) + zero, "x+y+z");

         // multiplicative annihilator
  EXPECT_PRINT(zero * one, "0");
  EXPECT_PRINT(one * zero, "0");
  EXPECT_PRINT(zero * x, "0");
  EXPECT_PRINT(x * zero, "0");
  EXPECT_PRINT(zero * (x + y + z), "0");
  EXPECT_PRINT((x + y + z) * zero, "0");

         // multiplicative identity
  EXPECT_PRINT(one * x, "x");
  EXPECT_PRINT(x * one, "x");
  EXPECT_PRINT(one * (x + y + z), "x+y+z");
  EXPECT_PRINT((x + y + z) * one, "x+y+z");
}

//
// PRINT_ADDITION — constant folding, term collection, canonical ordering
//
TYPED_TEST(ScalarFixture, PRINT_Addition) {
  auto &x=this->x,&y=this->y,&z=this->z;
  auto &_1=this->_1,&_2=this->_2,&_3=this->_3;

  EXPECT_PRINT(_2 + x, "2+x");
  EXPECT_PRINT(x + _2, "2+x");
  EXPECT_PRINT(_1 + x + _3, "4+x");
  EXPECT_PRINT(_1 + x + _3 + y + _2, "6+x+y");

  EXPECT_PRINT(x + x, "2*x");
  EXPECT_PRINT(x + x + _1, "1+2*x");
  EXPECT_PRINT(x + x + _1 + y, "1+2*x+y");
  EXPECT_PRINT((x + y + z) + (x + y + z), "2*x+2*y+2*z");
}

//
// PRINT_MULTIPLICATION_AND_POWERS — product rules and power bumping
//
TYPED_TEST(ScalarFixture, PRINT_MultiplicationAndPowers) {
  auto &x=this->x,&y=this->y;
  auto &_1=this->_1,&_2=this->_2,&_3=this->_3;

  EXPECT_PRINT(_2 * x + x, "3*x");
  EXPECT_PRINT(x + _2 * x, "3*x");
  EXPECT_PRINT(y * x + x, "x+x*y");
  EXPECT_PRINT(y * x + x * y, "2*x*y");
  EXPECT_PRINT(_2 * y * x + _3 * x * y, "5*x*y");
  EXPECT_PRINT(_2 * y * x + _3 * x * y + _1 * x * y, "6*x*y");

  EXPECT_PRINT(x * x, "pow(x,2)");
  EXPECT_PRINT(x * x * x, "pow(x,3)");
  EXPECT_PRINT(pow(x, _2) * x, "pow(x,3)");
  EXPECT_PRINT(x * pow(x, _2), "pow(x,3)");
  EXPECT_PRINT(x * (x * x), "pow(x,3)");
  EXPECT_PRINT((x * x) * x, "pow(x,3)");
  EXPECT_PRINT(pow(x, _2) * pow(x, _2), "pow(x,4)");
  EXPECT_PRINT(pow(x, x) * x, "pow(x,1+x)");
  EXPECT_PRINT(pow(x, _1 + x) * x, "pow(x,2+x)");
}

//
// PRINT_DIVISION_FORMAT — formatting and flattening (no cross-cancellation yet)
//
TYPED_TEST(ScalarFixture, PRINT_DivisionFormat) {
  using value_type = typename TestFixture::value_type;
  auto &x=this->x,&y=this->y,&a=this->a,&b=this->b,&c=this->c,&d=this->d;
  auto &_1=this->_1,&_2=this->_2;

  if constexpr (std::is_floating_point_v<value_type>)
    EXPECT_PRINT(_1 / _2, "0.5");
  else
    EXPECT_PRINT(_1 / _2, "1/2");

  EXPECT_PRINT(x / y, "x/y");
  EXPECT_PRINT(a / b / c / d, "a/(b*c*d)");
  EXPECT_PRINT((a / b) / (c / d), "a*d/(b*c)");
  EXPECT_PRINT((a + b) / (c + d), "(a+b)/(c+d)");
}

//
// PRINT_NEGATION_AND_PARENS — canonical parentheses and sign handling
//
TYPED_TEST(ScalarFixture, PRINT_NegationAndParens) {
  auto &x=this->x,&y=this->y,&z=this->z;

  EXPECT_PRINT(-x, "-x");
  EXPECT_PRINT(x - x, "0");
  EXPECT_PRINT(-x - x, "-2*x");
  EXPECT_PRINT(x - x - y - z, "-(y+z)");
  EXPECT_PRINT(-x - x - y - z, "-(2*x+y+z)");
  EXPECT_PRINT(-x - y - z - y, "-(x+2*y+z)");
  EXPECT_PRINT(-x - y - z - z, "-(x+y+2*z)");
  EXPECT_PRINT(-(x + y), "-(x+y)");
  EXPECT_PRINT(-(-x), "x");
  EXPECT_PRINT(x + (-x), "0");
}

//
// PRINT_MIXED — assorted collection & power bump checks
//
TYPED_TEST(ScalarFixture, PRINT_Mixed) {
  auto &x=this->x,&y=this->y,&z=this->z;
  auto &_1=this->_1,&_2=this->_2,&_3=this->_3;

  EXPECT_PRINT(x * _2, "2*x");
  EXPECT_PRINT(x * _2 + x, "3*x");
  EXPECT_PRINT((x * _2 + x) + x + _2, "2+4*x");
  EXPECT_PRINT(_2 * (x + y + z), "2*(x+y+z)");
  EXPECT_PRINT(_2 * _3, "6");
  EXPECT_PRINT(_2 * _3 * x, "6*x");
  EXPECT_PRINT(x * _2 * _3, "6*x");
  EXPECT_PRINT(_2 * x * _3, "6*x");
  EXPECT_PRINT((x + y + z) + (x + y + z), "2*x+2*y+2*z");
}

//
// PRINT_FUNCTIONS — trig/exp/log prints (no derivatives here)
//
TYPED_TEST(ScalarFixture, PRINT_Functions) {
  auto &x=this->x;
  EXPECT_PRINT(cos(x), "cos(x)");
  EXPECT_PRINT(sin(x), "sin(x)");
  EXPECT_PRINT(tan(x), "tan(x)");
  EXPECT_PRINT(exp(x), "exp(x)");
}

//
// DIFF_FUNDAMENTALS — constants, linear and power rules
//
TYPED_TEST(ScalarFixture, DIFF_Fundamentals) {
  auto &x=this->x,&y=this->y,&z=this->z;
  auto &_1=this->_1; (void)_1;
  auto &zero=this->_zero,&one=this->_one;

  EXPECT_PRINT(numsim::cas::diff(zero, x), "0");
  EXPECT_PRINT(numsim::cas::diff(one , x), "0");
  EXPECT_PRINT(numsim::cas::diff(this->_1, x), "0");
  EXPECT_PRINT(numsim::cas::diff(x, x), "1");
  EXPECT_PRINT(numsim::cas::diff(x * y * z, x), "y*z");
  EXPECT_PRINT(numsim::cas::diff(x + x, x), "2");
  EXPECT_PRINT(numsim::cas::diff(x * x, x), "2*x");
  EXPECT_PRINT(numsim::cas::diff(x * x * x, x), "3*pow(x,2)");
  EXPECT_PRINT(numsim::cas::diff(x * x * x * x, x), "4*pow(x,3)");
}

//
// DIFF_LINEARITY_AND_BASICS — linearity, const factors, simple quotient
//
TYPED_TEST(ScalarFixture, DIFF_LinearityAndBasics) {
  auto &x=this->x,&y=this->y,&a=this->a,&b=this->b,&c=this->c;

  EXPECT_PRINT(numsim::cas::diff(this->_2 * x + this->_3, x), "2");
  EXPECT_PRINT(numsim::cas::diff(a * x + b * y + c, x), "a"); // a,b,c independent of x
  EXPECT_PRINT(numsim::cas::diff(x * y, x), "y");             // treat y as const
  EXPECT_PRINT(numsim::cas::diff((x + y) * x, x), "2*x+y");   // product rule + simplification
  EXPECT_PRINT(numsim::cas::diff(x / y, x), "pow(y,-1)");           // constant denominator
}

//
// DIFF_CHAIN_RULE — simple chain rule on power of a sum
//
TYPED_TEST(ScalarFixture, DIFF_ChainRule) {
  auto &x=this->x;
  EXPECT_PRINT(numsim::cas::diff(pow(x + this->_1, this->_3), x), "3*pow(1+x,2)");
}

//
// PRINT_CANONICAL_ORDERING — ordering for sums/products; constant first in sums
//
TYPED_TEST(ScalarFixture, PRINT_CanonicalOrdering) {
  auto &x=this->x,&y=this->y,&z=this->z;
  EXPECT_PRINT(y * x, "x*y");
  EXPECT_PRINT(y + x + z + y + x, "2*x+2*y+z");
  EXPECT_PRINT(this->_2 + x, "2+x");
  EXPECT_PRINT(x + this->_2, "2+x");
}

//
// PRINT_DIVISION_NO_CANCEL — current behavior: no cross-cancellation across '/'
// (keep these expectations until you implement cancellation; pow/x does reduce.)
//
TYPED_TEST(ScalarFixture, PRINT_Division_NoCancel) {
  auto &x=this->x,&y=this->y,&a=this->a,&b=this->b;
  EXPECT_PRINT(x / x, "1");                             // trivial
  EXPECT_PRINT(y * x / x, "x*y/x");                     // no cross-cancel yet
  EXPECT_PRINT(pow(x, this->_3) / x, "pow(x,2)");       // power decrement is ok
  EXPECT_PRINT((a * b) / a, "a*b/a");                   // no cross-cancel yet
  EXPECT_PRINT((x / y) * y, "y*x/y");                   // no cross-cancel yet
}

//
// ADD_* — targeted add/combine cases (merges and constants)
//
TYPED_TEST(ScalarFixture, ADD_CombineSameSymbol) {
  auto &x=this->x;
  EXPECT_PRINT(x + x, "2*x");
}

TYPED_TEST(ScalarFixture, ADD_CombineMulAndSymbol_EitherSide) {
  auto &x=this->x; auto &_2=this->_2;
  EXPECT_PRINT(_2 * x + x, "3*x");
  EXPECT_PRINT(x + _2 * x, "3*x");
}

TYPED_TEST(ScalarFixture, ADD_CombineTwoMultiplies) {
  auto &x=this->x; auto &_2=this->_2, &_3=this->_3;
  EXPECT_PRINT(_2 * x + _3 * x, "5*x");
}

TYPED_TEST(ScalarFixture, ADD_ZeroRules) {
  auto &x=this->x,&y=this->y; auto &zero=this->_zero;
  EXPECT_PRINT(x + zero, "x");
  EXPECT_PRINT(zero + (x + y), "x+y");
}

TYPED_TEST(ScalarFixture, ADD_NegationAndInverse) {
  auto &x=this->x,&y=this->y;
  EXPECT_PRINT(x + (-x), "0");
  EXPECT_PRINT((-x) + (-x), "-2*x");
  EXPECT_PRINT((x + y) + (-x), "y");
}

TYPED_TEST(ScalarFixture, ADD_ConstantFoldingIntoNAry_LeftAndRight) {
  auto &x=this->x,&y=this->y; auto &_1=this->_1,&_2=this->_2;
  EXPECT_PRINT((x + y) + _2, "2+x+y");   // exercises lvalue copy-branch
  EXPECT_PRINT(_2 + (x + y), "2+x+y");   // symmetric path
  EXPECT_PRINT((x + _1) + _2, "3+x");
  EXPECT_PRINT(_2 + (x + _1), "3+x");
}

TYPED_TEST(ScalarFixture, ADD_MergeAcrossNestedAdd) {
  auto &x=this->x,&y=this->y,&z=this->z;
  EXPECT_PRINT((x + y) + x, "2*x+y");
  EXPECT_PRINT((x + y + z) + (y + x), "2*x+2*y+z");
}

TYPED_TEST(ScalarFixture, ADD_CanonicalOrderingAndCoeffFirst) {
  auto &x=this->x,&y=this->y,&z=this->z; auto &_2=this->_2,&_3=this->_3;
  EXPECT_PRINT(y + x + z + y + x, "2*x+2*y+z");
  EXPECT_PRINT(x + _2, "2+x");          // constant first
  EXPECT_PRINT(_3 + x + y + _2, "5+x+y");
}



// template <typename T> class ScalarExpressionTest : public ::testing::Test {
// protected:
//   using value_type = T;
//   using expr_type =
//       numsim::cas::expression_holder<numsim::cas::scalar_expression<T>>;

//   ScalarExpressionTest() {
//     std::tie(x, y, z) = numsim::cas::make_scalar_variable<T>("x", "y", "z");
//     std::tie(a, b, c, d) =
//         numsim::cas::make_scalar_variable<T>("a", "b", "c", "d");
//     std::tie(_1, _2, _3) = numsim::cas::make_scalar_constant<T>(1, 2, 3);
//   }

//   expr_type x, y, z;
//   expr_type a, b, c, d;
//   expr_type _1, _2, _3;
//   expr_type _zero{numsim::cas::get_scalar_zero<T>()};
//   expr_type _one{numsim::cas::get_scalar_one<T>()};
// };

// using TestTypes =
//     ::testing::Types<double, float, int /*, std::complex<double>*/>;
// TYPED_TEST_SUITE(ScalarExpressionTest, TestTypes);

// TYPED_TEST(ScalarExpressionTest, ScalarFundamentalsPrint) {
//   auto &x = this->x;
//   auto &y = this->y;
//   auto &z = this->z;
//   auto &_1 = this->_1;
//   auto &_2 = this->_2;
//   auto &_3 = this->_3;
//   auto &zero{this->_zero};
//   auto &one{this->_one};
//   EXPECT_EQ(std::to_string(_1), "1");
//   EXPECT_EQ(std::to_string(_1 + _2 + _3), "6");
//   EXPECT_EQ(std::to_string(zero + x), "x");
//   EXPECT_EQ(std::to_string(x + zero), "x");
//   EXPECT_EQ(std::to_string(zero + (x + y + z)), "x+y+z");
//   EXPECT_EQ(std::to_string((x + y + z) + zero), "x+y+z");

//   EXPECT_EQ(std::to_string(zero * one), "0");
//   EXPECT_EQ(std::to_string(one * zero), "0");
//   EXPECT_EQ(std::to_string(zero * x), "0");
//   EXPECT_EQ(std::to_string(x * zero), "0");
//   EXPECT_EQ(std::to_string(zero * (x + y + z)), "0");
//   EXPECT_EQ(std::to_string((x + y + z) * zero), "0");

//   EXPECT_EQ(std::to_string(one * x), "x");
//   EXPECT_EQ(std::to_string(x * one), "x");
//   EXPECT_EQ(std::to_string(one * (x + y + z)), "x+y+z");
//   EXPECT_EQ(std::to_string((x + y + z) * one), "x+y+z");
// }

// TYPED_TEST(ScalarExpressionTest, ScalarAdditionPrint) {
//   auto &x = this->x;
//   auto &y = this->y;
//   auto &z = this->z;
//   auto &_1 = this->_1;
//   auto &_2 = this->_2;
//   auto &_3 = this->_3;
//   EXPECT_EQ(std::to_string(_2 + x), "2+x");
//   EXPECT_EQ(std::to_string(x + _2), "2+x");
//   EXPECT_EQ(std::to_string(_1 + x + _3), "4+x");
//   EXPECT_EQ(std::to_string(_1 + x + _3 + y + _2), "6+x+y");
//   EXPECT_EQ(std::to_string(x + x), "2*x");
//   EXPECT_EQ(std::to_string(x + x + _1), "1+2*x");
//   EXPECT_EQ(std::to_string(x + x + _1 + y), "1+2*x+y");
//   EXPECT_EQ(std::to_string((x + y + z) + (x + y + z)), "2*x+2*y+2*z");
// }

// TYPED_TEST(ScalarExpressionTest, ScalarMultiplicationPrint) {
//   auto &x = this->x;
//   auto &y = this->y;
//   auto &_1 = this->_1;
//   auto &_2 = this->_2;
//   auto &_3 = this->_3;
//   EXPECT_EQ(std::to_string(_2 * x + x), "3*x");
//   EXPECT_EQ(std::to_string(x + _2 * x), "3*x");
//   EXPECT_EQ(std::to_string(y * x + x), "x+x*y");
//   EXPECT_EQ(std::to_string(y * x + x * y), "2*x*y");
//   EXPECT_EQ(std::to_string(_2 * y * x + _3 * x * y), "5*x*y");
//   EXPECT_EQ(std::to_string(_2 * y * x + _3 * x * y + _1 * x * y), "6*x*y");
// }

// TYPED_TEST(ScalarExpressionTest, ScalarDivPrint) {
//   using value_type = typename TestFixture::value_type;
//   auto &x = this->x;
//   auto &y = this->y;
//   auto &a = this->a;
//   auto &b = this->b;
//   auto &c = this->c;
//   auto &d = this->d;
//   auto &_1 = this->_1;
//   auto &_2 = this->_2; // auto& _3 = this->_3;
//   if constexpr (std::is_floating_point_v<value_type>) {
//     EXPECT_EQ(std::to_string(_1 / _2), "0.5");
//   } else {
//     EXPECT_EQ(std::to_string(_1 / _2), "1/2");
//   }
//   EXPECT_EQ(std::to_string(x / y), "x/y");
//   EXPECT_EQ(std::to_string(a / b / c / d), "a/(b*c*d)");
//   EXPECT_EQ(std::to_string((a / b) / (c / d)), "a*d/(b*c)");
//   EXPECT_EQ(std::to_string((a + b) / (c + d)), "(a+b)/(c+d)");
// }

// TYPED_TEST(ScalarExpressionTest, ScalarSubtractionAndNegationPrint) {
//   auto &x = this->x;
//   auto &y = this->y;
//   auto &z = this->z;
//   EXPECT_EQ(std::to_string(-x), "-x");
//   EXPECT_EQ(std::to_string(x - x), "0");
//   EXPECT_EQ(std::to_string(-x - x), "-2*x");
//   EXPECT_EQ(std::to_string(x - x - y - z), "-(y+z)");
//   EXPECT_EQ(std::to_string(-x - x - y - z), "-(2*x+y+z)");
//   EXPECT_EQ(std::to_string(-x - y - z - y), "-(x+2*y+z)");
//   EXPECT_EQ(std::to_string(-x - y - z - z), "-(x+y+2*z)");
// }

// TYPED_TEST(ScalarExpressionTest, ScalarMixedOperationsPrint) {
//   auto &x = this->x;
//   auto &y = this->y;
//   auto &z = this->z;
//   auto &_1 = this->_1;
//   auto &_2 = this->_2;
//   auto &_3 = this->_3;
//   EXPECT_EQ(std::to_string(x * _2), "2*x");
//   EXPECT_EQ(std::to_string(x * _2 + x), "3*x");
//   EXPECT_EQ(std::to_string((x * _2 + x) + x + _2), "2+4*x");
//   EXPECT_EQ(std::to_string(_2 * (x + y + z)), "2*(x+y+z)");
//   EXPECT_EQ(std::to_string(_2 * _3), "6");
//   EXPECT_EQ(std::to_string(_2 * _3 * x), "6*x");
//   EXPECT_EQ(std::to_string(x * _2 * _3), "6*x");
//   EXPECT_EQ(std::to_string(_2 * x * _3), "6*x");
//   EXPECT_EQ(std::to_string(x * x), "pow(x,2)");
//   EXPECT_EQ(std::to_string(x * x * x), "pow(x,3)");
//   EXPECT_EQ(std::to_string(x * x * x * x), "pow(x,4)");
//   EXPECT_EQ(std::to_string((x + y + z) + (x + y + z)), "2*x+2*y+2*z");
//   EXPECT_EQ(std::to_string(std::pow(x, _2) * x), "pow(x,3)");
//   EXPECT_EQ(std::to_string(x * std::pow(x, _2)), "pow(x,3)");
//   EXPECT_EQ(std::to_string(x * (x * x)), "pow(x,3)");
//   EXPECT_EQ(std::to_string((x * x) * x), "pow(x,3)");
//   EXPECT_EQ(std::to_string(std::pow(x, _2) * std::pow(x, _2)), "pow(x,4)");
//   EXPECT_EQ(std::to_string(std::pow(x, x) * x), "pow(x,1+x)");
//   EXPECT_EQ(std::to_string(std::pow(x, _1 + x) * x), "pow(x,2+x)");
// }

// TYPED_TEST(ScalarExpressionTest, ScalarFunctionPrint) {
//   [[maybe_unused]] auto &x = this->x;
//   [[maybe_unused]] auto &y = this->y;
//   [[maybe_unused]] auto &z = this->z;
//   [[maybe_unused]] auto &_1 = this->_1;
//   [[maybe_unused]] auto &_2 = this->_2;
//   [[maybe_unused]] auto &_3 = this->_3;

//   EXPECT_EQ(std::to_string(std::pow(x, _2) * x), "pow(x,3)");
//   EXPECT_EQ(std::to_string(std::pow(x, _2) * std::pow(x, _2)), "pow(x,4)");
//   EXPECT_EQ(std::to_string(std::pow(x, x) * x), "pow(x,1+x)");
//   EXPECT_EQ(std::to_string(std::pow(x, _1 + x) * x), "pow(x,2+x)");

//   EXPECT_EQ(std::to_string(std::cos(x)), "cos(x)");
//   EXPECT_EQ(std::to_string(std::sin(x)), "sin(x)");
//   EXPECT_EQ(std::to_string(std::tan(x)), "tan(x)");
//   EXPECT_EQ(std::to_string(std::exp(x)), "exp(x)");
// }

// TYPED_TEST(ScalarExpressionTest, ScalarFundamentalDerivatives) {
//   auto &x = this->x;
//   auto &y = this->y;
//   auto &z = this->z;
//   auto &_1 = this->_1;
//   auto &zero{this->_zero};
//   auto &one{this->_one};
//   EXPECT_EQ(std::to_string(numsim::cas::diff(zero, x)), "0");
//   EXPECT_EQ(std::to_string(numsim::cas::diff(one, x)), "0");
//   EXPECT_EQ(std::to_string(numsim::cas::diff(_1, x)), "0");
//   EXPECT_EQ(std::to_string(numsim::cas::diff(x, x)), "1");
//   EXPECT_EQ(std::to_string(numsim::cas::diff(x * y * z, x)), "y*z");
//   // EXPECT_EQ(std::to_string(numsim::cas::diff(x/x, x)), "2");
//   EXPECT_EQ(std::to_string(numsim::cas::diff(x + x, x)), "2");
//   EXPECT_EQ(std::to_string(numsim::cas::diff(x * x, x)), "2*x");
//   EXPECT_EQ(std::to_string(numsim::cas::diff(x * x * x, x)), "3*pow(x,2)");
//   EXPECT_EQ(std::to_string(numsim::cas::diff(x * x * x * x, x)), "4*pow(x,3)");

//   //  EXPECT_EQ(std::to_string(numsim::cas::diff(-x - x - y - z, x)), "-2");
//   //  EXPECT_EQ(std::to_string(numsim::cas::diff(-x - x - y - z, y)), "-1");
//   //  EXPECT_EQ(std::to_string(numsim::cas::diff(-x - x - y - z, z)), "-1");
// }

// TYPED_TEST(ScalarExpressionTest, ScalarCanonicalOrderingPrint) {
//   auto &x = this->x, &y = this->y, &z = this->z;
//   EXPECT_EQ(std::to_string(y * x), "x*y");                                 // product ordering
//   EXPECT_EQ(std::to_string(y + x + z + y + x), "2*x+2*y+z");               // sum collection + ordering
//   EXPECT_EQ(std::to_string(this->_2 + x), "2+x");                          // constant first
//   EXPECT_EQ(std::to_string(x + this->_2), "2+x");
// }

// TYPED_TEST(ScalarExpressionTest, ScalarDivisionSimplifyPrint) {
//   auto &x = this->x, &y = this->y, &a = this->a, &b = this->b;
//   using std::pow;
//   EXPECT_EQ(std::to_string(x / x), "1");
//   EXPECT_EQ(std::to_string(y * x / x), "x*y/x");
//   EXPECT_EQ(std::to_string(pow(x, this->_3) / x), "pow(x,2)");
//   EXPECT_EQ(std::to_string((a * b) / a), "a*b/a");
//   EXPECT_EQ(std::to_string((x / y) * y), "y*x/y");
// }

// TYPED_TEST(ScalarExpressionTest, ScalarNegationAndParensPrint) {
//   auto &x = this->x, &y = this->y;
//   EXPECT_EQ(std::to_string(-(x + y)), "-(x+y)");      // wrap sums in parens under negation
//   EXPECT_EQ(std::to_string(-(-x)), "x");              // double negation
//   EXPECT_EQ(std::to_string(x + (-x)), "0");           // additive inverse
// }

// TYPED_TEST(ScalarExpressionTest, ScalarPowerLawsPrint) {
//   auto &x = this->x, &y = this->y;
//   using std::pow;
//   EXPECT_EQ(std::to_string(pow(x, this->_2) * pow(x, this->_3)), "pow(x,5)");
//   EXPECT_EQ(std::to_string(pow(x, this->_2) / pow(x, this->_1)), "x");
//   EXPECT_EQ(std::to_string(pow(x, y) * x), "pow(x,1+y)");   // symbolic exponent bump
// }

// TYPED_TEST(ScalarExpressionTest, ScalarMoreDivisionFormatting) {
//   // int vs float already covered for 1/2; add a sanity for 2/1 -> "2"
//   auto &_1 = this->_1, &_2 = this->_2;
//   EXPECT_EQ(std::to_string(_2 / _1), "2");
// }

// TYPED_TEST(ScalarExpressionTest, DerivativeLinearityAndBasics) {
//   auto &x = this->x, &y = this->y, &a = this->a, &b = this->b, &c = this->c;
//   EXPECT_EQ(std::to_string(numsim::cas::diff(this->_2 * x + this->_3, x)), "2");
//   EXPECT_EQ(std::to_string(numsim::cas::diff(a * x + b * y + c, x)), "a"); // a,b,c independent of x
//   EXPECT_EQ(std::to_string(numsim::cas::diff(x * y, x)), "y");             // product rule w/ const factor
//   EXPECT_EQ(std::to_string(numsim::cas::diff((x + y) * x, x)), "2*x+y");   // product rule + simplification
//   EXPECT_EQ(std::to_string(numsim::cas::diff(x / y, x)), "pow(y,-1)");           // quotient rule w/ const denom
// }

// TYPED_TEST(ScalarExpressionTest, DerivativeChainRule) {
//   auto &x = this->x;
//   using std::pow;
//   // d/dx (x+1)^3 = 3*(x+1)^2  (your canonical form seems to prefer "1+x")
//   EXPECT_EQ(std::to_string(numsim::cas::diff(pow(x + this->_1, this->_3), x)), "3*pow(1+x,2)");
// }

// TYPED_TEST(ScalarExpressionTest, ScalarMixedCollection) {
//   auto &x = this->x, &y = this->y, &z = this->z;
//   EXPECT_EQ(std::to_string((x + y + z) + (y + x)), "2*x+2*y+z"); // cross-sum collection
//   EXPECT_EQ(std::to_string(this->_2 * (this->_3 + x)), "2*(3+x)"); // no forced distribution; constant first inside
// }

// TYPED_TEST(ScalarExpressionTest, Add_CombineSameSymbol) {
//   auto &x = this->x;
//   EXPECT_EQ(std::to_string(x + x), "2*x");
// }

// TYPED_TEST(ScalarExpressionTest, Add_CombineMulAndSymbol_EitherSide) {
//   auto &x = this->x;
//   auto &_2 = this->_2;
//   EXPECT_EQ(std::to_string(_2 * x + x), "3*x");
//   EXPECT_EQ(std::to_string(x + _2 * x), "3*x");
// }

// TYPED_TEST(ScalarExpressionTest, Add_CombineTwoMultiplies) {
//   auto &x = this->x;
//   auto &_2 = this->_2, &_3 = this->_3;
//   EXPECT_EQ(std::to_string(_2 * x + _3 * x), "5*x");
// }

// TYPED_TEST(ScalarExpressionTest, Add_ZeroRules) {
//   auto &x = this->x, &y = this->y;
//   auto &zero = this->_zero;
//   EXPECT_EQ(std::to_string(x + zero), "x");
//   EXPECT_EQ(std::to_string(zero + (x + y)), "x+y");
// }

// TYPED_TEST(ScalarExpressionTest, Add_NegationAndInverse) {
//   auto &x = this->x, &y = this->y;
//   EXPECT_EQ(std::to_string(x + (-x)), "0");
//   EXPECT_EQ(std::to_string((-x) + (-x)), "-2*x");
//   EXPECT_EQ(std::to_string((x + y) + (-x)), "y");
// }

// TYPED_TEST(ScalarExpressionTest, Add_ConstantFoldingIntoNAry_LeftAndRight) {
//   auto &x = this->x, &y = this->y;
//   auto &_1 = this->_1, &_2 = this->_2;
//   EXPECT_EQ(std::to_string((x + y) + _2), "2+x+y");   // exercises lvalue branch (bug #1)
//   EXPECT_EQ(std::to_string(_2 + (x + y)), "2+x+y");   // symmetric path
//   EXPECT_EQ(std::to_string((x + _1) + _2), "3+x");
//   EXPECT_EQ(std::to_string(_2 + (x + _1)), "3+x");
// }

// TYPED_TEST(ScalarExpressionTest, Add_MergeAcrossNestedAdd) {
//   auto &x = this->x, &y = this->y, &z = this->z;
//   EXPECT_EQ(std::to_string((x + y) + x), "2*x+y");
//   EXPECT_EQ(std::to_string((x + y + z) + (y + x)), "2*x+2*y+z");
// }

// TYPED_TEST(ScalarExpressionTest, Add_CanonicalOrderingAndCoeffFirst) {
//   auto &x = this->x, &y = this->y, &z = this->z;
//   auto &_2 = this->_2, &_3 = this->_3;
//   EXPECT_EQ(std::to_string(y + x + z + y + x), "2*x+2*y+z");
//   EXPECT_EQ(std::to_string(x + _2), "2+x");          // constant first
//   EXPECT_EQ(std::to_string(_3 + x + y + _2), "5+x+y");
// }



#endif // SCALAREXPRESSIONTEST_H
