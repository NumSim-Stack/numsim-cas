#include <numsim_cas/scalar/visitors/scalar_assumption_propagator.h>

#include <numsim_cas/scalar/scalar_assume.h>
#include <numsim_cas/scalar/scalar_functions.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <ranges>

namespace numsim::cas {

numeric_assumption_manager
scalar_assumption_propagator::apply(expr_holder_t const &expr) {
  if (!expr.is_valid()) {
    m_result = {};
    return m_result;
  }
  expr.template get<scalar_visitable_t>().accept(*this);
  // Write inferred assumptions back onto the node
  for (auto const &a : m_result.data()) {
    expr.data()->assumptions().insert(a);
  }
  return m_result;
}

// ─── Leaf nodes ────────────────────────────────────────────────────

void scalar_assumption_propagator::operator()(scalar const &node) {
  // Symbol: keep existing user-set assumptions
  m_result = {};
  for (auto const &a : node.assumptions().data()) {
    m_result.insert(a);
  }
}

void scalar_assumption_propagator::operator()(
    [[maybe_unused]] scalar_zero const &) {
  m_result = {};
  m_result.insert(nonnegative{});
  m_result.insert(nonpositive{});
  m_result.insert(real_tag{});
  m_result.insert(integer{});
}

void scalar_assumption_propagator::operator()(
    [[maybe_unused]] scalar_one const &) {
  m_result = {};
  m_result.insert(positive{});
  m_result.insert(nonnegative{});
  m_result.insert(nonzero{});
  m_result.insert(integer{});
  m_result.insert(real_tag{});
}

void scalar_assumption_propagator::operator()(scalar_constant const &v) {
  m_result = {};

  bool is_int = std::holds_alternative<std::int64_t>(v.value().raw());
  bool is_rational = std::holds_alternative<rational_t>(v.value().raw());
  bool is_complex =
      std::holds_alternative<std::complex<double>>(v.value().raw());
  double val = std::visit(
      [](auto const &x) -> double {
        using V = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<V, std::complex<double>>) {
          return x.real();
        } else if constexpr (std::is_same_v<V, rational_t>) {
          return static_cast<double>(x.num) / static_cast<double>(x.den);
        } else {
          return static_cast<double>(x);
        }
      },
      v.value().raw());

  // Complex values get NO predicates — not orderable, not real in general.
  // Parity with scalar_constant::annotate_from_value's silent-no-op branch
  // for complex. cpp-pro F7: previously the propagator unconditionally
  // inserted real_tag even for complex, diverging from the ctor.
  if (is_complex)
    return;

  m_result.insert(real_tag{});
  // Parity with scalar_constant::annotate_from_value: int64 implies
  // {integer, rational}; rational_t implies rational; zero (any spelling)
  // implies {integer, rational}. cpp-pro F1: the inferred-flag
  // short-circuit masks this path today, but a future cache
  // invalidation would otherwise strip facts that the ctor set.
  if (is_int) {
    m_result.insert(integer{});
    m_result.insert(rational{});
  } else if (is_rational) {
    m_result.insert(rational{});
  }

  if (val > 0) {
    m_result.insert(positive{});
    m_result.insert(nonnegative{});
    m_result.insert(nonzero{});
  } else if (val < 0) {
    m_result.insert(negative{});
    m_result.insert(nonpositive{});
    m_result.insert(nonzero{});
  } else {
    m_result.insert(nonnegative{});
    m_result.insert(nonpositive{});
    // Zero is integer regardless of storage spelling (S(0), S(0.0),
    // Rational(0)). SymPy convention.
    m_result.insert(integer{});
    m_result.insert(rational{});
  }
}

// ─── Arithmetic ────────────────────────────────────────────────────

