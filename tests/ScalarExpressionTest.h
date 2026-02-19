#ifndef SCALAREXPRESSIONTEST_H
#define SCALAREXPRESSIONTEST_H

#include "cas_test_helpers.h"
#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"

#include <cmath>
#include <sstream>
#include <string_view>
#include <tuple>

//
// Scalar fixture (NON templated)
//
struct ScalarFixture : ::testing::Test {
  using scalar_expr =
      numsim::cas::expression_holder<numsim::cas::scalar_expression>;

  scalar_expr x, y, z;
  scalar_expr a, b, c, d;
  scalar_expr _1, _2, _3;

  scalar_expr _zero{numsim::cas::get_scalar_zero()};
  scalar_expr _one{numsim::cas::get_scalar_one()};

  ScalarFixture() {
    std::tie(x, y, z) = numsim::cas::make_scalar_variable("x", "y", "z");
    std::tie(a, b, c, d) =
        numsim::cas::make_scalar_variable("a", "b", "c", "d");
    std::tie(_1, _2, _3) = numsim::cas::make_scalar_constant(1, 2, 3);
  }
};

// Bring numeric overloads (ADL will pick CAS overloads for expressions)
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

// NOTE: there is no std::sign; use numsim::cas::sign(...) if you have a free
// function, or test sign via make_expression<scalar_sign>(...).

/*
Nomenclature & conventions
- PRINT_*: checks printed canonical forms (ordering, collection, formatting)
- ADD_*: addition-specific merges
- We compare strings via EXPECT_PRINT(expr, "literal") unless equality up to
  printing is desired → EXPECT_SAME_PRINT(lhs, rhs).
*/

