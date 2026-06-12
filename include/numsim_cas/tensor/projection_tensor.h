#ifndef PROJECTION_TENSOR_H
#define PROJECTION_TENSOR_H

#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/tensor_space.h>

namespace numsim::cas {

// Directional projectors (onto || or ⟂ to a direction)
// These remain here because they reference
// expression_holder<tensor_expression>.
struct Parallel {
  // unit vector expression; your library's 1st-order tensor
  expression_holder<tensor_expression> n;
  bool normalize = true;
};

struct Perp {
  expression_holder<tensor_expression> n;
  bool normalize = true;
};

class tensor_projector final : public tensor_node_base_t<tensor_projector> {
public:
  using base = tensor_node_base_t<tensor_projector>;

  tensor_projector(std::size_t dim, std::size_t acts_on_rank,
                   tensor_space space)
      : base(dim, 2 * acts_on_rank), r_(acts_on_rank) {
    // Single source of truth: the projector's classification lives in the
    // base m_tensor_space (no separate space_ member). The non-optional
    // space() accessor below derefs the optional. Same closed-form-constant
    // pre-annotation pattern as identity_tensor — see
    // docs/sympy-assumption-redesign.md step 2.
    this->set_space(std::move(space));
  }

  std::size_t acts_on_rank() const { return r_; }
  // Non-optional accessor — projector's m_tensor_space is always populated
  // at construction. Hides the inherited std::optional return for
  // projector-specific call sites (evaluator, projector_algebra::classify)
  // that need the value directly. The invariant is enforced by the
  // clear_space() override below being a no-op.
  const tensor_space &space() const {
    assert(this->m_tensor_space.has_value() &&
           "tensor_projector::space invariant violated");
    return *this->m_tensor_space;
  }

  // Closed-form constant: structural classification is intrinsic. See
  // identity_tensor for the same override rationale. Without this,
  // clear_space() called through a tensor_expression* would leave
  // m_tensor_space empty and the next space() deref would be release-mode
  // UB.
  void clear_space() noexcept override {}

  friend bool operator<(tensor_projector const &lhs,
                        tensor_projector const &rhs) {
    return lhs.hash_value() < rhs.hash_value();
  }

  friend bool operator>(tensor_projector const &lhs,
                        tensor_projector const &rhs) {
    return rhs < lhs;
  }

  friend bool operator==(tensor_projector const &lhs,
                         tensor_projector const &rhs) {
    return lhs.hash_value() == rhs.hash_value();
  }

  friend bool operator!=(tensor_projector const &lhs,
                         tensor_projector const &rhs) {
    return !(lhs == rhs);
  }

  void update_hash_value() const override {
    base::m_hash_value = 0;
    hash_combine(base::m_hash_value, base::get_id());
    hash_combine(base::m_hash_value, this->dim());
    hash_combine(base::m_hash_value, r_);
    auto const &sp = this->space();
    hash_combine(base::m_hash_value, sp.perm.index());
    hash_combine(base::m_hash_value, sp.trace.index());
    std::visit(
        [this](auto const &v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, Young>) {
            for (auto const &block : v.blocks)
              hash_combine(base::m_hash_value, block);
          }
        },
        sp.perm);
    std::visit(
        [this](auto const &v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, PartialTraceTag>) {
            for (auto const &[a, b] : v.pairs) {
              hash_combine(base::m_hash_value, a);
              hash_combine(base::m_hash_value, b);
            }
          }
        },
        sp.trace);
  }

private:
  std::size_t r_;
};

inline auto make_projector(std::size_t dim, std::size_t r,
                           decltype(tensor_space::perm) perm,
                           decltype(tensor_space::trace) trace) {
  return make_expression<tensor_projector>(
      dim, r, tensor_space{std::move(perm), std::move(trace)});
}

// Common presets for rank-2:
inline auto P_sym(std::size_t d) {
  return make_projector(d, 2, Symmetric{}, AnyTraceTag{});
}
inline auto P_skew(std::size_t d) {
  return make_projector(d, 2, Skew{}, AnyTraceTag{});
}
inline auto P_vol(std::size_t d) {
  return make_projector(d, 2, Symmetric{}, VolumetricTag{});
}
inline auto P_devi(std::size_t d) {
  return make_projector(d, 2, Symmetric{}, DeviatoricTag{});
}
inline auto P_harm(std::size_t d, std::size_t r = 2) {
  return make_projector(d, r, Symmetric{}, HarmonicTag{});
}

// Rank-4 presets (#299): projectors that act on rank-4 tensors,
// producing rank-8 results. These are the analogs of P_sym / P_skew
// for the rank-2-input case, and are returned by the diff-visitor's
// leaf rule for `diff(annotated rank-4 tensor, itself)` so that the
// rank-4 Magnus chain rule sees the correct projected identity rather
// than the unconstrained rank-8 identity (which would over-count by an
// orbit-stabilizer factor — see #299 for derivation).
inline auto P_minor4(std::size_t d) {
  return make_projector(d, 4, Minor{}, AnyTraceTag{});
}
inline auto P_minor_major4(std::size_t d) {
  return make_projector(d, 4, MinorMajor{}, AnyTraceTag{});
}

class tensor_trace_print_visitor {
public:
  tensor_trace_print_visitor(tensor_projector const &proj) : m_proj(proj) {}

  constexpr inline std::string apply() {
    return std::visit(*this, m_proj.space().perm, m_proj.space().trace);
  }

  template <typename Perm, typename Trace>
  constexpr inline std::string operator()(Perm const &, Trace const &) const {
    return "P";
  }

  // Trace-specific: perm=Symmetric or General, trace=VolumetricTag
  template <typename Perm>
  constexpr inline std::string operator()(Perm const &,
                                          VolumetricTag const &) const {
    return "P_vol";
  }

  // Trace-specific: perm=Symmetric or General, trace=DeviatoricTag
  template <typename Perm>
  constexpr inline std::string operator()(Perm const &,
                                          DeviatoricTag const &) const {
    return "P_dev";
  }

  // Trace-specific: perm=Symmetric or General, trace=HarmonicTag
  template <typename Perm>
  constexpr inline std::string operator()(Perm const &,
                                          HarmonicTag const &) const {
    return "P_harm";
  }

  // Perm-specific: perm=Symmetric, trace=AnyTraceTag (no trace constraint)
  constexpr inline std::string operator()(Symmetric const &,
                                          AnyTraceTag const &) const {
    return "P_sym";
  }

  // Perm-specific: perm=Skew, trace=AnyTraceTag (no trace constraint)
  constexpr inline std::string operator()(Skew const &,
                                          AnyTraceTag const &) const {
    return "P_skew";
  }

private:
  const tensor_projector &m_proj;
};

} // namespace numsim::cas

#endif // PROJECTION_TENSOR_H
