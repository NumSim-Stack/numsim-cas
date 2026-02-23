#ifndef TENSOR_SPACE_H
#define TENSOR_SPACE_H

#include <optional>
#include <variant>
#include <vector>

namespace numsim::cas {

// Algebraic spaces on rank-2 tensors
struct Symmetric {}; // sym
struct Skew {};      // skew
struct Full {};      // identity on the whole space

// Higher-rank symmetrizers
struct Sym_r {
  std::size_t r;
}; // totally symmetric of rank r
struct Anti_r {
  std::size_t r;
}; // totally antisymmetric of rank r
struct Harm_r {
  std::size_t r;
}; // symmetric, trace-free (SO(d) harmonic)

// Rank-4 elasticity-style
struct Minor {};      // minor symmetry in (ij) and (kl)
struct Major {};      // major symmetry (ij) <-> (kl)
struct MinorMajor {}; // both

// Directional projectors (onto || or ⟂ to a direction)
// NOTE: Parallel and Perp remain in projection_tensor.h because they reference
// expression_holder<tensor_expression>, which is not available here.

// --- Permutation spaces (rank-agnostic) ---
struct General {}; // no permutation constraint
struct Young {     // generic Young symmetrizer
  // e.g. {{1,2},{3}} means sym over (1,2), hold 3 separate
  std::vector<std::vector<int>> blocks; // 1-based positions
};

// --- Trace spaces (rank-agnostic) ---
struct AnyTraceTag {};   // no trace constraint
struct VolumetricTag {}; // tr(.)/d * I
struct DeviatoricTag {}; // sym(.) - vol(.)
struct HarmonicTag {};   // fully trace-free (all pairs)
struct PartialTraceTag { // remove/keep traces on selected pairs
  std::vector<std::pair<int, int>> pairs; // 1-based positions to contract
};

// Bundle them (rank and dim are on the projector node)
struct tensor_space {
  std::variant<General, Symmetric, Skew, Young> perm;
  std::variant<AnyTraceTag, VolumetricTag, DeviatoricTag, HarmonicTag,
               PartialTraceTag>
      trace;
};

/// Join two tensor_space values for addition.
/// Same space → same; both Symmetric perm → {Symmetric, AnyTrace}; else →
/// nullopt.
inline std::optional<tensor_space> join_tensor_space(tensor_space const &a,
                                                     tensor_space const &b) {
  // Identical spaces join to themselves
  if (a.perm.index() == b.perm.index() && a.trace.index() == b.trace.index())
    return a;
  // Both in the Symmetric family (Sym/Vol/Dev/Harmonic) → widen to Sym
  if (std::holds_alternative<Symmetric>(a.perm) &&
      std::holds_alternative<Symmetric>(b.perm))
    return tensor_space{Symmetric{}, AnyTraceTag{}};
  // Incompatible (e.g. Symmetric + Skew) → no join
  return std::nullopt;
}

} // namespace numsim::cas

#endif // TENSOR_SPACE_H
