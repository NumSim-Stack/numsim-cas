#include <numsim_cas/scalar/visitors/scalar_assumption_propagator.h>

#include <numsim_cas/scalar/scalar_assume.h>
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
  double val = std::visit(
      [](auto const &x) -> double {
        using V = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<V, std::complex<double>>) {
          return x.real();
        } else {
          return static_cast<double>(x);
        }
      },
      v.value().raw());

  m_result.insert(real_tag{});
  if (is_int)
    m_result.insert(integer{});

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

  for (auto const &child : v.hash_map() | std::views::values) {
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

  for (auto const &child : v.hash_map() | std::views::values) {
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

  // Check if exponent is an even integer constant
  bool exp_even = false;
  if (is_same<scalar_constant>(v.expr_rhs())) {
    auto const &cv = v.expr_rhs().template get<scalar_constant>().value();
    if (std::holds_alternative<std::int64_t>(cv.raw())) {
      auto iv = std::get<std::int64_t>(cv.raw());
      exp_even = (iv % 2 == 0);
    }
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

void scalar_assumption_propagator::operator()(scalar_rational const &v) {
  auto num_a = apply(v.expr_lhs());
  auto den_a = apply(v.expr_rhs());

  m_result = {};
  if (num_a.contains(real_tag{}) && den_a.contains(real_tag{}))
    m_result.insert(real_tag{});

  bool num_pos = num_a.contains(positive{});
  bool num_neg = num_a.contains(negative{});
  bool den_pos = den_a.contains(positive{});
  bool den_neg = den_a.contains(negative{});

  if ((num_pos && den_pos) || (num_neg && den_neg)) {
    m_result.insert(positive{});
    m_result.insert(nonnegative{});
    m_result.insert(nonzero{});
  } else if ((num_pos && den_neg) || (num_neg && den_pos)) {
    m_result.insert(negative{});
    m_result.insert(nonpositive{});
    m_result.insert(nonzero{});
  }
  if (num_a.contains(nonnegative{}) && den_a.contains(positive{}))
    m_result.insert(nonnegative{});
  if (num_a.contains(nonpositive{}) && den_a.contains(positive{}))
    m_result.insert(nonpositive{});
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
    [[maybe_unused]] scalar_function const &) {
  m_result = {};
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
    double val = std::visit(
        [](auto const &x) -> double {
          using V = std::decay_t<decltype(x)>;
          if constexpr (std::is_same_v<V, std::complex<double>>)
            return x.real();
          else
            return static_cast<double>(x);
        },
        cv.raw());
    m_result.insert(real_tag{});
    if (is_int)
      m_result.insert(integer{});
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
    }
  }

  void operator()([[maybe_unused]] scalar_function const &) override {
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
    for (auto const &child : v.hash_map() | std::views::values) {
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
    for (auto const &child : v.hash_map() | std::views::values) {
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
    if (is_same<scalar_constant>(v.expr_rhs())) {
      auto const &cv = v.expr_rhs().template get<scalar_constant>().value();
      if (std::holds_alternative<std::int64_t>(cv.raw()))
        exp_even = (std::get<std::int64_t>(cv.raw()) % 2 == 0);
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

  void operator()(scalar_rational const &v) override {
    auto const &num_a = ensure_assumptions(v.expr_lhs());
    auto const &den_a = ensure_assumptions(v.expr_rhs());
    m_result = {};
    if (num_a.contains(real_tag{}) && den_a.contains(real_tag{}))
      m_result.insert(real_tag{});
    bool num_pos = num_a.contains(positive{});
    bool num_neg = num_a.contains(negative{});
    bool den_pos = den_a.contains(positive{});
    bool den_neg = den_a.contains(negative{});
    if ((num_pos && den_pos) || (num_neg && den_neg)) {
      m_result.insert(positive{});
      m_result.insert(nonnegative{});
      m_result.insert(nonzero{});
    } else if ((num_pos && den_neg) || (num_neg && den_pos)) {
      m_result.insert(negative{});
      m_result.insert(nonpositive{});
      m_result.insert(nonzero{});
    }
    if (num_a.contains(nonnegative{}) && den_a.contains(positive{}))
      m_result.insert(nonnegative{});
    if (num_a.contains(nonpositive{}) && den_a.contains(positive{}))
      m_result.insert(nonpositive{});
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

private:
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
