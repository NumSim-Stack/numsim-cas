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
  EXPECT_PRINT(_1 / _2, "1/2");
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

  // 0/x → 0
  auto &zero = this->_zero;
  EXPECT_PRINT(zero / x, "0");
  EXPECT_PRINT(zero / _2, "0");
}

//
// PRINT_RATIONAL_ARITHMETIC — exact rational constant folding
//
TEST_F(ScalarFixture, PRINT_RationalArithmetic) {
  auto &_1 = this->_1, &_2 = this->_2, &_3 = this->_3;

  // basic fractions
  EXPECT_PRINT(_1 / _3, "1/3");
  EXPECT_PRINT(_2 / _3, "2/3");

  // fraction addition
  EXPECT_PRINT(_1 / _3 + _1 / _3, "2/3");
  EXPECT_PRINT(_1 / _2 + _1 / _3, "5/6");

  // fraction multiplication
  EXPECT_PRINT((_1 / _2) * (_1 / _3), "1/6");

  // integer / integer that reduces
  EXPECT_PRINT(_2 / _2, "1");
  EXPECT_PRINT(_3 / _3, "1");

  // rational in multiplication context: parenthesized
  EXPECT_PRINT(x * (_1 / _3), "(1/3)*x");
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

//
// ABS simplification with assumptions
//
TEST_F(ScalarFixture, AbsSimplification) {
  using namespace numsim::cas;

  // abs(positive) → x
  auto px = make_expression<scalar>("px");
  assume(px, positive{});
  EXPECT_PRINT(abs(px), "px");

  // abs(-positive) → positive
  auto py = make_expression<scalar>("py");
  auto neg_py = -py;
  assume(py, positive{});
  EXPECT_PRINT(abs(neg_py), "py");

  // abs(positive constant) → constant
  auto c5 = make_expression<scalar_constant>(5);
  assume(c5, positive{});
  EXPECT_PRINT(abs(c5), "5");
}

//
// SIGN simplification with assumptions
//
TEST_F(ScalarFixture, SignSimplification) {
  using namespace numsim::cas;

  // sign(0) → 0
  EXPECT_PRINT(sign(_zero), "0");

  auto px = make_expression<scalar>("spx");
  assume(px, positive{});
  EXPECT_PRINT(sign(px), "1");

  auto ny = make_expression<scalar>("sny");
  assume(ny, negative{});
  EXPECT_PRINT(sign(ny), "-1");
}

//
// TRIG function simplification — sin/cos/tan of zero, inverse pairs
//
TEST_F(ScalarFixture, TrigFunctionSimplification) {
  using namespace numsim::cas;

  auto &x = this->x;
  auto &zero = this->_zero;

  // sin(0) → 0
  EXPECT_PRINT(sin(zero), "0");
  // cos(0) → 1
  EXPECT_PRINT(cos(zero), "1");
  // tan(0) → 0
  EXPECT_PRINT(tan(zero), "0");

  // sin(asin(x)) → x
  EXPECT_PRINT(sin(asin(x)), "x");
  // cos(acos(x)) → x
  EXPECT_PRINT(cos(acos(x)), "x");
  // tan(atan(x)) → x
  EXPECT_PRINT(tan(atan(x)), "x");
}

//
// EXP/LOG simplification — zero arg, inverse pairs
//
TEST_F(ScalarFixture, ExpLogSimplification) {
  using namespace numsim::cas;

  auto &x = this->x;
  auto &zero = this->_zero;
  auto &one = this->_one;
  auto &_1 = this->_1;

  // exp(0) → 1
  EXPECT_PRINT(exp(zero), "1");
  // log(1) → 0 (scalar_one)
  EXPECT_PRINT(log(one), "0");
  // log(1) → 0 (scalar_constant(1))
  EXPECT_PRINT(log(_1), "0");

  // exp(log(x)) → x
  EXPECT_PRINT(exp(log(x)), "x");
  // log(exp(x)) → x
  EXPECT_PRINT(log(exp(x)), "x");
}

//
// INVERSE TRIG simplification — zero/identity arg
//
TEST_F(ScalarFixture, InverseTrigSimplification) {
  using namespace numsim::cas;

  auto &zero = this->_zero;
  auto &one = this->_one;
  auto &_1 = this->_1;

  // asin(0) → 0
  EXPECT_PRINT(asin(zero), "0");
  // atan(0) → 0
  EXPECT_PRINT(atan(zero), "0");
  // acos(1) → 0 (scalar_one)
  EXPECT_PRINT(acos(one), "0");
  // acos(1) → 0 (scalar_constant(1))
  EXPECT_PRINT(acos(_1), "0");
}

//
// SQRT simplification — zero and one
//
TEST_F(ScalarFixture, SqrtSimplification) {
  using namespace numsim::cas;

  auto &zero = this->_zero;
  auto &one = this->_one;
  auto &_1 = this->_1;

  // sqrt(0) → 0
  EXPECT_PRINT(sqrt(zero), "0");
  // sqrt(1) → 1 (scalar_one)
  EXPECT_PRINT(sqrt(one), "1");
  // sqrt(1) → 1 (scalar_constant(1))
  EXPECT_PRINT(sqrt(_1), "1");
}

//
// EXP product simplification — exp(x)*exp(y) → exp(x+y)
//
TEST_F(ScalarFixture, MUL_ExpCombine) {
  EXPECT_PRINT(exp(x) * exp(y), "exp(x+y)");
  EXPECT_PRINT(exp(x) * exp(x), "exp(2*x)");
  EXPECT_PRINT(exp(x + y) * exp(z), "exp(x+y+z)");
  // n_ary: 2*exp(x)*exp(y)
  EXPECT_PRINT(_2 * exp(x) * exp(y), "2*exp(x+y)");
  // exp in mul RHS: exp(x)*(z*exp(y))
  EXPECT_PRINT(exp(x) * (z * exp(y)), "z*exp(x+y)");
}

//
// Pythagorean identity — sin^2 + cos^2 → 1
//
TEST_F(ScalarFixture, ADD_TrigPythagorean) {
  EXPECT_PRINT(pow(sin(x), _2) + pow(cos(x), _2), "1");
  EXPECT_PRINT(pow(cos(x), _2) + pow(sin(x), _2), "1"); // order invariance
  // n_ary: sin^2 + y + cos^2
  EXPECT_PRINT(pow(sin(x), _2) + y + pow(cos(x), _2), "1+y");
}

//
// ODD/EVEN function simplification — sin(-x) → -sin(x), cos(-x) → cos(x)
//
TEST_F(ScalarFixture, Scalar_OddEvenFunctions) {
  EXPECT_PRINT(sin(-x), "-sin(x)");
  EXPECT_PRINT(cos(-x), "cos(x)");
  EXPECT_PRINT(sin(-sin(x)), "-sin(sin(x))");
  EXPECT_PRINT(cos(-cos(x)), "cos(cos(x))");
}

//
// EXP pow simplification — exp(a)^n → exp(n*a)
//
TEST_F(ScalarFixture, Scalar_ExpPowSimplification) {
  EXPECT_PRINT(pow(exp(x), _2), "exp(2*x)");
  EXPECT_PRINT(pow(exp(x), _3), "exp(3*x)");
  EXPECT_PRINT(pow(exp(x + y), _2), "exp(2*(x+y))");
}

//
// Operator early-exit coverage: zero/one identity & annihilator for +, -, *, /
//
TEST_F(ScalarFixture, OperatorEarlyExit_SubZero) {
  using namespace numsim::cas;

  // 0 - x → -x
  EXPECT_PRINT(_zero - x, "-x");
  // x - 0 → x
  EXPECT_PRINT(x - _zero, "x");
  // 0 - 0 → 0
  EXPECT_PRINT(_zero - _zero, "0");
}

//
// PRINT_PowConstantFolding — pow(constant, constant) → constant
//
TEST_F(ScalarFixture, PRINT_PowConstantFolding) {
  EXPECT_PRINT(pow(_2, _3), "8");         // 2^3 = 8
  EXPECT_PRINT(pow(_3, _2), "9");         // 3^2 = 9
  EXPECT_PRINT(pow(_2, -_1), "1/2");      // 2^(-1) = 1/2
  EXPECT_PRINT(pow(_2, -_2), "1/4");      // 2^(-2) = 1/4
  EXPECT_PRINT(pow(_3, -_1), "1/3");      // 3^(-1) = 1/3
  EXPECT_PRINT(_2 * pow(_2, -_1), "1");   // 2 * 1/2 = 1 (the key case)
  EXPECT_PRINT(pow(_1, _3), "1");         // 1^3 = 1
  EXPECT_PRINT(pow(-_1, _2), "1");        // (-1)^2 = 1
  EXPECT_PRINT(pow(-_1, _3), "-1");       // (-1)^3 = -1
}

TEST_F(ScalarFixture, OperatorEarlyExit_NegZero) {
  using namespace numsim::cas;

  // -0 → 0
  EXPECT_PRINT(-_zero, "0");
  // -(-x) → x (already tested but included for completeness)
  EXPECT_PRINT(-(-x), "x");
}

#endif // SCALAREXPRESSIONTEST_H
