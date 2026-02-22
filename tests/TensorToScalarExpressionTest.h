#ifndef TENSORTOSCALAREXPRESSIONTEST_H
#define TENSORTOSCALAREXPRESSIONTEST_H

#include "cas_test_helpers.h"
#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"

namespace {

using TestDims = ::testing::Types<std::integral_constant<std::size_t, 1>,
                                  std::integral_constant<std::size_t, 2>,
                                  std::integral_constant<std::size_t, 3>>;

} // namespace

template <typename DimTag>
class TensorToScalarExpressionTest : public ::testing::Test {
protected:
  static constexpr std::size_t Dim = DimTag::value;

  using tensor_t =
      numsim::cas::expression_holder<numsim::cas::tensor_expression>;
  using scalar_t =
      numsim::cas::expression_holder<numsim::cas::scalar_expression>;

  TensorToScalarExpressionTest() {
    std::tie(X, Y, Z) = numsim::cas::make_tensor_variable(
        std::tuple{"X", Dim, 2}, std::tuple{"Y", Dim, 2},
        std::tuple{"Z", Dim, 2});

    std::tie(A, B, C) = numsim::cas::make_tensor_variable(
        std::tuple{"A", Dim, 4}, std::tuple{"B", Dim, 4},
        std::tuple{"C", Dim, 4});

    std::tie(x, y, z) = numsim::cas::make_scalar_variable("x", "y", "z");
    std::tie(a, b, c) = numsim::cas::make_scalar_variable("a", "b", "c");

    // IMPORTANT: do NOT "using std::pow;" in these tests.
    // Keep constants built through CAS so operator overloads stay in CAS-land.
    std::tie(_1, _2, _3) = numsim::cas::make_scalar_constant(1, 2, 3);

    _zero = scalar_t{numsim::cas::get_scalar_zero()};
    _one = scalar_t{numsim::cas::get_scalar_one()};

    _Zero = tensor_t{
        numsim::cas::make_expression<numsim::cas::tensor_zero>(Dim, 2)};
    _One = tensor_t{
        numsim::cas::make_expression<numsim::cas::kronecker_delta>(Dim)};
  }

  // tensors
  tensor_t X, Y, Z;
  tensor_t A, B, C;

  // scalars
  scalar_t x, y, z;
  scalar_t a, b, c;

  // scalar constants / special
  scalar_t _1, _2, _3;
  scalar_t _zero, _one;

  // tensor special
  tensor_t _Zero, _One;
};

TYPED_TEST_SUITE(TensorToScalarExpressionTest, TestDims);

// ---------- Basics: print and simple algebra ----------
TYPED_TEST(TensorToScalarExpressionTest,
           TensorToScalar_Basics_PrintAndAlgebra) {
  auto &X = this->X;

  auto X_tr = numsim::cas::trace(X);
  auto X_norm = numsim::cas::norm(X);
  auto X_det = numsim::cas::det(X);

  EXPECT_PRINT(X_tr, "tr(X)");
  EXPECT_PRINT(X_norm, "norm(X)");
  EXPECT_PRINT(X_det, "det(X)");
  EXPECT_PRINT(X_tr + X_tr, "2*tr(X)");
  EXPECT_PRINT(X_tr * X_tr, "pow(tr(X),2)");
  EXPECT_PRINT(X_tr / X_tr, "tr(X)/tr(X)");

  EXPECT_PRINT(X_norm * X_norm, "pow(norm(X),2)");
  EXPECT_PRINT(X_det * X_det, "pow(det(X),2)");
}

// ---------- Mixing with scalar symbols ----------
TYPED_TEST(TensorToScalarExpressionTest,
           TensorToScalar_WithScalars_OrderingAndPowers) {
  auto &X = this->X;
  auto &x = this->x;

  auto X_tr = numsim::cas::trace(X);

  EXPECT_SAME_PRINT(x + X_tr, X_tr + x);
  EXPECT_SAME_PRINT((x + X_tr) + x, 2 * x + X_tr);
  EXPECT_SAME_PRINT(x + (X_tr + x), 2 * x + X_tr);
  EXPECT_SAME_PRINT((x + X_tr) + X_tr, x + 2 * X_tr);
  EXPECT_SAME_PRINT(X_tr + (X_tr + x), x + 2 * X_tr);
  EXPECT_SAME_PRINT((X_tr + x) + (X_tr + x), 2 * x + 2 * X_tr);

  EXPECT_SAME_PRINT(x * X_tr, X_tr * x);
  EXPECT_PRINT((x * X_tr) * x, "pow(x,2)*tr(X)");
  EXPECT_PRINT(x * (X_tr * x), "pow(x,2)*tr(X)");
}

