#ifndef TENSORSPACEPROPAGATIONTEST_H
#define TENSORSPACEPROPAGATIONTEST_H

#include "cas_test_helpers.h"
#include <gtest/gtest.h>

#include <numsim_cas/core/diff.h>
#include <numsim_cas/numsim_cas.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_diff.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_std.h>

namespace numsim::cas {

class TensorSpacePropagationTest : public ::testing::Test {
protected:
  static constexpr std::size_t dim = 3;

  using tensor_t = expression_holder<tensor_expression>;
  using t2s_t = expression_holder<tensor_to_scalar_expression>;
  using scalar_t = expression_holder<scalar_expression>;

  TensorSpacePropagationTest() {
    // C is symmetric
    C = std::get<0>(make_tensor_variable(std::tuple{"C", dim, 2}));
    assume_symmetric(C);

    // D is deviatoric
    D = std::get<0>(make_tensor_variable(std::tuple{"D", dim, 2}));
    assume_deviatoric(D);

    // W is skew
    W = std::get<0>(make_tensor_variable(std::tuple{"W", dim, 2}));
    assume_skew(W);

    // V is volumetric
    V = std::get<0>(make_tensor_variable(std::tuple{"V", dim, 2}));
    assume_volumetric(V);

    // X has no assumption
    X = std::get<0>(make_tensor_variable(std::tuple{"X", dim, 2}));

    I = make_expression<kronecker_delta>(dim);

    std::tie(_2, _3) = make_scalar_constant(2, 3);
    std::tie(x) = make_scalar_variable("x");
  }

