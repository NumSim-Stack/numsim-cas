#ifndef FUZZYSCALARDIFFTEST_H
#define FUZZYSCALARDIFFTEST_H

// FuzzyScalarMachine — seeded random scalar expression tree generator.
// Derives from FuzzyDiffBase via CRTP.

#include "FuzzyDiffBase.h"

#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_diff.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/scalar/visitors/scalar_evaluator.h>

namespace numsim::cas {
namespace fuzzy_detail {

// ===========================================================================
// Scalar-specific types
// ===========================================================================
struct ScalarExprInfo {
  expression_holder<scalar_expression> expr;
  std::set<std::string> used_vars;
};

struct ScalarVarEntry {
  std::string name;
  expression_holder<scalar_expression> expr;
};

// ===========================================================================
// FuzzyScalarMachine
// ===========================================================================
class FuzzyScalarMachine
    : public FuzzyDiffBase<FuzzyScalarMachine, ScalarExprInfo> {
  using Base = FuzzyDiffBase<FuzzyScalarMachine, ScalarExprInfo>;
  friend Base;

public:
  explicit FuzzyScalarMachine(unsigned seed, std::size_t depth = 3)
      : Base(seed, depth) {
    create_variable_pool();
    register_default_ops();
  }

  // --- CRTP hooks ---

  ScalarExprInfo generate_leaf() {
    std::uniform_real_distribution<double> coin(0.0, 1.0);
    if (coin(this->m_rng) < 0.7) {
      std::uniform_int_distribution<std::size_t> dist(0, m_vars.size() - 1);
      auto idx = dist(this->m_rng);
      return {m_vars[idx].expr, {m_vars[idx].name}};
    }
    std::uniform_real_distribution<double> val_dist(0.5, 2.0);
    double v = val_dist(this->m_rng);
    return {make_expression<scalar_constant>(v), {}};
  }

  ScalarExprInfo negation_fallback(std::size_t depth) {
    auto sub = this->generate(depth - 1);
    return {-sub.expr, sub.used_vars};
  }

  std::vector<ScalarVarEntry> const &get_diff_vars() const { return m_vars; }

  static std::string get_var_name(ScalarVarEntry const &v) { return v.name; }

  static bool can_diff_wrt(ScalarExprInfo const &, ScalarVarEntry const &) {
    return true;
  }

  std::optional<expression_holder<scalar_expression>>
  do_diff(ScalarExprInfo const &info, ScalarVarEntry const &var) {
    auto d = diff(info.expr, var.expr);
    if (!d.is_valid())
      return std::nullopt;
    return d;
  }

  std::vector<ScalarVarEntry> const &vars() const { return m_vars; }

private:
  std::vector<ScalarVarEntry> m_vars;

  void create_variable_pool() {
    auto add_var = [&](std::string name) {
      auto expr = make_expression<scalar>(name);
      m_vars.push_back({std::move(name), expr});
    };
    add_var("x");
    add_var("y");
    add_var("z");
    add_var("w");
  }

  void register_default_ops() {
    this->add_op("addition", 20,
                 [](FuzzyScalarMachine &m,
                    std::size_t depth) -> std::optional<ScalarExprInfo> {
                   auto lhs = m.generate(depth - 1);
                   auto rhs = m.generate(depth - 1);
                   return ScalarExprInfo{
                       lhs.expr + rhs.expr,
                       Base::merge_vars(lhs.used_vars, rhs.used_vars)};
                 });

    this->add_op("multiplication", 20,
                 [](FuzzyScalarMachine &m,
                    std::size_t depth) -> std::optional<ScalarExprInfo> {
                   auto lhs = m.generate(depth - 1);
                   auto rhs = m.generate(depth - 1);
                   return ScalarExprInfo{
                       lhs.expr * rhs.expr,
                       Base::merge_vars(lhs.used_vars, rhs.used_vars)};
                 });

    this->add_op("negation", 8,
                 [](FuzzyScalarMachine &m,
                    std::size_t depth) -> std::optional<ScalarExprInfo> {
                   auto sub = m.generate(depth - 1);
                   return ScalarExprInfo{-sub.expr, sub.used_vars};
                 });

    this->add_op("const_mul", 8,
                 [](FuzzyScalarMachine &m,
                    std::size_t depth) -> std::optional<ScalarExprInfo> {
                   auto sub = m.generate(depth - 1);
                   int sv = m.pick_nonzero_scalar();
                   auto sc = make_scalar_constant(sv);
                   return ScalarExprInfo{sc * sub.expr, sub.used_vars};
                 });

    this->add_op("pow", 10,
                 [](FuzzyScalarMachine &m,
                    std::size_t depth) -> std::optional<ScalarExprInfo> {
                   auto sub = m.generate(depth - 1);
                   std::uniform_int_distribution<int> exp_dist(2, 3);
                   int exponent = exp_dist(m.rng());
                   auto expr = pow(sub.expr, make_scalar_constant(exponent));
                   return ScalarExprInfo{expr, sub.used_vars};
                 });

    this->add_op("sin", 10,
                 [](FuzzyScalarMachine &m,
                    std::size_t depth) -> std::optional<ScalarExprInfo> {
                   auto sub = m.generate(depth - 1);
                   return ScalarExprInfo{sin(sub.expr), sub.used_vars};
                 });

    this->add_op("cos", 10,
                 [](FuzzyScalarMachine &m,
                    std::size_t depth) -> std::optional<ScalarExprInfo> {
                   auto sub = m.generate(depth - 1);
                   return ScalarExprInfo{cos(sub.expr), sub.used_vars};
                 });

    this->add_op("exp", 5,
                 [](FuzzyScalarMachine &m,
                    std::size_t depth) -> std::optional<ScalarExprInfo> {
                   auto sub = m.generate(depth - 1);
                   return ScalarExprInfo{exp(sub.expr), sub.used_vars};
                 });

    this->add_op("division", 5,
                 [](FuzzyScalarMachine &m,
                    std::size_t depth) -> std::optional<ScalarExprInfo> {
                   auto lhs = m.generate(depth - 1);
                   auto rhs = m.generate(depth - 1);
                   return ScalarExprInfo{
                       lhs.expr / rhs.expr,
                       Base::merge_vars(lhs.used_vars, rhs.used_vars)};
                 });

    this->add_op("tan", 4,
                 [](FuzzyScalarMachine &m,
                    std::size_t depth) -> std::optional<ScalarExprInfo> {
                   auto sub = m.generate(depth - 1);
                   return ScalarExprInfo{tan(sub.expr), sub.used_vars};
                 });
  }

  // -----------------------------------------------------------------------
  // Verification
  // -----------------------------------------------------------------------
  VerifyResult verify(ScalarExprInfo const &info, ScalarVarEntry const &var,
                      expression_holder<scalar_expression> const &d) {
    std::mt19937 data_rng(this->m_seed + 0x9e3779b9u);
    std::uniform_real_distribution<double> val_dist(0.5, 2.0);

    scalar_evaluator<double> ev;
    struct VarData {
      std::string name;
      double value;
    };
    std::vector<VarData> var_data;

    for (auto const &v : m_vars) {
      double value = val_dist(data_rng);
      ev.set(v.expr, value);
      var_data.push_back({v.name, value});
    }

    double sym_val = ev.apply(d);
    if (!std::isfinite(sym_val))
      return {true, {}};

    double diff_var_value = 0;
    for (auto const &vd : var_data) {
      if (vd.name == var.name) {
        diff_var_value = vd.value;
        break;
      }
    }

    constexpr double h = 1e-5;
    ev.set(var.expr, diff_var_value + h);
    double f_plus = ev.apply(info.expr);
    ev.set(var.expr, diff_var_value - h);
    double f_minus = ev.apply(info.expr);
    ev.set(var.expr, diff_var_value);

    if (!std::isfinite(f_plus) || !std::isfinite(f_minus))
      return {true, {}};

    double num_val = (f_plus - f_minus) / (2.0 * h);

    if (!std::isfinite(num_val))
      return {true, {}};

    // Skip stiff expressions where finite differences are unreliable
    // (e.g., near tan() singularities)
    double mag = std::max(std::abs(sym_val), std::abs(num_val));
    if (mag > 1e5)
      return {true, {}};

    constexpr double abs_tol = 1e-6;
    constexpr double rel_tol = 1e-4;
    double err = std::abs(sym_val - num_val);

    if (err > abs_tol && err > rel_tol * mag) {
      std::ostringstream oss;
      oss << "symbolic vs numerical mismatch: sym=" << sym_val
          << " num=" << num_val << " err=" << err;
      return {false, oss.str()};
    }
    return {true, {}};
  }

  // -----------------------------------------------------------------------
  // Failure reporting
  // -----------------------------------------------------------------------
  void report_failure(std::string const &diagnostic, ScalarExprInfo const &info,
                      ScalarVarEntry const *var = nullptr,
                      expression_holder<scalar_expression> const * = nullptr) {
    std::ostringstream oss;
    oss << "seed=" << this->m_seed << " depth=" << this->m_depth;
    oss << "\n  " << diagnostic;
    if (info.expr.is_valid()) {
      try {
        oss << "\n  expr: " << to_string(info.expr);
      } catch (...) {
        oss << "\n  expr: <print error>";
      }
    }
    if (var)
      oss << "\n  var: " << var->name;
    if (!this->op_trace().empty())
      oss << "\n  ops: " << join(this->op_trace(), " -> ");
    oss << "\n  " << reproduction_command();
    ADD_FAILURE() << oss.str();
  }

  std::string reproduction_command() const {
    unsigned param =
        (this->m_depth == 4) ? this->m_seed - 20000u : this->m_seed;
    return "To reproduce: ./numsim_cas_fuzz_test "
           "--gtest_filter='FuzzyScalarSeeds/FuzzyScalarDiffTest.Depth" +
           std::to_string(this->m_depth) + "/" + std::to_string(param) + "'";
  }
};

} // namespace fuzzy_detail

namespace {

// ===========================================================================
// Value-parameterized test suite
// ===========================================================================
class FuzzyScalarDiffTest : public ::testing::TestWithParam<unsigned> {};

FUZZY_DIFF_TEST_P(FuzzyScalarDiffTest, Depth3, FuzzyScalarMachine, 0, 3)
FUZZY_DIFF_TEST_P(FuzzyScalarDiffTest, Depth4, FuzzyScalarMachine, 20000, 4)

INSTANTIATE_TEST_SUITE_P(FuzzyScalarSeeds, FuzzyScalarDiffTest,
                         ::testing::Range(1u, 101u));

} // namespace
} // namespace numsim::cas

#endif // FUZZYSCALARDIFFTEST_H