// ---------- Division chains with tensor-to-scalar nodes ----------
TYPED_TEST(TensorToScalarExpressionTest,
           TensorToScalar_Division_ChainsAndSimplify) {
  auto &Y = this->Y;
  auto &_2 = this->_2;
  auto &_3 = this->_3;

  using numsim::cas::norm;
  using numsim::cas::trace;

  auto trY = trace(Y);
  auto nY = norm(Y);

  auto scalar_norm = (_2 / nY);
  auto norm_scalar = nY / _2;

  // EXPECT_PRINT(numsim::cas::diff(trace(Y), Y), "I");
  // EXPECT_PRINT(numsim::cas::diff(trace(Y * Y), Y), "2*I");
  // EXPECT_PRINT(numsim::cas::diff(numsim::cas::dot(Y), Y), "2*Y");

  EXPECT_PRINT((_3 / trY), "3/tr(Y)");
  EXPECT_SAME_PRINT((_3 / trY) / (_2 / nY), (_3 * nY) / (_2 * trY));
  EXPECT_SAME_PRINT(norm_scalar / (_3 / trY), (nY * trY) / (_3 * _2));
  EXPECT_SAME_PRINT(norm_scalar / _3, nY / (_2 * _3));
  EXPECT_SAME_PRINT(scalar_norm / trY, _2 / (nY * trY));
}

// ---------- Mixed products ----------
TYPED_TEST(TensorToScalarExpressionTest,
           TensorToScalar_MixedProductsAndPowers) {
  auto &X = this->X;
  auto &x = this->x;

  auto t = numsim::cas::trace(X);
  EXPECT_PRINT(x * t * t, "x*pow(tr(X),2)");
}

// ---------- Subtraction ----------
TYPED_TEST(TensorToScalarExpressionTest,
           TensorToScalar_Subtraction_BasicRules) {
  auto &X = this->X;
  auto &Y = this->Y;

  auto X_tr = numsim::cas::trace(X);
  auto Y_tr = numsim::cas::trace(Y);

  // tr(X) - tr(X) --> 0
  EXPECT_PRINT(X_tr - X_tr, "0");

  // tr(X) - 0 --> tr(X)
  auto t2s_zero =
      numsim::cas::make_expression<numsim::cas::tensor_to_scalar_zero>();
  EXPECT_PRINT(X_tr - t2s_zero, "tr(X)");

  // 0 - tr(X) --> -tr(X)
  EXPECT_PRINT(t2s_zero - X_tr, "-tr(X)");

  // tr(X) - tr(Y) --> tr(X)-tr(Y)  (default: add with negation)
  auto diff = X_tr - Y_tr;
  // Just check it doesn't crash and produces something reasonable
  auto s = testcas::S(diff);
  EXPECT_FALSE(s.empty());
}