void scalar_assumption_propagator::operator()(scalar_add const &v) {
  bool all_pos = true, all_neg = true;
  bool all_nonneg = true, all_nonpos = true;

  if (v.coeff().is_valid()) {
    auto ca = apply(v.coeff());
    all_pos &= ca.contains(positive{});
    all_neg &= ca.contains(negative{});
    all_nonneg &= ca.contains(nonnegative{});
    all_nonpos &= ca.contains(nonpositive{});
  }

  for (auto const &child : v.symbol_map() | std::views::values) {
    auto ca = apply(child);
    all_pos &= ca.contains(positive{});
    all_neg &= ca.contains(negative{});
    all_nonneg &= ca.contains(nonnegative{});
    all_nonpos &= ca.contains(nonpositive{});
  }

  m_result = {};
  if (all_pos)
    m_result.insert(positive{});
  if (all_neg)
    m_result.insert(negative{});
  if (all_nonneg)
    m_result.insert(nonnegative{});
  if (all_nonpos)
    m_result.insert(nonpositive{});
  if (all_pos || all_neg)
    m_result.insert(nonzero{});
  if (all_nonneg || all_nonpos)
    m_result.insert(real_tag{});
}

void scalar_assumption_propagator::operator()(scalar_mul const &v) {
  int neg_count = 0;
  bool all_nonzero = true;
  bool all_real = true;
  bool all_have_sign = true; // all children have a definite sign

  if (v.coeff().is_valid()) {
    auto ca = apply(v.coeff());
    if (ca.contains(negative{}))
      ++neg_count;
    else if (!ca.contains(positive{}) && !ca.contains(nonnegative{}))
      all_have_sign = false;
    all_nonzero &= ca.contains(nonzero{});
    all_real &= ca.contains(real_tag{});
  }

  for (auto const &child : v.symbol_map() | std::views::values) {
    auto ca = apply(child);
    if (ca.contains(negative{}))
      ++neg_count;
    else if (!ca.contains(positive{}) && !ca.contains(nonnegative{}))
      all_have_sign = false;
    all_nonzero &= ca.contains(nonzero{});
    all_real &= ca.contains(real_tag{});
  }

  m_result = {};
  if (all_real)
    m_result.insert(real_tag{});
  if (all_nonzero)
    m_result.insert(nonzero{});
  if (all_have_sign && (neg_count % 2 == 0)) {
    m_result.insert(nonnegative{});
    if (all_nonzero)
      m_result.insert(positive{});
  } else if (all_have_sign && (neg_count % 2 == 1)) {
    m_result.insert(nonpositive{});
    if (all_nonzero)
      m_result.insert(negative{});
  }
}

void scalar_assumption_propagator::operator()(scalar_negative const &v) {
  auto ca = apply(v.expr());
  m_result = {};

  // flip: pos<->neg, nonneg<->nonpos
  if (ca.contains(positive{}))
    m_result.insert(negative{});
  if (ca.contains(negative{}))
    m_result.insert(positive{});
  if (ca.contains(nonnegative{}))
    m_result.insert(nonpositive{});
  if (ca.contains(nonpositive{}))
    m_result.insert(nonnegative{});
  // preserve nonzero, real, integer
  if (ca.contains(nonzero{}))
    m_result.insert(nonzero{});
  if (ca.contains(real_tag{}))
    m_result.insert(real_tag{});
  if (ca.contains(integer{}))
    m_result.insert(integer{});
}

void scalar_assumption_propagator::operator()(scalar_pow const &v) {
  auto base_a = apply(v.expr_lhs());
  auto exp_a = apply(v.expr_rhs());

  m_result = {};

  // Check if exponent is an even integer constant. Uses
  // try_int_constant (#284 architectural rule) so the
  // scalar_one / scalar_zero / scalar_negative singletons —
  // produced by the parser and the pow factory for the
  // literal-0 / literal-1 / literal-negation cases — are
  // recognised. The bare `is_same<scalar_constant>` check this
  // replaced missed pow(x, 0) (scalar_zero, even) and would have
  // mis-classified pow(x, -2) (scalar_negative(scalar_constant{2})).
  bool exp_even = false;
  if (auto iv = try_int_constant(v.expr_rhs())) {
    exp_even = (*iv % 2 == 0);
  }
  // Also check if exponent has even assumption
  if (exp_a.contains(even{}))
    exp_even = true;

  if (exp_even) {
    m_result.insert(nonnegative{});
    m_result.insert(real_tag{});
    if (base_a.contains(nonzero{}))
      m_result.insert(positive{});
  }
  if (base_a.contains(positive{})) {
    m_result.insert(positive{});
    m_result.insert(nonnegative{});
    m_result.insert(nonzero{});
    m_result.insert(real_tag{});
  }
  if (base_a.contains(real_tag{}) && exp_a.contains(real_tag{}))
    m_result.insert(real_tag{});
}

