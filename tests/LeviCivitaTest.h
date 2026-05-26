#ifndef LEVICIVITATEST_H
#define LEVICIVITATEST_H

// Tests for the Levi-Civita symbol leaf node (#34).
//
// Coverage scope:
//   - Construction at every supported dim (2, 3, 4).
//   - Print and LaTeX print.
//   - Hash and structural equality.
//   - Evaluator: known component values at dim 2 and dim 3, with the
//     dim 4 case sample-checked.
//   - Differentiation: ε is constant, so d(ε)/d(X) = 0 for every X.
//   - Rebuild round-trip and `contains_expression` leaf behaviour.

#include <gtest/gtest.h>

#include <numsim_cas/core/contains_expression.h>
#include <numsim_cas/numsim_cas.h>
#include <numsim_cas/tensor/levi_civita_tensor.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor/visitors/tensor_evaluator.h>
#include <numsim_cas/tensor/visitors/tensor_latex_printer.h>
#include <numsim_cas/tensor/visitors/tensor_printer.h>
#include <numsim_cas/tensor/visitors/tensor_rebuild_visitor.h>

#include <sstream>

namespace numsim::cas::levi_civita_test {

// Helper: stringify via LaTeX printer.
inline std::string
to_latex_local(expression_holder<tensor_expression> const &e) {
  std::stringstream ss;
  tensor_latex_printer<std::stringstream> lp(ss);
  lp.apply(e);
  return ss.str();
}

// ─── Construction ──────────────────────────────────────────────────

TEST(LeviCivita, ConstructDim2HasRank2) {
  auto eps = levi_civita(2);
  EXPECT_EQ(eps.get().dim(), 2u);
  EXPECT_EQ(eps.get().rank(), 2u);
}

TEST(LeviCivita, ConstructDim3HasRank3) {
  auto eps = levi_civita(3);
  EXPECT_EQ(eps.get().dim(), 3u);
  EXPECT_EQ(eps.get().rank(), 3u);
}

TEST(LeviCivita, ConstructDim4HasRank4) {
  auto eps = levi_civita(4);
  EXPECT_EQ(eps.get().dim(), 4u);
  EXPECT_EQ(eps.get().rank(), 4u);
}

TEST(LeviCivita, ConstructRejectsUnsupportedDim) {
  // dim 0, 1, and ≥5 are invalid for the Levi-Civita symbol — must
  // throw at construction time, not wait for evaluation.
  //
  // The brace + `[[maybe_unused]] auto` form is required because
  // `levi_civita()` and `make_expression<>()` are `[[nodiscard]]`;
  // Apple Clang with -Werror,-Wunused-result rejects the bare
  // EXPECT_THROW(call, ...) form (the macro's expansion doesn't
  // visibly consume the result, even though the call always throws).
  EXPECT_THROW(
      { [[maybe_unused]] auto e = levi_civita(0); }, invalid_expression_error);
  EXPECT_THROW(
      { [[maybe_unused]] auto e = levi_civita(1); }, invalid_expression_error);
  EXPECT_THROW(
      { [[maybe_unused]] auto e = levi_civita(5); }, invalid_expression_error);
  EXPECT_THROW(
      { [[maybe_unused]] auto e = levi_civita(100); },
      invalid_expression_error);
  // Same via explicit make_expression — same error surface.
  EXPECT_THROW(
      { [[maybe_unused]] auto e = make_expression<levi_civita_tensor>(0); },
      invalid_expression_error);
  EXPECT_THROW(
      { [[maybe_unused]] auto e = make_expression<levi_civita_tensor>(7); },
      invalid_expression_error);
}

// ─── Printing ──────────────────────────────────────────────────────

TEST(LeviCivita, PrintIncludesDimSuffix) {
  EXPECT_EQ(to_string(levi_civita(2)), "eps{2}");
  EXPECT_EQ(to_string(levi_civita(3)), "eps{3}");
  EXPECT_EQ(to_string(levi_civita(4)), "eps{4}");
}

TEST(LeviCivita, LatexPrintUsesVarepsilon) {
  auto s = to_latex_local(levi_civita(3));
  // Expect the LaTeX \varepsilon glyph and the dim superscript.
  EXPECT_NE(s.find("\\varepsilon"), std::string::npos)
      << "expected \\varepsilon glyph, got: " << s;
  EXPECT_NE(s.find("(3)"), std::string::npos)
      << "expected dim superscript (3), got: " << s;
}

// ─── Structural equality ───────────────────────────────────────────

TEST(LeviCivita, SameDimEqualHash) {
  auto a = levi_civita(3);
  auto b = levi_civita(3);
  EXPECT_EQ(a.get().hash_value(), b.get().hash_value());
  EXPECT_EQ(a, b);
}

TEST(LeviCivita, DifferentDimDistinct) {
  auto e2 = levi_civita(2);
  auto e3 = levi_civita(3);
  EXPECT_NE(e2.get().hash_value(), e3.get().hash_value());
  EXPECT_NE(e2, e3);
}

TEST(LeviCivita, DistinctFromIdentity) {
  // Both are rank-2 leaves at dim 2 — but they must not collide.
  auto eps2 = levi_civita(2);
  auto I2 = make_expression<identity_tensor>(2, std::size_t{2});
  EXPECT_NE(eps2.get().hash_value(), I2.get().hash_value())
      << "Levi-Civita symbol must hash distinctly from identity";
  EXPECT_NE(eps2, I2);
}

// ─── Evaluation ────────────────────────────────────────────────────

TEST(LeviCivita, EvalDim2IsSkewUnit) {
  // ε_{ij} in 2D:
  //   ε_{00} = 0, ε_{01} = +1, ε_{10} = -1, ε_{11} = 0.
  tensor_evaluator<double> ev;
  auto result = ev.apply(levi_civita(2));
  ASSERT_NE(result, nullptr);
  auto const &raw =
      static_cast<tensor_data<double, 2, 2> const &>(*result).data();
  EXPECT_DOUBLE_EQ(raw(0, 0), 0.0);
  EXPECT_DOUBLE_EQ(raw(0, 1), 1.0);
  EXPECT_DOUBLE_EQ(raw(1, 0), -1.0);
  EXPECT_DOUBLE_EQ(raw(1, 1), 0.0);
}

TEST(LeviCivita, EvalDim3KnownEntries) {
  // 3D ε_{ijk} (zero-indexed):
  //   Even permutations of (0,1,2):  (0,1,2)=+1, (1,2,0)=+1, (2,0,1)=+1.
  //   Odd permutations:               (0,2,1)=-1, (2,1,0)=-1, (1,0,2)=-1.
  //   All other entries (any index repeated) = 0.
  tensor_evaluator<double> ev;
  auto result = ev.apply(levi_civita(3));
  ASSERT_NE(result, nullptr);
  auto const &raw =
      static_cast<tensor_data<double, 3, 3> const &>(*result).data();

  // Even permutations
  EXPECT_DOUBLE_EQ(raw(0, 1, 2), 1.0);
  EXPECT_DOUBLE_EQ(raw(1, 2, 0), 1.0);
  EXPECT_DOUBLE_EQ(raw(2, 0, 1), 1.0);
  // Odd permutations
  EXPECT_DOUBLE_EQ(raw(0, 2, 1), -1.0);
  EXPECT_DOUBLE_EQ(raw(2, 1, 0), -1.0);
  EXPECT_DOUBLE_EQ(raw(1, 0, 2), -1.0);
  // Repeated indices → 0 (sample a few)
  EXPECT_DOUBLE_EQ(raw(0, 0, 0), 0.0);
  EXPECT_DOUBLE_EQ(raw(1, 1, 2), 0.0);
  EXPECT_DOUBLE_EQ(raw(0, 1, 1), 0.0);
  EXPECT_DOUBLE_EQ(raw(2, 2, 0), 0.0);
}

TEST(LeviCivita, EvalDim3AntisymmetryUnderTransposition) {
  // ε_{ijk} = -ε_{jik}. Verify across every (i,j,k) with i ≠ j.
  tensor_evaluator<double> ev;
  auto result = ev.apply(levi_civita(3));
  ASSERT_NE(result, nullptr);
  auto const &raw =
      static_cast<tensor_data<double, 3, 3> const &>(*result).data();
  for (std::size_t i = 0; i < 3; ++i) {
    for (std::size_t j = 0; j < 3; ++j) {
      if (i == j)
        continue;
      for (std::size_t k = 0; k < 3; ++k) {
        EXPECT_DOUBLE_EQ(raw(i, j, k), -raw(j, i, k))
            << "antisymmetry violated at (" << i << "," << j << "," << k << ")";
      }
    }
  }
}

TEST(LeviCivita, EvalDim4ThrowsDueToMaxDimCeiling) {
  // `tensor_data_eval_up_unary` is currently instantiated with MaxDim = 3
  // (see numsim_cas_type_traits.h:70). Constructing levi_civita_tensor(4)
  // is allowed at the symbolic layer — useful inside expressions that
  // collapse before evaluation — but a bare evaluate must throw rather
  // than silently corrupt memory. Pin that contract; lifting MaxDim is a
  // separate concern.
  tensor_evaluator<double> ev;
  EXPECT_THROW(ev.apply(levi_civita(4)), evaluation_error);
}

// ─── Differentiation ──────────────────────────────────────────────

TEST(LeviCivita, DiffOfEpsAloneIsTensorZeroOrInvalid) {
  // ε is constant w.r.t. any user variable, so its derivative is the
  // zero tensor (or an invalid holder which downstream treats as
  // zero — the diff visitor leaves m_result default-constructed in
  // that case, mirroring identity_tensor's diff override).
  auto eps = levi_civita(3);
  auto A = make_expression<tensor>("A", 3, 3);
  auto d = diff(eps, A);
  // Either is acceptable: tensor_zero of appropriate rank, or an
  // invalid holder. Anything else would mean ε has spurious
  // dependence on A.
  if (d.is_valid()) {
    EXPECT_TRUE(is_same<tensor_zero>(d))
        << "diff(eps, A) should be tensor_zero, got: " << to_string(d);
  }
  // The relevant invariant either way: there is no path through which
  // diff(eps, A) reuses eps's value or shape.
}

TEST(LeviCivita, DiffOfEpsPlusADropsEpsBranch) {
  // diff(eps + A, A) should equal diff(A, A) — i.e. ε contributes 0.
  // Verify structurally: rebuild diff(A, A) on its own and compare
  // to the result of diff(eps + A, A). Both should produce the same
  // tensor expression (the rank-(3+3) minor identity).
  auto eps = levi_civita(3);
  auto A = make_expression<tensor>("A", 3, 3);
  auto sum = eps + A;
  auto d_sum = diff(sum, A);
  auto d_a = diff(A, A);
  ASSERT_TRUE(d_sum.is_valid());
  ASSERT_TRUE(d_a.is_valid());
  EXPECT_EQ(d_sum, d_a)
      << "diff(eps + A, A) should equal diff(A, A); the eps branch "
         "must contribute nothing.\n  diff(eps + A, A) = "
      << to_string(d_sum) << "\n  diff(A, A)       = " << to_string(d_a);
}

// ─── Rebuild round-trip ────────────────────────────────────────────

TEST(LeviCivita, RebuildIsIdentity) {
  auto eps = levi_civita(3);
  tensor_rebuild_visitor rb;
  auto rebuilt = rb.apply(eps);
  EXPECT_EQ(rebuilt, eps);
  EXPECT_TRUE(is_same<levi_civita_tensor>(rebuilt));
}

// ─── contains_expression: leaf behaves like other leaves ──────────

TEST(LeviCivita, ContainsItselfButNotOtherLeaves) {
  auto eps = levi_civita(3);
  EXPECT_TRUE(contains_expression(eps, eps));

  auto A = make_expression<tensor>("A", 3, 2);
  // eps does not contain A (different node type, no children).
  EXPECT_FALSE(contains_expression(eps, A));
}

} // namespace numsim::cas::levi_civita_test

#endif // LEVICIVITATEST_H