// ---------- Numeric coefficient placement in add/mul ----------
TYPED_TEST(TensorToScalarExpressionTest,
           TensorToScalar_NumericCoefficientPlacement) {
  auto &X = this->X;
  auto &_2 = this->_2;
  auto &_3 = this->_3;

  using numsim::cas::trace;

  auto trX = trace(X);
  auto t2s_one =
      numsim::cas::make_expression<numsim::cas::tensor_to_scalar_one>();
  auto t2s_zero =
      numsim::cas::make_expression<numsim::cas::tensor_to_scalar_zero>();

  // --- Addition: numeric scalar goes to coeff slot ---
  // scalar_constant(3) + tr(X) --> coeff=3, child=tr(X)
  EXPECT_PRINT(_3 + trX, "3+tr(X)");
  // tr(X) + scalar_constant(3) --> same result (commutative)
  EXPECT_PRINT(trX + _3, "3+tr(X)");

  // --- Addition: fold two numeric operands ---
  // t2s_one + t2s_one --> 2
  EXPECT_PRINT(t2s_one + t2s_one, "2");
  // scalar_constant(3) + t2s_one --> 4
  EXPECT_PRINT(_3 + t2s_one, "4");

  // --- Addition: n_ary_add folds numeric coeff with incoming numeric ---
  // (2 + tr(X)) + t2s_one --> 3 + tr(X)
  EXPECT_PRINT((_2 + trX) + t2s_one, "3+tr(X)");
  // (2 + tr(X)) + scalar_constant(3) --> 5 + tr(X)
  EXPECT_PRINT((_2 + trX) + _3, "5+tr(X)");

  // --- Addition: zero identity ---
  EXPECT_PRINT(t2s_zero + trX, "tr(X)");
  EXPECT_PRINT(trX + t2s_zero, "tr(X)");

  // --- Multiplication: numeric scalar goes to coeff slot ---
  // scalar_constant(3) * tr(X) --> coeff=3, child=tr(X)
  EXPECT_PRINT(_3 * trX, "3*tr(X)");
  // tr(X) * scalar_constant(3) --> same result
  EXPECT_PRINT(trX * _3, "3*tr(X)");

  // --- Multiplication: fold two numeric operands ---
  // scalar_constant(2) * scalar_constant(3) --> 6
  EXPECT_PRINT(_2 * _3 * trX, "6*tr(X)");

  // --- Subtraction with numerics ---
  // wrap(3) - t2s_one --> 2
  auto t2s_3 = numsim::cas::make_expression<
      numsim::cas::tensor_to_scalar_scalar_wrapper>(
      numsim::cas::make_expression<numsim::cas::scalar_constant>(3));
  EXPECT_PRINT(t2s_3 - t2s_one, "2");
  // t2s_one - t2s_one --> 0
  EXPECT_PRINT(t2s_one - t2s_one, "0");
}

// ---------- Pow simplification ----------
TYPED_TEST(TensorToScalarExpressionTest, TensorToScalar_PowSimplification) {
  auto &X = this->X;
  auto &Y = this->Y;
  auto &x = this->x;
  auto &_2 = this->_2;
  auto &_3 = this->_3;

  using numsim::cas::norm;
  using numsim::cas::trace;

  auto trX = trace(X);
  auto trY = trace(Y);
  auto nX = norm(X);

  auto t2s_zero =
      numsim::cas::make_expression<numsim::cas::tensor_to_scalar_zero>();
  auto t2s_one =
      numsim::cas::make_expression<numsim::cas::tensor_to_scalar_one>();

  // --- Identity / zero exponent ---
  EXPECT_PRINT(numsim::cas::pow(trX, 0), "1");
  EXPECT_PRINT(numsim::cas::pow(trX, 1), "tr(X)");
  EXPECT_PRINT(numsim::cas::pow(trX, t2s_zero), "1");
  EXPECT_PRINT(numsim::cas::pow(trX, t2s_one), "tr(X)");
  EXPECT_PRINT(numsim::cas::pow(t2s_one, trX), "1");
  EXPECT_PRINT(numsim::cas::pow(t2s_one, 5), "1");

  // numeric 0/1 via scalar_wrapper
  EXPECT_PRINT(numsim::cas::pow(trX, this->_zero), "1");
  EXPECT_PRINT(numsim::cas::pow(trX, this->_one), "tr(X)");

  // --- pow of pow: pow(pow(tr(X),a),b) → pow(tr(X),a*b) ---
  EXPECT_PRINT(numsim::cas::pow(numsim::cas::pow(trX, 2), 3), "pow(tr(X),6)");
  EXPECT_PRINT(numsim::cas::pow(numsim::cas::pow(trX, 3), 2), "pow(tr(X),6)");
  EXPECT_PRINT(numsim::cas::pow(numsim::cas::pow(trX, _2), _3), "pow(tr(X),6)");
  EXPECT_PRINT(numsim::cas::pow(numsim::cas::pow(trX, x), _2),
               "pow(tr(X),2*x)");

  // pow of pow with different base expressions
  EXPECT_PRINT(numsim::cas::pow(numsim::cas::pow(nX, 2), 3), "pow(norm(X),6)");

  // --- Negative base extraction: pow(-expr, p) → -pow(expr, p) ---
  EXPECT_PRINT(numsim::cas::pow(-trX, 2), "-pow(tr(X),2)");
  EXPECT_PRINT(numsim::cas::pow(-trX, _3), "-pow(tr(X),3)");

  // --- Division cancellation: pow(expr, -expr) → 1 ---
  EXPECT_PRINT(numsim::cas::pow(trX, -trX), "1");

  // --- Mul factor cancel: pow(expr*x, -x) → expr ---
  EXPECT_PRINT(numsim::cas::pow(trX * nX, -nX), "tr(X)");

  // --- Mul-pow extraction: pow(a*pow(b,c), d) → pow(a,d)*pow(b,c*d) ---
  EXPECT_PRINT(numsim::cas::pow(trX * numsim::cas::pow(nX, 2), 3),
               "pow(norm(X),6)*pow(tr(X),3)");

  // --- Interplay with existing mul simplifier ---
  // tr(X)*tr(X) creates pow(tr(X),2), then pow(pow(tr(X),2),3) simplifies
  auto t_sq = trX * trX;
  EXPECT_PRINT(t_sq, "pow(tr(X),2)");
  EXPECT_PRINT(numsim::cas::pow(t_sq, 3), "pow(tr(X),6)");
}