  tensor_t C, D, W, V, X;
  tensor_t I;
  scalar_t _2, _3, x;
};

// ═══════════════════════════════════════════════════════════════════════════════
// Space propagation through scalar_mul
// ═══════════════════════════════════════════════════════════════════════════════

TEST_F(TensorSpacePropagationTest, SymOfScalarMul) {
  // sym(2*C) → 2*C when C is symmetric
  EXPECT_PRINT(sym(2 * C), "2*C");
}

TEST_F(TensorSpacePropagationTest, SkewOfScalarMul) {
  // skew(2*C) → 0 when C is symmetric
  EXPECT_PRINT(skew(2 * C), "0");
}

TEST_F(TensorSpacePropagationTest, DevOfScalarMulDeviatoric) {
  // dev(2*D) → 2*D when D is deviatoric
  EXPECT_PRINT(dev(2 * D), "2*D");
}

TEST_F(TensorSpacePropagationTest, VolOfScalarMulDeviatoric) {
  // vol(2*D) → 0 when D is deviatoric
  EXPECT_PRINT(vol(2 * D), "0");
}

TEST_F(TensorSpacePropagationTest, SymOfScalarMulSymbolicCoeff) {
  // sym(x*C) → x*C when C is symmetric
  EXPECT_PRINT(sym(x * C), "x*C");
}

// ═══════════════════════════════════════════════════════════════════════════════
// Space propagation through negation
// ═══════════════════════════════════════════════════════════════════════════════

TEST_F(TensorSpacePropagationTest, SymOfNeg) {
  // sym(-C) → -C when C is symmetric
  EXPECT_PRINT(sym(-C), "-C");
}

TEST_F(TensorSpacePropagationTest, SkewOfNeg) {
  // skew(-C) → 0 when C is symmetric
  EXPECT_PRINT(skew(-C), "0");
}

TEST_F(TensorSpacePropagationTest, DevOfNegDeviatoric) {
  // dev(-D) → -D when D is deviatoric
  EXPECT_PRINT(dev(-D), "-D");
}

// ═══════════════════════════════════════════════════════════════════════════════
// Space propagation through pow
// ═══════════════════════════════════════════════════════════════════════════════

TEST_F(TensorSpacePropagationTest, SymOfPow) {
  // sym(pow(C,2)) → pow(C,2) when C is symmetric
  EXPECT_PRINT(sym(pow(C, 2)), "pow(C,2)");
}

TEST_F(TensorSpacePropagationTest, SkewOfPow) {
  // skew(pow(C,2)) → 0 when C is symmetric
  EXPECT_PRINT(skew(pow(C, 2)), "0");
}

TEST_F(TensorSpacePropagationTest, PowSkewDoesNotPropagateSkew) {
  // pow(W,2) should NOT have Skew space — W^2 is symmetric, not skew
  auto p = pow(W, 2);
  EXPECT_FALSE(is_skew(p))
      << "pow(W,2) must not be skew — (W^2)^T = W^T W^T = (-W)(-W) = W^2";
}

TEST_F(TensorSpacePropagationTest, PowDevDoesNotPropagateDeviatoric) {
  // pow(D,2) should NOT have Deviatoric space — tr(D^2) = D:D ≠ 0
  auto p = pow(D, 2);
  EXPECT_FALSE(is_deviatoric(p))
      << "pow(D,2) must not be deviatoric — tr(D^2) ≠ 0 in general";
}

TEST_F(TensorSpacePropagationTest, PowDevDowngradesToSymmetric) {
  // pow(D,2) should be Symmetric — D is symmetric, so D^n is too
  auto p = pow(D, 2);
  EXPECT_TRUE(is_symmetric(p))
      << "pow(D,2) should be symmetric — (D^n)^T = (D^T)^n = D^n";
}

TEST_F(TensorSpacePropagationTest, SymOfPowDev) {
  // sym(pow(D,2)) → pow(D,2) because D^2 is symmetric
  EXPECT_PRINT(sym(pow(D, 2)), "pow(D,2)");
}

TEST_F(TensorSpacePropagationTest, PowVolPropagatesVolumetric) {
  // pow(V,2) stays volumetric: V = (tr(V)/d)*I, V^n = ((tr(V)/d))^n * I
  auto p = pow(V, 2);
  EXPECT_TRUE(is_volumetric(p))
      << "pow(V,2) should be volumetric — V^n is still proportional to I";
}

// ═══════════════════════════════════════════════════════════════════════════════
// Space propagation through inv
// ═══════════════════════════════════════════════════════════════════════════════

TEST_F(TensorSpacePropagationTest, SymOfInv) {
  // sym(inv(C)) → inv(C) when C is symmetric
  EXPECT_PRINT(sym(inv(C)), "inv(C)");
}

TEST_F(TensorSpacePropagationTest, SkewOfInv) {
  // skew(inv(C)) → 0 when C is symmetric
  EXPECT_PRINT(skew(inv(C)), "0");
}

TEST_F(TensorSpacePropagationTest, InvDevDoesNotPropagateDeviatoric) {
  // inv(D) should NOT have Deviatoric space — tr(D^{-1}) ≠ 0 in general
  auto i = inv(D);
  EXPECT_FALSE(is_deviatoric(i))
      << "inv(D) must not be deviatoric — tr(D^{-1}) ≠ 0 in general";
}

TEST_F(TensorSpacePropagationTest, InvDevDowngradesToSymmetric) {
  // inv(D) should still be Symmetric — D is symmetric, inv preserves that
  auto i = inv(D);
  EXPECT_TRUE(is_symmetric(i))
      << "inv(D) should be symmetric — (D^{-1})^T = (D^T)^{-1} = D^{-1}";
}

TEST_F(TensorSpacePropagationTest, SymOfInvDev) {
  // sym(inv(D)) → inv(D) because inv(D) is symmetric
  EXPECT_PRINT(sym(inv(D)), "inv(D)");
}

TEST_F(TensorSpacePropagationTest, InvSkewPreservesSkew) {
  // inv(W) preserves Skew: (W^{-1})^T = (W^T)^{-1} = (-W)^{-1} = -W^{-1}
  auto i = inv(W);
  EXPECT_TRUE(is_skew(i)) << "inv(W) should be skew — (W^{-1})^T = -W^{-1}";
}

// ═══════════════════════════════════════════════════════════════════════════════
// Space propagation through addition
// ═══════════════════════════════════════════════════════════════════════════════

TEST_F(TensorSpacePropagationTest, SymOfCPlusC) {
  // sym(C+C) → 2*C when C is symmetric (via T+T→2*T)
  EXPECT_PRINT(sym(C + C), "2*C");
}

TEST_F(TensorSpacePropagationTest, SymOfSymPlusVol) {
  // C+V has join Sym; sym(C+V) → C+V
  auto sum = C + V;
  EXPECT_PRINT(sym(sum), ::testcas::S(sum));
}

TEST_F(TensorSpacePropagationTest, SkewOfSymPlusVol) {
  // C+V has join Sym; skew(C+V) → 0
  EXPECT_PRINT(skew(C + V), "0");
}

// ═══════════════════════════════════════════════════════════════════════════════
// No-propagation: expressions without space assumptions stay unaffected
// ═══════════════════════════════════════════════════════════════════════════════

TEST_F(TensorSpacePropagationTest, NoAssumptionNoEarlyReturn) {
  // sym(X) should NOT simplify when X has no assumption
  auto result = sym(X);
  EXPECT_FALSE(is_same<tensor>(result))
      << "sym(X) should not return X when X has no assumption";
}

// ═══════════════════════════════════════════════════════════════════════════════
// Kronecker delta special cases
// ═══════════════════════════════════════════════════════════════════════════════

TEST_F(TensorSpacePropagationTest, SymOfI) {
  // sym(I) → I
  EXPECT_PRINT(sym(I), "I");
}

TEST_F(TensorSpacePropagationTest, VolOfI) {
  // vol(I) → I  (because vol(I) = tr(I)/d * I = d/d * I = I)
  EXPECT_PRINT(vol(I), "I");
}

TEST_F(TensorSpacePropagationTest, DevOfI) {
  // dev(I) → 0  (because dev = sym - vol, so dev(I) = I - I = 0)
  EXPECT_PRINT(dev(I), "0");
}

TEST_F(TensorSpacePropagationTest, SkewOfI) {
  // skew(I) → 0  (I is symmetric, so skew part is zero)
  EXPECT_PRINT(skew(I), "0");
}

// ═══════════════════════════════════════════════════════════════════════════════
// Inner-product normalization: X:{1,2} P:{1,2} → sym/dev/vol/skew(X)
// ═══════════════════════════════════════════════════════════════════════════════

TEST_F(TensorSpacePropagationTest, ReversedProjSymNormalization) {
  // inner_product(X, {1,2}, P_sym, {1,2}) → sym(X)
  auto result = inner_product(X, sequence{1, 2}, P_sym(dim), sequence{1, 2});
  EXPECT_PRINT(result, ::testcas::S(sym(X)));
}

TEST_F(TensorSpacePropagationTest, ReversedProjDevNormalization) {
  // inner_product(X, {1,2}, P_dev, {1,2}) → dev(X)
  auto result = inner_product(X, sequence{1, 2}, P_devi(dim), sequence{1, 2});
  EXPECT_PRINT(result, ::testcas::S(dev(X)));
}

TEST_F(TensorSpacePropagationTest, ReversedProjVolNormalization) {
  // inner_product(X, {1,2}, P_vol, {1,2}) → vol(X)
  auto result = inner_product(X, sequence{1, 2}, P_vol(dim), sequence{1, 2});
  EXPECT_PRINT(result, ::testcas::S(vol(X)));
}

TEST_F(TensorSpacePropagationTest, ReversedProjSkewNormalization) {
  // inner_product(X, {1,2}, P_skew, {1,2}) → skew(X)
  auto result = inner_product(X, sequence{1, 2}, P_skew(dim), sequence{1, 2});
  EXPECT_PRINT(result, ::testcas::S(skew(X)));
}

TEST_F(TensorSpacePropagationTest, ReversedProjSymOnSymmetric) {
  // inner_product(C, {1,2}, P_sym, {1,2}) → C when C is symmetric
  // (normalized to sym(C), then space check returns C)
  auto result = inner_product(C, sequence{1, 2}, P_sym(dim), sequence{1, 2});
  EXPECT_PRINT(result, "C");
}

TEST_F(TensorSpacePropagationTest, ReversedProjOnKroneckerDelta) {
  // inner_product(I, {1,2}, P_sym, {1,2}) → sym(I) → I
  auto result = inner_product(I, sequence{1, 2}, P_sym(dim), sequence{1, 2});
  EXPECT_PRINT(result, "I");
}

TEST_F(TensorSpacePropagationTest, NormalProjNotAffected) {
  // inner_product(P_sym, {3,4}, X, {1,2}) is the normal form — should stay
  auto result = inner_product(P_sym(dim), sequence{3, 4}, X, sequence{1, 2});
  EXPECT_PRINT(result, "sym(X)");
}

// ═══════════════════════════════════════════════════════════════════════════════
// Differentiation with space assumptions
// ═══════════════════════════════════════════════════════════════════════════════

TEST_F(TensorSpacePropagationTest, DiffSymmetricSelf) {
  // dC/dC → P_sym when C is symmetric
  auto d = diff(C, C);
  EXPECT_TRUE(is_same<tensor_projector>(d))
      << "Expected tensor_projector, got: " << to_string(d);
  EXPECT_PRINT(d, "P_sym{4}");
}

TEST_F(TensorSpacePropagationTest, DiffDeviatoric) {
  // dD/dD → P_devi when D is deviatoric
  auto d = diff(D, D);
  EXPECT_TRUE(is_same<tensor_projector>(d))
      << "Expected tensor_projector, got: " << to_string(d);
  EXPECT_PRINT(d, "P_dev{4}");
}

TEST_F(TensorSpacePropagationTest, DiffSkew) {
  // dW/dW → P_skew when W is skew
  auto d = diff(W, W);
  EXPECT_TRUE(is_same<tensor_projector>(d))
      << "Expected tensor_projector, got: " << to_string(d);
  EXPECT_PRINT(d, "P_skew{4}");
}

TEST_F(TensorSpacePropagationTest, DiffVolumetric) {
  // dV/dV → P_vol when V is volumetric
  auto d = diff(V, V);
  EXPECT_TRUE(is_same<tensor_projector>(d))
      << "Expected tensor_projector, got: " << to_string(d);
  EXPECT_PRINT(d, "P_vol{4}");
}

TEST_F(TensorSpacePropagationTest, DiffNoAssumptionGivesIdentity) {
  // dX/dX → I{4} when X has no assumption
  auto d = diff(X, X);
  EXPECT_TRUE(is_same<identity_tensor>(d))
      << "Expected identity_tensor, got: " << to_string(d);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Cross-domain differentiation with assumptions (chain rule)
// ═══════════════════════════════════════════════════════════════════════════════

TEST_F(TensorSpacePropagationTest, DiffTraceSymmetric) {
  // d(tr(C))/dC → I when C is symmetric
  // Chain rule: I:{1,2} P_sym:{1,2} → sym(I) → I
  auto d = diff(trace(C), C);
  EXPECT_PRINT(d, "I");
}

TEST_F(TensorSpacePropagationTest, DiffDotSymmetric) {
  // d(C:C)/dC → 2*C when C is symmetric
  auto d = diff(dot(C), C);
  EXPECT_PRINT(d, "2*C");
}

TEST_F(TensorSpacePropagationTest, DiffTraceSquaredSymmetric) {
  // d(tr(C)^2)/dC → 2*tr(C)*I when C is symmetric
  auto trC = trace(C);
  auto d = diff(pow(trC, 2), C);
  auto s = ::testcas::S(d);
  EXPECT_TRUE(s.find("tr") != std::string::npos &&
              s.find("I") != std::string::npos)
      << "Expected 2*tr(C)*I, got: " << s;
}

TEST_F(TensorSpacePropagationTest, DiffScalarMulSymmetric) {
  // d(2*C)/dC → 2*P_sym when C is symmetric
  auto d = diff(_2 * C, C);
  EXPECT_PRINT(d, "2*P_sym{4}");
}

TEST_F(TensorSpacePropagationTest, DiffNegSymmetric) {
  // d(-C)/dC → -P_sym when C is symmetric
  auto d = diff(-C, C);
  EXPECT_PRINT(d, "-P_sym{4}");
}

TEST_F(TensorSpacePropagationTest, DiffAddSymmetric) {
  // d(C+C)/dC → 2*P_sym when C is symmetric
  auto d = diff(C + C, C);
  EXPECT_PRINT(d, "2*P_sym{4}");
}

} // namespace numsim::cas

#endif // TENSORSPACEPROPAGATIONTEST_H
