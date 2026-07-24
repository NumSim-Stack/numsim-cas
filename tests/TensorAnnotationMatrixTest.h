#ifndef TENSORANNOTATIONMATRIXTEST_H
#define TENSORANNOTATIONMATRIXTEST_H

#include "cas_test_helpers.h"
#include <gtest/gtest.h>

#include <numsim_cas/numsim_cas.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_std.h>

#include <functional>
#include <vector>

// ════════════════════════════════════════════════════════════════════════════
// #111 — tensor space-annotation preservation matrix.
//
// Consolidates and extends the per-case tests in TensorSpacePropagationTest.h
// into a systematic (input space × composition node) walk. Each cell is
// checked against the observable annotation contract — the four query
// predicates is_symmetric / is_skew / is_volumetric / is_deviatoric (all
// backed by classify_space, see tensor_assume.h) — and failures report the
// (input, node) coordinate.
//
// Expected values are the *mathematically provable* annotation, encoded as a
// tri-state so we lock in correct facts without pinning conservative gaps:
//   Yes  — annotation must be present (proven true; code derives it).
//   No   — annotation must be absent (proven false; a false positive here
//          would be a real bug, since it drives unsound simplifications).
//   Any  — not asserted. Used only where the annotation is mathematically
//          derivable but the implementation conservatively drops it (dropping
//          is always sound; a future PR that derives it should flip Any→Yes).
//
// Space tags exercised: Skew, Symmetric, Symmetric×Volumetric,
// Symmetric×Deviatoric, and no-annotation (control). HarmonicTag is out of
// scope here: there is no assume_harmonic() helper, so a harmonic input can
// only be built via raw set_space(), which the propagation paths do not
// specially track — deliberately unsupported.
// ════════════════════════════════════════════════════════════════════════════

namespace numsim::cas {
namespace annotation_matrix {

enum class Tri { No, Yes, Any };

struct Cell {
  Tri sym, skew, vol, dev;
};

inline void check_cell(Cell c, expression_holder<tensor_expression> const &e) {
  auto chk = [&](Tri t, bool actual, char const *which) {
    if (t == Tri::Yes) {
      EXPECT_TRUE(actual) << which << " should be present";
    } else if (t == Tri::No) {
      EXPECT_FALSE(actual) << which << " should be absent";
    }
  };
  chk(c.sym, is_symmetric(e), "is_symmetric");
  chk(c.skew, is_skew(e), "is_skew");
  chk(c.vol, is_volumetric(e), "is_volumetric");
  chk(c.dev, is_deviatoric(e), "is_deviatoric");
}

} // namespace annotation_matrix

class TensorAnnotationMatrixTest : public ::testing::Test {
protected:
  using tensor_t = expression_holder<tensor_expression>;
  using Tri = annotation_matrix::Tri;
  using Cell = annotation_matrix::Cell;

  static tensor_t mk(char const *n, std::size_t d) {
    return std::get<0>(make_tensor_variable(std::tuple{n, d, std::size_t{2}}));
  }

  tensor_t C, D, V, W, X;
  expression_holder<scalar_expression> two;

