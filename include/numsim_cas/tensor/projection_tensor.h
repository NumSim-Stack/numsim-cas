#ifndef PROJECTION_TENSOR_H
#define PROJECTION_TENSOR_H

#include "tensor_expression.h"

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
struct Parallel {
  // unit vector expression; your library’s 1st-order tensor
  expression_holder<tensor_expression> n;
  bool normalize = true;
};

struct Perp {
  expression_holder<tensor_expression> n;
  bool normalize = true;
};

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

static constexpr inline auto is_tensor_space(tensor_space const &) {}

class tensor_projector final
    : public expression_crtp<tensor_projector, tensor_expression> {
public:
  using base = expression_crtp<tensor_projector, tensor_expression>;

  tensor_projector(std::size_t dim, std::size_t acts_on_rank,
                   tensor_space space)
      : base(dim, 2 * acts_on_rank), r_(acts_on_rank),
        space_(std::move(space)) {}

  std::size_t acts_on_rank() const { return r_; }
  const tensor_space &space() const { return space_; }

  friend bool operator<(tensor_projector const &lhs,
                        tensor_projector const &rhs);
  friend bool operator>(tensor_projector const &lhs,
                        tensor_projector const &rhs);
  friend bool operator==(tensor_projector const &lhs,
                         tensor_projector const &rhs);
  friend bool operator!=(tensor_projector const &lhs,
                         tensor_projector const &rhs);

  virtual void update_hash_value() const override {
    hash_combine(base::m_hash_value, base::get_id());
  }

private:
  std::size_t r_;
  tensor_space space_;
};

bool operator<(tensor_projector const &lhs, tensor_projector const &rhs) {
  return lhs.hash_value() < rhs.hash_value();
}

bool operator>(tensor_projector const &lhs, tensor_projector const &rhs) {
  return rhs < lhs;
}

bool operator==(tensor_projector const &lhs, tensor_projector const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

bool operator!=(tensor_projector const &lhs, tensor_projector const &rhs) {
  return !(lhs == rhs);
}

auto make_projector(std::size_t dim, std::size_t r,
                    decltype(tensor_space::perm) perm,
                    decltype(tensor_space::trace) trace) {
  return make_expression<tensor_projector>(
      dim, r, tensor_space{std::move(perm), std::move(trace)});
}

// Common presets for rank-2:
auto P_sym(std::size_t d) {
  return make_projector(d, 2, Symmetric{}, AnyTraceTag{});
}
auto P_skew(std::size_t d) {
  return make_projector(d, 2, Skew{}, AnyTraceTag{});
}
auto P_vol(std::size_t d) {
  return make_projector(d, 2, Symmetric{}, VolumetricTag{});
}
auto P_devi(std::size_t d) {
  return make_projector(d, 2, Symmetric{}, DeviatoricTag{});
}
auto P_harm(std::size_t d, std::size_t r = 2) {
  return make_projector(d, r, Symmetric{}, HarmonicTag{});
}

class tensor_trace_print_visitor {
public:
  tensor_trace_print_visitor(tensor_projector const &proj) : m_proj(proj) {}

  constexpr inline auto apply() {
    return std::visit(*this, m_proj.space().perm, m_proj.space().trace);
  }

  template <typename Perm, typename Space>
  constexpr inline auto operator()(Perm const &, Space const &) {
    // static_assert(true, "tensor_projector::printer_visitor::operator() no
    // matching overload");
    return "";
  }

  template <typename Perm>
  constexpr inline auto operator()(Perm const &, Symmetric const &) {
    return "sym";
  }

  template <typename Perm>
  constexpr inline auto operator()(Perm const &, Skew const &) {
    return "skew";
  }

  template <typename Perm>
  constexpr inline auto operator()(Perm const &, VolumetricTag const &) {
    return "vol";
  }

  template <typename Perm>
  constexpr inline auto operator()(Perm const &, DeviatoricTag const &) {
    return "dev";
  }

  template <typename Perm>
  constexpr inline auto operator()(Perm const &, HarmonicTag const &) {
    return "harm";
  }

private:
  const tensor_projector &m_proj;
};

} // namespace numsim::cas

#endif // PROJECTION_TENSOR_H
