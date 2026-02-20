#ifndef PROJECTION_TENSOR_H
#define PROJECTION_TENSOR_H

#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/tensor_space.h>

namespace numsim::cas {

// Directional projectors (onto || or ⟂ to a direction)
// These remain here because they reference expression_holder<tensor_expression>.
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
      : base(dim, 2 * acts_on_rank), r_(acts_on_rank),
        space_(std::move(space)) {}

  std::size_t acts_on_rank() const { return r_; }
  const tensor_space &space() const { return space_; }

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

  virtual void update_hash_value() const override {
    base::m_hash_value = 0;
    hash_combine(base::m_hash_value, base::get_id());
    hash_combine(base::m_hash_value, this->dim());
    hash_combine(base::m_hash_value, r_);
    hash_combine(base::m_hash_value, space_.perm.index());
    hash_combine(base::m_hash_value, space_.trace.index());
    std::visit([this](auto const &v) {
      using T = std::decay_t<decltype(v)>;
      if constexpr (std::is_same_v<T, Young>) {
        for (auto const &block : v.blocks)
          hash_combine(base::m_hash_value, block);
      }
    }, space_.perm);
    std::visit([this](auto const &v) {
      using T = std::decay_t<decltype(v)>;
      if constexpr (std::is_same_v<T, PartialTraceTag>) {
        for (auto const &[a, b] : v.pairs) {
          hash_combine(base::m_hash_value, a);
          hash_combine(base::m_hash_value, b);
        }
      }
    }, space_.trace);
  }

private:
  std::size_t r_;
  tensor_space space_;
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