  TensorAnnotationMatrixTest() {
    C = mk("C", 3);
    assume_symmetric(C);
    D = mk("D", 3);
    assume_deviatoric(D);
    V = mk("V", 3);
    assume_volumetric(V);
    // Skew input at dim 4: an odd-dim skew tensor is singular, so inv(W)
    // would throw; dim 4 keeps the inv() cell exercisable.
    W = mk("W", 4);
    assume_skew(W);
    X = mk("X", 3); // no annotation (control)
    two = make_scalar_constant(2);
  }
};

// The unary / self-binary algebraic nodes, in a fixed order shared by every
// input row's expectation table.
TEST_F(TensorAnnotationMatrixTest, PreservationMatrix) {
  using annotation_matrix::check_cell;
  constexpr Tri Y = Tri::Yes, N = Tri::No, A = Tri::Any;

  struct NodeDef {
    char const *name;
    std::function<tensor_t(tensor_t const &)> apply;
  };
  std::vector<NodeDef> nodes = {
      {"neg", [](tensor_t const &t) { return -t; }},
      {"trans", [](tensor_t const &t) { return trans(t); }},
      {"scalar_mul", [this](tensor_t const &t) { return two * t; }},
      {"pow2", [](tensor_t const &t) { return pow(t, 2); }},
      {"pow3", [](tensor_t const &t) { return pow(t, 3); }},
      {"add", [](tensor_t const &t) { return t + t; }},
      {"mul", [](tensor_t const &t) { return t * t; }},
      {"inv", [](tensor_t const &t) { return inv(t); }},
  };

  struct Row {
    char const *name;
    tensor_t input;
    std::vector<Cell> expected; // one per node, same order as `nodes`
  };

  // Column order:      neg          trans        scalar_mul   pow2
  //                    pow3         add          mul          inv
  std::vector<Row> rows = {
      // C symmetric — perm symmetry survives every algebraic node.
      {"C(sym)",
       C,
       {{Y, N, N, N},
        {Y, N, N, N},
        {Y, N, N, N},
        {Y, N, N, N},
        {Y, N, N, N},
        {Y, N, N, N},
        {Y, N, N, N},
        {Y, N, N, N}}},
      // D deviatoric — symmetry always survives; the trace-free property
      // survives sign/scale/add but is *downgraded* by pow/mul/inv
      // (tr(D^n), tr(D^{-1}) ≠ 0 in general), leaving plain symmetric.
      {"D(dev)",
       D,
       {{Y, N, N, Y},   // neg: -D still deviatoric
        {Y, N, N, Y},   // trans: D^T = D
        {Y, N, N, Y},   // scalar_mul: 2D deviatoric
        {Y, N, N, N},   // pow2: D^2 symmetric, tr≠0 → not dev
        {Y, N, N, N},   // pow3
        {Y, N, N, Y},   // add: 2D deviatoric
        {Y, N, N, N},   // mul: D·D = D^2, not dev
        {Y, N, N, N}}}, // inv: symmetric, tr(D^{-1})≠0 → not dev
      // V volumetric — V = c·I, so every node keeps it proportional to I.
      {"V(vol)",
       V,
       {{Y, N, Y, N},
        {Y, N, Y, N},
        {Y, N, Y, N},
        {Y, N, Y, N},
        {Y, N, Y, N},
        {Y, N, Y, N},
        {Y, N, Y, N},
        {Y, N, Y, N}}},
      // W skew (dim 4). Skew survives sign/scale/add/trans and inv (even
      // dim: (W^{-1})^T = -W^{-1}). Through pow/mul it is conservatively
      // dropped: W^2 is symmetric and W^3 is skew mathematically, but the
      // annotation is not currently derived (Any) — see #111 scenario 3.
      {"W(skew,d4)",
       W,
       {{N, Y, N, N},   // neg
        {N, Y, N, N},   // trans: W^T = -W
        {N, Y, N, N},   // scalar_mul
        {A, N, N, N},   // pow2: W^2 symmetric (not derived); never skew
        {N, A, N, N},   // pow3: W^3 skew (not derived); never symmetric
        {N, Y, N, N},   // add: 2W skew
        {A, N, N, N},   // mul: W·W = W^2 symmetric (not derived)
        {N, Y, N, N}}}, // inv: skew in even dim
      // X unannotated — nothing to propagate; every node stays annotation-free.
      {"X(none)",
       X,
       {{N, N, N, N},
        {N, N, N, N},
        {N, N, N, N},
        {N, N, N, N},
        {N, N, N, N},
        {N, N, N, N},
        {N, N, N, N},
        {N, N, N, N}}},
  };

  for (auto const &row : rows) {
    ASSERT_EQ(row.expected.size(), nodes.size())
        << "row " << row.name << " expectation count mismatch";
    for (std::size_t j = 0; j < nodes.size(); ++j) {
      SCOPED_TRACE(::testing::Message()
                   << "input=" << row.name << " node=" << nodes[j].name);
      check_cell(row.expected[j], nodes[j].apply(row.input));
    }
  }
}

// Scenario 4 — derivation: an annotation appears on a composite even though
// the operand carried none. trans(A) - A is skew-symmetric by construction.
TEST_F(TensorAnnotationMatrixTest, DerivedSkewFromTransMinusSelf) {
  auto A = mk("A", 3);
  EXPECT_TRUE(is_skew(trans(A) - A))
      << "trans(A) - A must carry the derived Skew annotation";
  EXPECT_TRUE(is_skew(trans(A) + (-A)))
      << "trans(A) + (-A) must mirror the sub form's Skew annotation";
}

// Product / cross-domain nodes are annotation-opaque: they never *falsely*
// claim skew/vol/dev (a false positive would drive unsound simplifications).
// Symmetry is left unasserted (Any) — some products are provably symmetric
// but the annotation is conservatively dropped.
TEST_F(TensorAnnotationMatrixTest,
       OpaqueProductNodesNeverClaimNonSymmetricSpaces) {
  using annotation_matrix::check_cell;
  constexpr Tri N = Tri::No, A = Tri::Any;
  auto B = mk("B", 3);
  std::vector<std::pair<char const *, expression_holder<tensor_expression>>>
      cases = {
          {"inner(C,C)", inner_product(C, sequence{2}, C, sequence{1})},
          {"otimes(C,B)", otimes(C, B)},
          {"trace(C)*C", trace(C) * C},
          {"inner(W,W)", inner_product(W, sequence{2}, W, sequence{1})},
      };
  for (auto const &[name, e] : cases) {
    SCOPED_TRACE(::testing::Message() << "product node=" << name);
    check_cell(Cell{A, N, N, N}, e);
  }
}

} // namespace numsim::cas

#endif // TENSORANNOTATIONMATRIXTEST_H