// ─── Functions ─────────────────────────────────────────────────────

void scalar_assumption_propagator::operator()(scalar_abs const &v) {
  auto ca = apply(v.expr());
  m_result = {};
  m_result.insert(nonnegative{});
  m_result.insert(real_tag{});
  if (ca.contains(nonzero{}))
    m_result.insert(positive{});
}

void scalar_assumption_propagator::operator()(
    [[maybe_unused]] scalar_sqrt const &v) {
  apply(v.expr());
  m_result = {};
  m_result.insert(nonnegative{});
  m_result.insert(real_tag{});
}

void scalar_assumption_propagator::operator()(
    [[maybe_unused]] scalar_exp const &v) {
  apply(v.expr());
  m_result = {};
  m_result.insert(positive{});
  m_result.insert(nonnegative{});
  m_result.insert(nonzero{});
  m_result.insert(real_tag{});
}

void scalar_assumption_propagator::operator()(scalar_sign const &v) {
  apply(v.expr());
  m_result = {};
  m_result.insert(integer{});
  m_result.insert(real_tag{});
}

void scalar_assumption_propagator::operator()(scalar_log const &v) {
  auto ca = apply(v.expr());
  m_result = {};
  if (ca.contains(positive{}))
    m_result.insert(real_tag{});
}

void scalar_assumption_propagator::operator()(scalar_sin const &v) {
  auto ca = apply(v.expr());
  m_result = {};
  if (ca.contains(real_tag{}))
    m_result.insert(real_tag{});
}

void scalar_assumption_propagator::operator()(scalar_cos const &v) {
  auto ca = apply(v.expr());
  m_result = {};
  if (ca.contains(real_tag{}))
    m_result.insert(real_tag{});
}

void scalar_assumption_propagator::operator()(scalar_tan const &v) {
  auto ca = apply(v.expr());
  m_result = {};
  if (ca.contains(real_tag{}))
    m_result.insert(real_tag{});
}

void scalar_assumption_propagator::operator()(scalar_asin const &v) {
  auto ca = apply(v.expr());
  m_result = {};
  if (ca.contains(real_tag{}))
    m_result.insert(real_tag{});
}

void scalar_assumption_propagator::operator()(scalar_acos const &v) {
  auto ca = apply(v.expr());
  m_result = {};
  if (ca.contains(real_tag{}))
    m_result.insert(real_tag{});
}

void scalar_assumption_propagator::operator()(scalar_atan const &v) {
  auto ca = apply(v.expr());
  m_result = {};
  if (ca.contains(real_tag{}))
    m_result.insert(real_tag{});
}

void scalar_assumption_propagator::operator()(
    [[maybe_unused]] scalar_named_expression const &) {
  m_result = {};
}

// ─── Comparison nodes (#136) ───────────────────────────────────────
// Indicator values: always in {0, 1} ⇒ real, integer, nonnegative.
// Children visited for completeness so their assumptions land in any
// downstream caches; only the indicator's own tags get reported.
namespace {
void set_indicator_assumptions(numeric_assumption_manager &r) {
  r = {};
  r.insert(real_tag{});
  r.insert(integer{});
  r.insert(nonnegative{});
}
} // namespace

template <typename BinaryNode>
void scalar_assumption_propagator::handle_comparison_node(BinaryNode const &v) {
  apply(v.expr_lhs());
  apply(v.expr_rhs());
  set_indicator_assumptions(m_result);
}

void scalar_assumption_propagator::operator()(scalar_lt const &v) {
  handle_comparison_node(v);
}
void scalar_assumption_propagator::operator()(scalar_gt const &v) {
  handle_comparison_node(v);
}
void scalar_assumption_propagator::operator()(scalar_le const &v) {
  handle_comparison_node(v);
}
void scalar_assumption_propagator::operator()(scalar_ge const &v) {
  handle_comparison_node(v);
}
void scalar_assumption_propagator::operator()(scalar_eq const &v) {
  handle_comparison_node(v);
}
void scalar_assumption_propagator::operator()(scalar_ne const &v) {
  handle_comparison_node(v);
}