//
// PRINT_FUNDAMENTALS — zeros, ones, identity/annihilator laws
//
TEST_F(ScalarFixture, PRINT_Fundamentals) {
  auto &x = this->x, &y = this->y, &z = this->z;
  auto &_1 = this->_1, &_2 = this->_2, &_3 = this->_3;
  auto &zero = this->_zero, &one = this->_one;

  EXPECT_PRINT(_1, "1");
  EXPECT_PRINT(_1 + _2 + _3, "6");
  EXPECT_PRINT(-_1 + _2 + _3, "4");
  EXPECT_PRINT(-_1 - _2 - _3, "-6");
  EXPECT_PRINT(_1 - _2 - _3, "-4");
  EXPECT_PRINT(-_1 + x + _1, "x");
  EXPECT_PRINT(-_1 + _1 + x, "x");
  EXPECT_PRINT(-_1 + (_1 + x), "x");
  EXPECT_PRINT(_1 + x - _1, "x");
  EXPECT_PRINT((_1 + x) - _1, "x");

  EXPECT_PRINT(-1 + x + 1, "x");
  EXPECT_PRINT(-1 + 1 + x, "x");
  EXPECT_PRINT(-1 + (1 + x), "x");
  EXPECT_PRINT(1 + x - 1, "x");
  EXPECT_PRINT((1 + x) - 1, "x");

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
TEST_F(ScalarFixture, PRINT_Addition) {
  auto &x = this->x, &y = this->y, &z = this->z;
  auto &_1 = this->_1, &_2 = this->_2, &_3 = this->_3;

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
TEST_F(ScalarFixture, PRINT_MultiplicationAndPowers) {
  auto &x = this->x, &y = this->y;
  auto &_1 = this->_1, &_2 = this->_2, &_3 = this->_3;

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
// PRINT_DIVISION_FORMAT — formatting and flattening
//
TEST_F(ScalarFixture, PRINT_DivisionFormat) {
  auto &x = this->x, &y = this->y, &a = this->a, &b = this->b, &c = this->c,
       &d = this->d;
  auto &_1 = this->_1, &_2 = this->_2;

  EXPECT_PRINT(z * pow(x * y, -_2), "z/pow(x*y,2)");
  EXPECT_PRINT(z * pow(x, -_1) * pow(y, -_1), "z/(x*y)");
  EXPECT_PRINT(z * pow(x, -1) * pow(y, -1), "z/(x*y)");
  EXPECT_PRINT(_1 / _2, "pow(2,-1)");
  EXPECT_PRINT(pow(y, -_zero), "1");
  EXPECT_PRINT(pow(pow(y, -_1), -_1), "y");
  EXPECT_PRINT(pow(pow(y, -_2), -_2), "pow(y,4)");
  EXPECT_PRINT(pow(pow(y, -_2), _2), "pow(y,-4)");
  EXPECT_PRINT(x * pow(y, -_2), "x/pow(y,2)");
  EXPECT_PRINT(pow(x, -_1) * pow(y, -_1), "pow(x*y,-1)");
  EXPECT_PRINT(pow(pow(cos(x), -1), 2), "pow(cos(x),-2)");
  // pow(pow(x, a), -y) --> pow(x, -a*y)
  EXPECT_PRINT(pow(pow(x, _2), -y), "pow(x,-2*y)");
  EXPECT_PRINT(pow(pow(x, _3), -y), "pow(x,-3*y)");
  EXPECT_PRINT(pow(pow(x, _2), -x), "pow(x,-2*x)");
  EXPECT_PRINT(pow(pow(x, y), -z), "pow(x,-y*z)");
  EXPECT_PRINT(pow(pow(x, y), -x), "pow(x,-x*y)");
  EXPECT_PRINT(x / 1, "x");
  EXPECT_PRINT(x / (-2), "-x/2");
  EXPECT_PRINT(x / y, "x/y");
  EXPECT_PRINT(a / b / c / d, "a/(b*c*d)");
  EXPECT_PRINT(a / b + c + d, "c+d+a/b");
  EXPECT_PRINT(a / (b + c + d), "a/(b+c+d)");
  // a*pow(b,-1) / (c * pow(d, -1)) --> a*pow(b,-1) * pow(c*pow(d,-1),-1)
  EXPECT_PRINT((a / b) / (c / d), "a*d/(b*c)");
  EXPECT_PRINT((a + b) / (c + d), "(a+b)/(c+d)");
}

//
// PRINT_NEGATION_AND_PARENS — canonical parentheses and sign handling
//
TEST_F(ScalarFixture, PRINT_NegationAndParens) {
  auto &x = this->x, &y = this->y, &z = this->z;

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
TEST_F(ScalarFixture, PRINT_Mixed) {
  auto &x = this->x, &y = this->y, &z = this->z;
  auto &_2 = this->_2, &_3 = this->_3;

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
// PRINT_FUNCTIONS — trig/exp/log prints
//
TEST_F(ScalarFixture, PRINT_Functions) {
  using namespace numsim::cas;

  auto &x = this->x;

  // Base literals / symbols
  EXPECT_PRINT(x, "x");
  EXPECT_PRINT(make_expression<scalar_zero>(), "0");
  EXPECT_PRINT(make_expression<scalar_one>(), "1");

  auto two = make_expression<scalar_constant>(2);
  EXPECT_PRINT(two, "2");

  // Unary ops
  EXPECT_PRINT(make_expression<scalar_negative>(x), "-x");
  EXPECT_PRINT(make_expression<scalar_abs>(x), "abs(x)");
  EXPECT_PRINT(make_expression<scalar_sign>(x), "sign(x)");

  // Binary ops
  auto add = make_expression<scalar_add>();
  add.template get<scalar_add>().push_back(x);
  add.template get<scalar_add>().push_back(two);
  EXPECT_PRINT(add, "x+2");

  auto mul = make_expression<scalar_mul>();
  mul.template get<scalar_mul>().push_back(x);
  mul.template get<scalar_mul>().push_back(two);
  EXPECT_PRINT(mul, "x*2");

  // EXPECT_PRINT(make_expression<scalar_div>(x, two), "x/2");

  // Powers / exponentials / logs
  EXPECT_PRINT(pow(z, z) * pow(x, x) * pow(y, y), "pow(x,x)*pow(y,y)*pow(z,z)");
  EXPECT_PRINT(pow(x, two), "pow(x,2)");
  EXPECT_PRINT(sqrt(x), "sqrt(x)");
  EXPECT_PRINT(log(x), "log(x)");
  EXPECT_PRINT(exp(x), "exp(x)");

  // Trig
  EXPECT_PRINT(sin(x), "sin(x)");
  EXPECT_PRINT(cos(x), "cos(x)");
  EXPECT_PRINT(tan(x), "tan(x)");

  // Inverse trig
  EXPECT_PRINT(asin(x), "asin(x)");
  EXPECT_PRINT(acos(x), "acos(x)");
  EXPECT_PRINT(atan(x), "atan(x)");

  // Function wrapper
  EXPECT_PRINT(make_expression<scalar_named_expression>("func", x * x),
               "func = pow(x,2)");

  auto expr = x * sqrt(x) * log(x) * exp(x) * cos(x) * sin(x) * tan(x) *
              acos(x) * asin(x) * atan(x) * abs(x) * sign(x) * pow(x, x) *
              (two / x);

  EXPECT_PRINT(expr,
               "2*pow(x,x)*atan(x)*acos(x)*asin(x)*tan(x)*cos(x)*sin(x)*abs(x)*"
               "sign(x)*exp(x)*log(x)*sqrt(x)");
}

//
// PRINT_CANONICAL_ORDERING — ordering for sums/products; constant first in sums
//
TEST_F(ScalarFixture, PRINT_CanonicalOrdering) {
  auto &x = this->x, &y = this->y, &z = this->z;
  EXPECT_PRINT(y * x, "x*y");
  EXPECT_PRINT(y + x + z + y + x, "2*x+2*y+z");
  EXPECT_PRINT(this->_2 + x, "2+x");
  EXPECT_PRINT(x + this->_2, "2+x");
}

//
// PRINT_DIVISION_NO_CANCEL — current behavior (no cross-cancellation yet)
//
TEST_F(ScalarFixture, PRINT_Division_NoCancel) {
  auto &x = this->x, &y = this->y, &a = this->a, &b = this->b;
  EXPECT_PRINT(x / x, "1");
  EXPECT_PRINT(y * x / x, "y");
  EXPECT_PRINT(pow(x, numsim::cas::get_scalar_one()) / x, "1");
  EXPECT_PRINT(pow(x, numsim::cas::get_scalar_zero()) / x, "pow(x,-1)");
  EXPECT_PRINT(pow(x, this->_1) / x, "1");
  EXPECT_PRINT(pow(x, this->_3) / x, "pow(x,2)");
  EXPECT_PRINT(x * pow(x, x), "pow(x,1+x)");
  EXPECT_PRINT(pow(x, x) / x, "pow(x,-1+x)");
  EXPECT_PRINT(pow(x, x) * (2 / x), "2*pow(x,-1+x)");
  EXPECT_PRINT(x * pow(x, x) * (2 / x), "2*pow(x,x)");
  EXPECT_PRINT(pow(x, _1) * (x * y * z), "pow(x,2)*y*z");
  EXPECT_PRINT(pow(x, _1 + x) / x, "pow(x,x)");
  EXPECT_PRINT((a * b) / a, "b");
  EXPECT_PRINT((x / y) * y, "x");
}

//
// ADD_* — targeted add/combine cases
//
//
// POW_Simplification — pow-of-pow flattening, identity/zero exponent,
//                      negative base extraction, mul-pow extraction
//
TEST_F(ScalarFixture, POW_Simplification) {
  auto &x = this->x, &y = this->y, &z = this->z;
  auto &_1 = this->_1, &_2 = this->_2, &_3 = this->_3;
  auto &zero = this->_zero, &one = this->_one;

  // --- Identity / zero exponent ---
  EXPECT_PRINT(pow(x, zero), "1");
  EXPECT_PRINT(pow(x, one), "x");
  EXPECT_PRINT(pow(x, _1), "x");
  EXPECT_PRINT(pow(one, x), "1");
  EXPECT_PRINT(pow(one, _3), "1");

  // --- pow of pow: pow(pow(x,a),b) → pow(x,a*b) ---
  EXPECT_PRINT(pow(pow(x, _2), _3), "pow(x,6)");
  EXPECT_PRINT(pow(pow(x, _3), _2), "pow(x,6)");
  EXPECT_PRINT(pow(pow(x, y), z), "pow(x,y*z)");
  EXPECT_PRINT(pow(pow(x, y), _2), "pow(x,2*y)");
  EXPECT_PRINT(pow(pow(x, _2), y), "pow(x,2*y)");

  // --- pow of pow with negation ---
  EXPECT_PRINT(pow(pow(x, -_1), -_1), "x");
  EXPECT_PRINT(pow(pow(x, -_2), -_2), "pow(x,4)");
  EXPECT_PRINT(pow(pow(x, -_2), _2), "pow(x,-4)");

  // --- Negative base extraction: pow(-x, p) → -pow(x, p) ---
  EXPECT_PRINT(pow(-x, _2), "-pow(x,2)");
  EXPECT_PRINT(pow(-x, y), "-pow(x,y)");

  // --- Division cancellation: pow(x, -x) → 1 ---
  EXPECT_PRINT(pow(x, -x), "1");

  // --- Mul factor cancel: pow(x*y, -y) → x ---
  EXPECT_PRINT(pow(x * y, -y), "x");

  // --- Mul-pow extraction: pow(x*pow(y,a), b) → pow(x,b)*pow(y,a*b) ---
  EXPECT_PRINT(pow(x * pow(y, _2), _3), "pow(y,6)*pow(x,3)");
  EXPECT_PRINT(pow(x * pow(y, z), _2), "pow(y,2*z)*pow(x,2)");
}

TEST_F(ScalarFixture, ADD_CombineSameSymbol) {
  auto &x = this->x;
  EXPECT_PRINT(x + x, "2*x");
}

TEST_F(ScalarFixture, ADD_CombineMulAndSymbol_EitherSide) {
  auto &x = this->x;
  auto &_2 = this->_2;
  EXPECT_PRINT(_2 * x + x, "3*x");
  EXPECT_PRINT(x + _2 * x, "3*x");
}

TEST_F(ScalarFixture, ADD_CombineTwoMultiplies) {
  auto &x = this->x;
  auto &_2 = this->_2, &_3 = this->_3;
  EXPECT_PRINT(_2 * x + _3 * x, "5*x");
}

TEST_F(ScalarFixture, ADD_ZeroRules) {
  auto &x = this->x, &y = this->y;
  auto &zero = this->_zero;
  EXPECT_PRINT(x + zero, "x");
  EXPECT_PRINT(zero + (x + y), "x+y");
}

TEST_F(ScalarFixture, ADD_NegationAndInverse) {
  auto &x = this->x, &y = this->y;
  EXPECT_PRINT(x + (-x), "0");
  EXPECT_PRINT((-x) + (-x), "-2*x");
  EXPECT_PRINT((x + y) + (-x), "y");
}

TEST_F(ScalarFixture, ADD_ConstantFoldingIntoNAry_LeftAndRight) {
  auto &x = this->x, &y = this->y;
  auto &_1 = this->_1, &_2 = this->_2;
  EXPECT_PRINT((x + y) + _2, "2+x+y");
  EXPECT_PRINT(_2 + (x + y), "2+x+y");
  EXPECT_PRINT((x + _1) + _2, "3+x");
  EXPECT_PRINT(_2 + (x + _1), "3+x");
}

TEST_F(ScalarFixture, ADD_MergeAcrossNestedAdd) {
  auto &x = this->x, &y = this->y, &z = this->z;
  EXPECT_PRINT((x + y) + x, "2*x+y");
  EXPECT_PRINT((x + y + z) + (y + x), "2*x+2*y+z");
}

TEST_F(ScalarFixture, ADD_CanonicalOrderingAndCoeffFirst) {
  auto &x = this->x, &y = this->y, &z = this->z;
  auto &_2 = this->_2, &_3 = this->_3;
  EXPECT_PRINT(y + x + z + y + x, "2*x+2*y+z");
  EXPECT_PRINT(x + _2, "2+x");
  EXPECT_PRINT(_3 + x + y + _2, "5+x+y");
}

#endif // SCALAREXPRESSIONTEST_H
