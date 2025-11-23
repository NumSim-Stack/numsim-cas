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
template <class T> struct Parallel {
  // unit vector expression; your library’s 1st-order tensor
  expression_holder<tensor_expression<T>> n;
  bool normalize = true;
};

template <class T> struct Perp {
  expression_holder<tensor_expression<T>> n;
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
template <class T> struct tensor_space {
  std::variant<General, Symmetric, Skew, Young> perm;
  std::variant<AnyTraceTag, VolumetricTag, DeviatoricTag, HarmonicTag,
               PartialTraceTag>
      trace;
};

template <typename T>
static constexpr inline auto is_tensor_space(tensor_space<T> const &) {}

template <class T>
class tensor_projector final
    : public expression_crtp<tensor_projector<T>, tensor_expression<T>> {
public:
  using base = expression_crtp<tensor_projector<T>, tensor_expression<T>>;
  using value_type = T;

  tensor_projector(std::size_t dim, std::size_t acts_on_rank,
                   tensor_space<T> space)
      : base(dim, 2 * acts_on_rank), r_(acts_on_rank),
        space_(std::move(space)) {}

  std::size_t acts_on_rank() const { return r_; }
  const tensor_space<T> &space() const { return space_; }

  template <typename _ValueType>
  friend bool operator<(tensor_projector<_ValueType> const &lhs,
                        tensor_projector<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator>(tensor_projector<_ValueType> const &lhs,
                        tensor_projector<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator==(tensor_projector<_ValueType> const &lhs,
                         tensor_projector<_ValueType> const &rhs);
  template <typename _ValueType>
  friend bool operator!=(tensor_projector<_ValueType> const &lhs,
                         tensor_projector<_ValueType> const &rhs);

  virtual void update_hash_value() const override {
    hash_combine(base::m_hash_value, base::get_id());
  }

private:
  std::size_t r_;
  tensor_space<T> space_;
};

template <typename ValueType>
bool operator<(tensor_projector<ValueType> const &lhs,
               tensor_projector<ValueType> const &rhs) {
  return lhs.hash_value() < rhs.hash_value();
}

template <typename ValueType>
bool operator>(tensor_projector<ValueType> const &lhs,
               tensor_projector<ValueType> const &rhs) {
  return !(lhs < rhs);
}

template <typename ValueType>
bool operator==(tensor_projector<ValueType> const &lhs,
                tensor_projector<ValueType> const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

template <typename ValueType>
bool operator!=(tensor_projector<ValueType> const &lhs,
                tensor_projector<ValueType> const &rhs) {
  return !(lhs == rhs);
}

template <class T>
auto make_projector(std::size_t dim, std::size_t r,
                    decltype(tensor_space<T>::perm) perm,
                    decltype(tensor_space<T>::trace) trace) {
  return make_expression<tensor_projector<T>>(
      dim, r, tensor_space<T>{std::move(perm), std::move(trace)});
}

// Common presets for rank-2:
template <class T> auto P_sym(std::size_t d) {
  return make_projector<T>(d, 2, Symmetric{}, AnyTraceTag{});
}
template <class T> auto P_skew(std::size_t d) {
  return make_projector<T>(d, 2, Skew{}, AnyTraceTag{});
}
template <class T> auto P_vol(std::size_t d) {
  return make_projector<T>(d, 2, Symmetric{}, VolumetricTag{});
}
template <class T> auto P_devi(std::size_t d) {
  return make_projector<T>(d, 2, Symmetric{}, DeviatoricTag{});
}
template <class T> auto P_harm(std::size_t d, std::size_t r = 2) {
  return make_projector<T>(d, r, Symmetric{}, HarmonicTag{});
}

template <typename T> class tensor_trace_print_visitor {
public:
  tensor_trace_print_visitor(tensor_projector<T> const &proj) : m_proj(proj) {}

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
  const tensor_projector<T> &m_proj;
};

} // namespace numsim::cas

#endif // PROJECTION_TENSOR_H