// Min / max (#137). Recurse into both children to populate the inference
// cache; the result range is bounded by the operand ranges in the obvious
// Sign-monotonicity propagation (review of #207):
//
//   max(a, b) ≥ max(lower(a), lower(b))   ⇒ either side ≥ 0 ⇒ max ≥ 0
//   min(a, b) ≤ min(upper(a), upper(b))   ⇒ either side ≤ 0 ⇒ min ≤ 0
//
// In particular for max:
//   - either operand positive       → max positive
//   - either operand nonnegative    → max nonnegative
//   - both operands nonpositive     → max nonpositive
//   - both operands negative        → max negative
// And symmetric facts for min. `real_tag` propagates when both operands
// are real. Used by the Macauley-bracket downstream consumer (#138) so
// that `max(x, 0) ≥ 0` and `min(x, 0) ≤ 0` aren't dropped.
namespace {

inline void propagate_max_signs(numeric_assumption_manager const &cl,
                                numeric_assumption_manager const &cr,
                                numeric_assumption_manager &out) {
  if (cl.contains(real_tag{}) && cr.contains(real_tag{}))
    out.insert(real_tag{});
  if (cl.contains(positive{}) || cr.contains(positive{})) {
    out.insert(positive{});
    out.insert(nonnegative{});
    out.insert(nonzero{});
  } else if (cl.contains(nonnegative{}) || cr.contains(nonnegative{})) {
    out.insert(nonnegative{});
  } else if (cl.contains(negative{}) && cr.contains(negative{})) {
    out.insert(negative{});
    out.insert(nonpositive{});
    out.insert(nonzero{});
  } else if (cl.contains(nonpositive{}) && cr.contains(nonpositive{})) {
    out.insert(nonpositive{});
  }
}

inline void propagate_min_signs(numeric_assumption_manager const &cl,
                                numeric_assumption_manager const &cr,
                                numeric_assumption_manager &out) {
  if (cl.contains(real_tag{}) && cr.contains(real_tag{}))
    out.insert(real_tag{});
  if (cl.contains(negative{}) || cr.contains(negative{})) {
    out.insert(negative{});
    out.insert(nonpositive{});
    out.insert(nonzero{});
  } else if (cl.contains(nonpositive{}) || cr.contains(nonpositive{})) {
    out.insert(nonpositive{});
  } else if (cl.contains(positive{}) && cr.contains(positive{})) {
    out.insert(positive{});
    out.insert(nonnegative{});
    out.insert(nonzero{});
  } else if (cl.contains(nonnegative{}) && cr.contains(nonnegative{})) {
    out.insert(nonnegative{});
  }
}

} // namespace

void scalar_assumption_propagator::operator()(scalar_max const &v) {
  auto const cl = apply(v.expr_lhs());
  auto const cr = apply(v.expr_rhs());
  m_result = numeric_assumption_manager{};
  propagate_max_signs(cl, cr, m_result);
}
void scalar_assumption_propagator::operator()(scalar_min const &v) {
  auto const cl = apply(v.expr_lhs());
  auto const cr = apply(v.expr_rhs());
  m_result = numeric_assumption_manager{};
  propagate_min_signs(cl, cr, m_result);
}

// if_then_else (#135). The result is either expr_then or expr_else,
// so any assumption tag they BOTH carry is also carried by the
// if_then_else as a whole — i.e. the intersection. We don't need
// interval-union machinery for this; tag intersection is enough to
// catch the common cases (both branches nonnegative ⇒ result
// nonnegative, both real ⇒ real, etc.). #209 review.
namespace {
inline void
propagate_if_then_else_intersection(numeric_assumption_manager const &ct,
                                    numeric_assumption_manager const &ce,
                                    numeric_assumption_manager &out) {
  auto intersect = [&](auto tag) {
    if (ct.contains(tag) && ce.contains(tag))
      out.insert(tag);
  };
  intersect(positive{});
  intersect(negative{});
  intersect(nonzero{});
  intersect(nonnegative{});
  intersect(nonpositive{});
  intersect(real_tag{});
  intersect(integer{});
}
} // namespace
void scalar_assumption_propagator::operator()(scalar_if_then_else const &v) {
  apply(v.expr_cond());
  auto const ct = apply(v.expr_then());
  auto const ce = apply(v.expr_else());
  m_result = numeric_assumption_manager{};
  propagate_if_then_else_intersection(ct, ce, m_result);
}