// ---------- ADD: constant_add (LHS = scalar_wrapper) ----------
TYPED_TEST(TensorToScalarExpressionTest,
           TensorToScalar_ConstantAdd_Simplification) {
  auto &X = this->X;
  auto &_2 = this->_2;

  using numsim::cas::trace;
  auto trX = trace(X);

  auto t2s_one =
      numsim::cas::make_expression<numsim::cas::tensor_to_scalar_one>();

  auto wrap = [](int v) {
    return numsim::cas::make_expression<
        numsim::cas::tensor_to_scalar_scalar_wrapper>(
        numsim::cas::make_expression<numsim::cas::scalar_constant>(v));
  };

  // scalar_wrapper + scalar_wrapper → constant fold
  EXPECT_PRINT(wrap(3) + wrap(2), "5");
  EXPECT_PRINT(wrap(5) + wrap(-5), "0");

  // scalar_wrapper + one → constant fold
  EXPECT_PRINT(wrap(3) + t2s_one, "4");
  EXPECT_PRINT(wrap(-1) + t2s_one, "0");

  // scalar_wrapper + add → merge into coeff
  EXPECT_PRINT(wrap(3) + (_2 + trX), "5+tr(X)");

  // scalar_wrapper + (-scalar_wrapper) → constant fold
  EXPECT_PRINT(wrap(3) + (-wrap(1)), "2");
  EXPECT_PRINT(wrap(1) + (-wrap(1)), "0");

  // scalar_wrapper + non-numeric → get_default (creates add)
  EXPECT_PRINT(wrap(3) + trX, "3+tr(X)");
}

// ---------- ADD: one_add (LHS = tensor_to_scalar_one) ----------
TYPED_TEST(TensorToScalarExpressionTest, TensorToScalar_OneAdd_Simplification) {
  auto &X = this->X;
  auto &_2 = this->_2;

  using numsim::cas::trace;
  auto trX = trace(X);

  auto t2s_one =
      numsim::cas::make_expression<numsim::cas::tensor_to_scalar_one>();

  auto wrap = [](int v) {
    return numsim::cas::make_expression<
        numsim::cas::tensor_to_scalar_scalar_wrapper>(
        numsim::cas::make_expression<numsim::cas::scalar_constant>(v));
  };

  // one + scalar_wrapper → constant fold
  EXPECT_PRINT(t2s_one + wrap(3), "4");

  // one + one → 2
  EXPECT_PRINT(t2s_one + t2s_one, "2");

  // one + add → merge into coeff
  EXPECT_PRINT(t2s_one + (_2 + trX), "3+tr(X)");

  // one + (-one) → 0
  EXPECT_PRINT(t2s_one + (-t2s_one), "0");

  // one + non-numeric → get_default (creates add)
  EXPECT_PRINT(t2s_one + trX, "1+tr(X)");
}

// ---------- ADD/SUB: n_ary_mul coefficient combination ----------
TYPED_TEST(TensorToScalarExpressionTest, TensorToScalar_MulCoeffCombination) {
  auto &X = this->X;
  auto &Y = this->Y;
  auto &_2 = this->_2;
  auto &_3 = this->_3;

  using numsim::cas::trace;
  auto trX = trace(X);
  auto trY = trace(Y);

  // n_ary_mul_add: c1*expr + c2*expr → (c1+c2)*expr
  EXPECT_PRINT((_3 * trX) + (_2 * trX), "5*tr(X)");

  // n_ary_mul_add: different children → not combined
  auto sum = (_3 * trX) + (_2 * trY);
  auto s = testcas::S(sum);
  EXPECT_FALSE(s.empty()); // just verify it doesn't crash

  // n_ary_mul_sub: c1*expr - c2*expr → (c1-c2)*expr
  EXPECT_PRINT((_3 * trX) - (_2 * trX), "tr(X)");

  // n_ary_mul_sub: equal → 0
  EXPECT_PRINT((_3 * trX) - (_3 * trX), "0");

  // n_ary_mul_sub: reversed sign
  EXPECT_PRINT((_2 * trX) - (_3 * trX), "-tr(X)");
}