// ─── Convenience function ──────────────────────────────────────────

numeric_assumption_manager
propagate_assumptions(expression_holder<scalar_expression> const &expr) {
  scalar_assumption_propagator prop;
  return prop.apply(expr);
}

// ═══════════════════════════════════════════════════════════════════════
//  Shallow inference: reads children's existing assumptions, no recursion
// ═══════════════════════════════════════════════════════════════════════

namespace {

using expr_holder_t = expression_holder<scalar_expression>;

// Ensure a child node's assumptions are populated via lazy inference.
numeric_assumption_manager const &
ensure_assumptions(expr_holder_t const &expr) {
  infer_assumptions(expr);
  return expr.data()->assumptions();
}

// Shallow inference visitor — dispatches on node type, reads children's
// already-set assumptions without recursion, sets inferred assumptions
// on the node.
class shallow_inference_visitor final : public scalar_visitor_const_t {
public:
  void run(expr_holder_t const &expr) {
    if (!expr.is_valid())
      return;
    expr.template get<scalar_visitable_t>().accept(*this);
    // Write inferred assumptions onto the node
    auto &a = expr.data()->assumptions();
    a.clear();
    for (auto const &x : m_result.data())
      a.insert(x);
  }

  // Leaf nodes — compute intrinsic assumptions
  void operator()(scalar const &node) override {
    m_result = {};
    for (auto const &a : node.assumptions().data())
      m_result.insert(a);
  }

  void operator()([[maybe_unused]] scalar_zero const &) override {
    m_result = {};
    m_result.insert(nonnegative{});
    m_result.insert(nonpositive{});
    m_result.insert(real_tag{});
    m_result.insert(integer{});
  }

  void operator()([[maybe_unused]] scalar_one const &) override {
    m_result = {};
    m_result.insert(positive{});
    m_result.insert(nonnegative{});
    m_result.insert(nonzero{});
    m_result.insert(integer{});
    m_result.insert(real_tag{});
  }

  void operator()(scalar_constant const &v) override {
    m_result = {};
    auto const &cv = v.value();
    bool is_int = std::holds_alternative<std::int64_t>(cv.raw());
    bool is_rational = std::holds_alternative<rational_t>(cv.raw());
    bool is_complex = std::holds_alternative<std::complex<double>>(cv.raw());
    double val = std::visit(
        [](auto const &x) -> double {
          using V = std::decay_t<decltype(x)>;
          if constexpr (std::is_same_v<V, std::complex<double>>)
            return x.real();
          else if constexpr (std::is_same_v<V, rational_t>)
            return static_cast<double>(x.num) / static_cast<double>(x.den);
          else
            return static_cast<double>(x);
        },
        cv.raw());
    // Parity with annotate_from_value — see other propagator handler.
    if (is_complex)
      return;
    m_result.insert(real_tag{});
    if (is_int) {
      m_result.insert(integer{});
      m_result.insert(rational{});
    } else if (is_rational) {
      m_result.insert(rational{});
    }
    if (val > 0) {
      m_result.insert(positive{});
      m_result.insert(nonnegative{});
      m_result.insert(nonzero{});
    } else if (val < 0) {
      m_result.insert(negative{});
      m_result.insert(nonpositive{});
      m_result.insert(nonzero{});
    } else {
      m_result.insert(nonnegative{});
      m_result.insert(nonpositive{});
      m_result.insert(integer{});
      m_result.insert(rational{});
    }
  }

  void operator()([[maybe_unused]] scalar_named_expression const &) override {
    m_result = {};
  }

  // Arithmetic
  void operator()(scalar_add const &v) override {
    bool all_pos = true, all_neg = true;
    bool all_nonneg = true, all_nonpos = true;
    if (v.coeff().is_valid()) {
      auto const &ca = ensure_assumptions(v.coeff());
      all_pos &= ca.contains(positive{});
      all_neg &= ca.contains(negative{});
      all_nonneg &= ca.contains(nonnegative{});
      all_nonpos &= ca.contains(nonpositive{});
    }
    for (auto const &child : v.symbol_map() | std::views::values) {
      auto const &ca = ensure_assumptions(child);
      all_pos &= ca.contains(positive{});
      all_neg &= ca.contains(negative{});
      all_nonneg &= ca.contains(nonnegative{});
      all_nonpos &= ca.contains(nonpositive{});
    }
    m_result = {};
    if (all_pos)
      m_result.insert(positive{});
    if (all_neg)
      m_result.insert(negative{});
    if (all_nonneg)
      m_result.insert(nonnegative{});
    if (all_nonpos)
      m_result.insert(nonpositive{});
    if (all_pos || all_neg)
      m_result.insert(nonzero{});
    if (all_nonneg || all_nonpos)
      m_result.insert(real_tag{});
  }