// ---------- SUB: constant_sub and one_sub ----------
TYPED_TEST(TensorToScalarExpressionTest,
           TensorToScalar_ConstantAndOneSub_Simplification) {
  auto &X = this->X;
  auto &_2 = this->_2;

  using numsim::cas::trace;
  auto trX = trace(X);

  auto t2s_one =
      numsim::cas::make_expression<numsim::cas::tensor_to_scalar_one>();

  auto wrap = [](int v) {
    return numsim::cas::make_expression<
        numsim::cas::tensor_to_scalar_scalar_wrapper>(
        numsim::cas::make_expression<numsim::cas::scalar_constant>(v));
  };

  // constant_sub: scalar_wrapper - scalar_wrapper → constant fold
  EXPECT_PRINT(wrap(5) - wrap(2), "3");

  // constant_sub: scalar_wrapper - one → constant fold
  EXPECT_PRINT(wrap(3) - t2s_one, "2");

  // constant_sub: scalar_wrapper - add → adjust coeff and negate children
  EXPECT_PRINT(wrap(5) - (_2 + trX), "3-tr(X)");

  // one_sub: one - scalar_wrapper → constant fold
  EXPECT_PRINT(t2s_one - wrap(3), "-2");

  // negative_sub: -expr - (coeff + x) → -(expr + coeff + x)
  EXPECT_PRINT(-trX - (_2 + trX), "-(2+2*tr(X))");
  EXPECT_SAME_PRINT(-trX - (wrap(3) + trX), -(wrap(3) + _2 * trX));
}

// ---------- MUL: zero, one, negative, constant_mul ----------
TYPED_TEST(TensorToScalarExpressionTest,
           TensorToScalar_MulSimplifier_Specializations) {
  auto &X = this->X;
  auto &Y = this->Y;
  auto &_2 = this->_2;

  using numsim::cas::trace;
  auto trX = trace(X);
  auto trY = trace(Y);

  auto t2s_zero =
      numsim::cas::make_expression<numsim::cas::tensor_to_scalar_zero>();
  auto t2s_one =
      numsim::cas::make_expression<numsim::cas::tensor_to_scalar_one>();

  auto wrap = [](int v) {
    return numsim::cas::make_expression<
        numsim::cas::tensor_to_scalar_scalar_wrapper>(
        numsim::cas::make_expression<numsim::cas::scalar_constant>(v));
  };

  // zero * expr → zero
  EXPECT_PRINT(t2s_zero * trX, "0");

  // one * expr → expr
  EXPECT_PRINT(t2s_one * trX, "tr(X)");

  // (-expr) * expr → -(expr * expr)
  EXPECT_SAME_PRINT((-trX) * trY, -(trX * trY));

  // constant_mul: scalar_wrapper * scalar_wrapper → numeric multiply
  EXPECT_PRINT(wrap(3) * wrap(2), "6");

  // constant_mul: scalar_wrapper(1) * expr → identity
  EXPECT_PRINT(wrap(1) * trX, "tr(X)");

  // constant_mul: scalar_wrapper * mul → merge coeff
  EXPECT_PRINT(wrap(3) * (_2 * trX), "6*tr(X)");

  // constant_mul: scalar_wrapper * non-numeric → creates mul
  EXPECT_PRINT(wrap(3) * trX, "3*tr(X)");
}