  void operator()(scalar_mul const &v) override {
    int neg_count = 0;
    bool all_nonzero = true, all_real = true, all_have_sign = true;
    if (v.coeff().is_valid()) {
      auto const &ca = ensure_assumptions(v.coeff());
      if (ca.contains(negative{}))
        ++neg_count;
      else if (!ca.contains(positive{}) && !ca.contains(nonnegative{}))
        all_have_sign = false;
      all_nonzero &= ca.contains(nonzero{});
      all_real &= ca.contains(real_tag{});
    }
    for (auto const &child : v.symbol_map() | std::views::values) {
      auto const &ca = ensure_assumptions(child);
      if (ca.contains(negative{}))
        ++neg_count;
      else if (!ca.contains(positive{}) && !ca.contains(nonnegative{}))
        all_have_sign = false;
      all_nonzero &= ca.contains(nonzero{});
      all_real &= ca.contains(real_tag{});
    }
    m_result = {};
    if (all_real)
      m_result.insert(real_tag{});
    if (all_nonzero)
      m_result.insert(nonzero{});
    if (all_have_sign && (neg_count % 2 == 0)) {
      m_result.insert(nonnegative{});
      if (all_nonzero)
        m_result.insert(positive{});
    } else if (all_have_sign && (neg_count % 2 == 1)) {
      m_result.insert(nonpositive{});
      if (all_nonzero)
        m_result.insert(negative{});
    }
  }

  void operator()(scalar_negative const &v) override {
    auto const &ca = ensure_assumptions(v.expr());
    m_result = {};
    if (ca.contains(positive{}))
      m_result.insert(negative{});
    if (ca.contains(negative{}))
      m_result.insert(positive{});
    if (ca.contains(nonnegative{}))
      m_result.insert(nonpositive{});
    if (ca.contains(nonpositive{}))
      m_result.insert(nonnegative{});
    if (ca.contains(nonzero{}))
      m_result.insert(nonzero{});
    if (ca.contains(real_tag{}))
      m_result.insert(real_tag{});
    if (ca.contains(integer{}))
      m_result.insert(integer{});
  }

  void operator()(scalar_pow const &v) override {
    auto const &base_a = ensure_assumptions(v.expr_lhs());
    auto const &exp_a = ensure_assumptions(v.expr_rhs());
    m_result = {};
    bool exp_even = false;
    // try_int_constant (#284 architectural rule); see the equivalent
    // call in the earlier pow handler above for the singleton context.
    if (auto iv = try_int_constant(v.expr_rhs())) {
      exp_even = (*iv % 2 == 0);
    }
    if (exp_a.contains(even{}))
      exp_even = true;
    if (exp_even) {
      m_result.insert(nonnegative{});
      m_result.insert(real_tag{});
      if (base_a.contains(nonzero{}))
        m_result.insert(positive{});
    }
    if (base_a.contains(positive{})) {
      m_result.insert(positive{});
      m_result.insert(nonnegative{});
      m_result.insert(nonzero{});
      m_result.insert(real_tag{});
    }
    if (base_a.contains(real_tag{}) && exp_a.contains(real_tag{}))
      m_result.insert(real_tag{});
  }

  // Functions
  void operator()(scalar_abs const &v) override {
    auto const &ca = ensure_assumptions(v.expr());
    m_result = {};
    m_result.insert(nonnegative{});
    m_result.insert(real_tag{});
    if (ca.contains(nonzero{}))
      m_result.insert(positive{});
  }

  void operator()([[maybe_unused]] scalar_sqrt const &v) override {
    m_result = {};
    m_result.insert(nonnegative{});
    m_result.insert(real_tag{});
  }