// ---------- Cross-domain subtraction (scalar - t2s, t2s - scalar) ----------
TYPED_TEST(TensorToScalarExpressionTest,
           TensorToScalar_CrossDomainSubtraction) {
  auto &X = this->X;
  auto &x = this->x;
  auto &_2 = this->_2;
  auto &_3 = this->_3;

  using numsim::cas::trace;

  auto trX = trace(X);

  // t2s - scalar: tr(X) - scalar_constant(2)
  EXPECT_PRINT(trX - _2, "-2+tr(X)");

  // scalar - t2s: scalar_constant(3) - tr(X)
  EXPECT_PRINT(_3 - trX, "3-tr(X)");

  // t2s - scalar_variable: tr(X) - x
  EXPECT_PRINT(trX - x, "-x+tr(X)");

  // scalar_variable - t2s: x - tr(X)
  EXPECT_PRINT(x - trX, "x-tr(X)");
}

// ---------- Scalar wrapper merging across operations ----------
TYPED_TEST(TensorToScalarExpressionTest, TensorToScalar_ScalarWrapperMerging) {
  auto &X = this->X;
  auto &x = this->x;
  auto &y = this->y;
  auto &_2 = this->_2;

  using numsim::cas::trace;

  auto trX = trace(X);

  // --- constant_add: wrapper(a) + wrapper(b) → wrapper(a+b) ---
  // Non-numeric scalar wrappers should merge in scalar domain
  EXPECT_SAME_PRINT(trX + x + y, x + y + trX);

  // --- constant_sub: wrapper(a) - wrapper(b) → wrapper(a-b) ---
  EXPECT_SAME_PRINT(x - y + trX, trX + x - y);

  // --- n_ary_add scanning: (trX + wrapper(x)) + wrapper(y) merges wrappers ---
  EXPECT_SAME_PRINT((trX + x) + y, x + y + trX);

  // --- n_ary_sub scanning: (trX + wrapper(x)) - wrapper(y) merges wrappers ---
  EXPECT_SAME_PRINT((trX + x) - y, x - y + trX);

  // --- wrapper cancellation via scalar sub ---
  EXPECT_PRINT(trX + x - x, "tr(X)");

  // --- constant_mul: wrapper(a) * wrapper(b) → wrapper(a*b) ---
  EXPECT_SAME_PRINT((x * trX) * (y * trX), numsim::cas::pow(trX, _2) * x * y);
}

// ---------- trace() simplification ----------
TYPED_TEST(TensorToScalarExpressionTest, TensorToScalar_TraceSimplification) {
  auto &X = this->X;
  auto &x = this->x;
  auto &_2 = this->_2;
  auto &Zero = this->_Zero;
  auto &One = this->_One;

  using numsim::cas::trace;

  // trace(0) → 0
  EXPECT_PRINT(trace(Zero), "0");

  // trace(I) → dim
  EXPECT_PRINT(trace(One), std::to_string(TestFixture::Dim));

  // trace(s*A) → s*trace(A)
  EXPECT_PRINT(trace(_2 * X), "2*tr(X)");
  EXPECT_PRINT(trace(x * X), "x*tr(X)");

  // normal case unchanged
  EXPECT_PRINT(trace(X), "tr(X)");
}

// ---------- det() simplification ----------
TYPED_TEST(TensorToScalarExpressionTest, TensorToScalar_DetSimplification) {
  auto &X = this->X;
  auto &Zero = this->_Zero;
  auto &One = this->_One;

  using numsim::cas::det;

  // det(0) → 0
  EXPECT_PRINT(det(Zero), "0");

  // det(I) → 1
  EXPECT_PRINT(det(One), "1");

  // normal case unchanged
  EXPECT_PRINT(det(X), "det(X)");
}

// ---------- norm() simplification ----------
TYPED_TEST(TensorToScalarExpressionTest, TensorToScalar_NormSimplification) {
  auto &X = this->X;
  auto &Zero = this->_Zero;

  using numsim::cas::norm;

  // norm(0) → 0
  EXPECT_PRINT(norm(Zero), "0");

  // normal case unchanged
  EXPECT_PRINT(norm(X), "norm(X)");
}

// ---------- trace() linearity ----------
TYPED_TEST(TensorToScalarExpressionTest, TensorToScalar_TraceLinearity) {
  auto &X = this->X;
  auto &Y = this->Y;
  auto &Z = this->Z;
  auto &_2 = this->_2;

  using numsim::cas::trace;

  // trace(A+B) → trace(A) + trace(B)
  EXPECT_SAME_PRINT(trace(X + Y), trace(X) + trace(Y));

  // trace(A+B+C) → trace(A) + trace(B) + trace(C)
  EXPECT_SAME_PRINT(trace(X + Y + Z), trace(X) + trace(Y) + trace(Z));

  // trace(2*A + B) → 2*trace(A) + trace(B) (scalar pull-through composes)
  EXPECT_SAME_PRINT(trace(_2 * X + Y), _2 * trace(X) + trace(Y));
}