  void operator()([[maybe_unused]] scalar_exp const &) override {
    m_result = {};
    m_result.insert(positive{});
    m_result.insert(nonnegative{});
    m_result.insert(nonzero{});
    m_result.insert(real_tag{});
  }

  void operator()([[maybe_unused]] scalar_sign const &) override {
    m_result = {};
    m_result.insert(integer{});
    m_result.insert(real_tag{});
  }

  void operator()(scalar_log const &v) override {
    auto const &ca = ensure_assumptions(v.expr());
    m_result = {};
    if (ca.contains(positive{}))
      m_result.insert(real_tag{});
  }

  void operator()(scalar_sin const &v) override {
    auto const &ca = ensure_assumptions(v.expr());
    m_result = {};
    if (ca.contains(real_tag{}))
      m_result.insert(real_tag{});
  }

  void operator()(scalar_cos const &v) override {
    auto const &ca = ensure_assumptions(v.expr());
    m_result = {};
    if (ca.contains(real_tag{}))
      m_result.insert(real_tag{});
  }

  void operator()(scalar_tan const &v) override {
    auto const &ca = ensure_assumptions(v.expr());
    m_result = {};
    if (ca.contains(real_tag{}))
      m_result.insert(real_tag{});
  }

  void operator()(scalar_asin const &v) override {
    auto const &ca = ensure_assumptions(v.expr());
    m_result = {};
    if (ca.contains(real_tag{}))
      m_result.insert(real_tag{});
  }

  void operator()(scalar_acos const &v) override {
    auto const &ca = ensure_assumptions(v.expr());
    m_result = {};
    if (ca.contains(real_tag{}))
      m_result.insert(real_tag{});
  }

  void operator()(scalar_atan const &v) override {
    auto const &ca = ensure_assumptions(v.expr());
    m_result = {};
    if (ca.contains(real_tag{}))
      m_result.insert(real_tag{});
  }

  // ─── Comparison nodes (#136) ─────────────────────────────────────
  // Indicator values: {0, 1} ⇒ real, integer, nonnegative.
  void operator()(scalar_lt const &v) override { handle_comparison(v); }
  void operator()(scalar_gt const &v) override { handle_comparison(v); }
  void operator()(scalar_le const &v) override { handle_comparison(v); }
  void operator()(scalar_ge const &v) override { handle_comparison(v); }
  void operator()(scalar_eq const &v) override { handle_comparison(v); }
  void operator()(scalar_ne const &v) override { handle_comparison(v); }

  // ─── Min / max (#137) ────────────────────────────────────────────
  // The result is real iff both operands are real (real numbers are
  // totally ordered, so min/max is well-defined). Conservatively
  // assume nothing beyond that.
  void operator()(scalar_max const &v) override {
    auto const &cl = ensure_assumptions(v.expr_lhs());
    auto const &cr = ensure_assumptions(v.expr_rhs());
    m_result = {};
    propagate_max_signs(cl, cr, m_result);
  }
  void operator()(scalar_min const &v) override {
    auto const &cl = ensure_assumptions(v.expr_lhs());
    auto const &cr = ensure_assumptions(v.expr_rhs());
    m_result = {};
    propagate_min_signs(cl, cr, m_result);
  }

  // ─── if_then_else (#135) ─────────────────────────────────────────
  // The result is real iff both branches are real. The condition
  // assumptions don't propagate (they describe a 0/1 indicator,
  // not the selected value).
  void operator()(scalar_if_then_else const &v) override {
    ensure_assumptions(v.expr_cond());
    auto const &ct = ensure_assumptions(v.expr_then());
    auto const &ce = ensure_assumptions(v.expr_else());
    m_result = {};
    propagate_if_then_else_intersection(ct, ce, m_result);
  }

private:
  template <typename BinaryNode> void handle_comparison(BinaryNode const &v) {
    ensure_assumptions(v.expr_lhs());
    ensure_assumptions(v.expr_rhs());
    set_indicator_assumptions(m_result);
  }

  numeric_assumption_manager m_result;
};

} // anonymous namespace

void infer_assumptions(expression_holder<scalar_expression> const &expr) {
  if (!expr.is_valid() || expr.data()->assumptions().inferred())
    return;
  shallow_inference_visitor v;
  v.run(expr);
  expr.data()->assumptions().set_inferred();
}

} // namespace numsim::cas