// ---------- det() scaling ----------
TYPED_TEST(TensorToScalarExpressionTest, TensorToScalar_DetScaling) {
  auto &X = this->X;
  auto &x = this->x;
  auto &_2 = this->_2;

  using numsim::cas::det;

  constexpr auto Dim = TestFixture::Dim;

  // det(2*A) → pow(2,dim) * det(A)
  if constexpr (Dim == 1) {
    EXPECT_PRINT(det(_2 * X), "2*det(X)");
  } else {
    EXPECT_PRINT(det(_2 * X), "pow(2," + std::to_string(Dim) + ")*det(X)");
  }

  // det(x*A) → pow(x, dim) * det(A)
  if constexpr (Dim == 1) {
    EXPECT_PRINT(det(x * X), "x*det(X)");
  } else {
    EXPECT_PRINT(det(x * X), "pow(x," + std::to_string(Dim) + ")*det(X)");
  }
}

// ---------- det() multiplicativity ----------
TYPED_TEST(TensorToScalarExpressionTest, TensorToScalar_DetMultiplicativity) {
  auto &X = this->X;
  auto &Y = this->Y;

  using numsim::cas::det;

  // det(A*B) → det(A)*det(B)
  EXPECT_SAME_PRINT(det(X * Y), det(X) * det(Y));
}

// ---------- norm() scaling ----------
TYPED_TEST(TensorToScalarExpressionTest, TensorToScalar_NormScaling) {
  auto &X = this->X;
  auto &x = this->x;
  auto &_2 = this->_2;

  using numsim::cas::norm;

  // norm(2*A) → 2*norm(A)  (abs(2)=2 since 2 is positive)
  EXPECT_PRINT(norm(_2 * X), "2*norm(X)");

  // norm(x*A) → abs(x)*norm(A)
  EXPECT_PRINT(norm(x * X), "abs(x)*norm(X)");
}

// ---------- exp() simplification ----------
TYPED_TEST(TensorToScalarExpressionTest, TensorToScalar_ExpSimplification) {
  auto &X = this->X;

  using numsim::cas::exp;
  using numsim::cas::log;
  using numsim::cas::trace;

  auto trX = trace(X);
  auto t2s_zero =
      numsim::cas::make_expression<numsim::cas::tensor_to_scalar_zero>();

  // exp(0) → 1
  EXPECT_PRINT(exp(t2s_zero), "1");

  // exp(log(tr(X))) → tr(X) (inverse pair)
  EXPECT_PRINT(exp(log(trX)), "tr(X)");

  // normal case unchanged
  EXPECT_PRINT(exp(trX), "exp(tr(X))");

  // exp(tr(X)) + exp(tr(X)) → 2*exp(tr(X))
  EXPECT_PRINT(exp(trX) + exp(trX), "2*exp(tr(X))");
}

// ---------- sqrt() simplification ----------
TYPED_TEST(TensorToScalarExpressionTest, TensorToScalar_SqrtSimplification) {
  auto &X = this->X;

  using numsim::cas::sqrt;
  using numsim::cas::trace;

  auto trX = trace(X);
  auto t2s_zero =
      numsim::cas::make_expression<numsim::cas::tensor_to_scalar_zero>();
  auto t2s_one =
      numsim::cas::make_expression<numsim::cas::tensor_to_scalar_one>();

  // sqrt(0) → 0
  EXPECT_PRINT(sqrt(t2s_zero), "0");

  // sqrt(1) → 1
  EXPECT_PRINT(sqrt(t2s_one), "1");

  // normal case unchanged
  EXPECT_PRINT(sqrt(trX), "sqrt(tr(X))");
}

// ---------- exp(a)^n → exp(n*a) ----------
TYPED_TEST(TensorToScalarExpressionTest, TensorToScalar_ExpPowSimplification) {
  auto &X = this->X;
  auto &_2 = this->_2;

  using numsim::cas::exp;
  using numsim::cas::trace;

  auto trX = trace(X);

  EXPECT_PRINT(numsim::cas::pow(exp(trX), _2), "exp(2*tr(X))");
}

#endif // TENSORTOSCALAREXPRESSIONTEST_H
