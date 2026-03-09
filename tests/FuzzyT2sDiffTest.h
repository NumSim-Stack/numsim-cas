#ifndef FUZZYT2SDIFFTEST_H
#define FUZZYT2SDIFFTEST_H

// FuzzyT2sMachine — seeded random tensor-to-scalar expression tree generator.
// Derives from FuzzyDiffBase via CRTP.
// Supports pure mode (tensor variables only) and combined mode (+ scalars).

#include "FuzzyDiffBase.h"

#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/scalar/visitors/scalar_evaluator.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_diff.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_functions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_std.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_evaluator.h>

namespace numsim::cas {
namespace fuzzy_detail {

// ===========================================================================
// T2s-specific types
// ===========================================================================
struct T2sExprInfo {
  expression_holder<tensor_to_scalar_expression> expr;
  std::set<std::string> used_vars;
};

struct T2sVarEntry {
  std::string name;
  std::size_t rank;
  expression_holder<tensor_expression> expr;
};

struct T2sScalarVarEntry {
  std::string name;
  expression_holder<scalar_expression> expr;
};

// ===========================================================================
// FuzzyT2sMachine
// ===========================================================================
class FuzzyT2sMachine : public FuzzyDiffBase<FuzzyT2sMachine, T2sExprInfo> {
  using Base = FuzzyDiffBase<FuzzyT2sMachine, T2sExprInfo>;
  friend Base;

public:
  explicit FuzzyT2sMachine(unsigned seed, std::size_t depth = 3,
                           bool combined_mode = false)
      : Base(seed, depth), m_combined(combined_mode) {
    create_variable_pool();
    register_default_ops();
  }

  // --- CRTP hooks ---

  T2sExprInfo generate_leaf() {
    if (m_combined && !m_scalar_vars.empty()) {
      std::uniform_real_distribution<double> coin(0.0, 1.0);
      if (coin(this->m_rng) < 0.2) {
        std::uniform_int_distribution<std::size_t> dist(
            0, m_scalar_vars.size() - 1);
        auto idx = dist(this->m_rng);
        auto wrapped = make_expression<tensor_to_scalar_scalar_wrapper>(
            m_scalar_vars[idx].expr);
        return {wrapped, {m_scalar_vars[idx].name}};
      }
    }
    return generate_t2s_primitive();
  }

  T2sExprInfo negation_fallback(std::size_t depth) {
    auto sub = this->generate(depth - 1);
    return {-sub.expr, sub.used_vars};
  }

  std::vector<T2sVarEntry> const &get_diff_vars() const {
    return m_tensor_vars;
  }

  static std::string get_var_name(T2sVarEntry const &v) { return v.name; }

  static bool can_diff_wrt(T2sExprInfo const &, T2sVarEntry const &) {
    return true;
  }

  std::optional<expression_holder<tensor_expression>>
  do_diff(T2sExprInfo const &info, T2sVarEntry const &var) {
    auto d = diff(info.expr, var.expr);
    if (!d.is_valid())
      return std::nullopt;
    return d;
  }

  std::vector<T2sVarEntry> const &tensor_vars() const { return m_tensor_vars; }
  std::vector<T2sScalarVarEntry> const &scalar_vars() const {
    return m_scalar_vars;
  }

private:
  bool m_combined;
  std::vector<T2sVarEntry> m_tensor_vars;
  std::vector<T2sScalarVarEntry> m_scalar_vars;

  void create_variable_pool() {
    auto add_tensor = [&](std::string name, std::size_t rank) {
      auto expr = make_expression<tensor>(name, FDIM, rank);
      m_tensor_vars.push_back({std::move(name), rank, expr});
    };
    add_tensor("A", 2);
    add_tensor("B", 2);
    add_tensor("C", 2);

    if (m_combined) {
      auto add_scalar = [&](std::string name) {
        auto expr = make_expression<scalar>(name);
        m_scalar_vars.push_back({std::move(name), expr});
      };
      add_scalar("x");
      add_scalar("y");
    }
  }

  T2sExprInfo generate_t2s_primitive() {
    std::uniform_int_distribution<std::size_t> var_dist(
        0, m_tensor_vars.size() - 1);
    auto idx = var_dist(this->m_rng);
    auto const &tv = m_tensor_vars[idx];

    std::uniform_int_distribution<int> prim_dist(0, 3);
    int prim = prim_dist(this->m_rng);

    expression_holder<tensor_to_scalar_expression> expr;
    switch (prim) {
    case 0:
      expr = trace(tv.expr);
      break;
    case 1:
      expr = det(tv.expr);
      break;
    case 2:
      expr = norm(tv.expr);
      break;
    case 3:
      expr = dot(tv.expr);
      break;
    }
    return {expr, {tv.name}};
  }

  T2sExprInfo
  pick_random_tensor_var_apply(expression_holder<tensor_to_scalar_expression> (
      *fn)(expression_holder<tensor_expression> const &)) {
    std::uniform_int_distribution<std::size_t> dist(0,
                                                    m_tensor_vars.size() - 1);
    auto idx = dist(this->m_rng);
    auto const &tv = m_tensor_vars[idx];
    return {fn(tv.expr), {tv.name}};
  }

  void register_default_ops() {
    this->add_op(
        "trace", 15,
        [](FuzzyT2sMachine &m, std::size_t) -> std::optional<T2sExprInfo> {
          return m.pick_random_tensor_var_apply(trace);
        });

    this->add_op(
        "det", 10,
        [](FuzzyT2sMachine &m, std::size_t) -> std::optional<T2sExprInfo> {
          return m.pick_random_tensor_var_apply(det);
        });

    this->add_op(
        "norm", 10,
        [](FuzzyT2sMachine &m, std::size_t) -> std::optional<T2sExprInfo> {
          return m.pick_random_tensor_var_apply(norm);
        });

    this->add_op(
        "dot", 10,
        [](FuzzyT2sMachine &m, std::size_t) -> std::optional<T2sExprInfo> {
          return m.pick_random_tensor_var_apply(dot);
        });

    this->add_op(
        "dot_product", 8,
        [](FuzzyT2sMachine &m, std::size_t) -> std::optional<T2sExprInfo> {
          std::uniform_int_distribution<std::size_t> dist(
              0, m.m_tensor_vars.size() - 1);
          auto i1 = dist(m.rng());
          auto i2 = dist(m.rng());
          auto const &tv1 = m.m_tensor_vars[i1];
          auto const &tv2 = m.m_tensor_vars[i2];
          auto expr =
              dot_product(tv1.expr, sequence{1, 2}, tv2.expr, sequence{1, 2});
          return T2sExprInfo{expr, Base::merge_vars({tv1.name}, {tv2.name})};
        });

    this->add_op("t2s_add", 15,
                 [](FuzzyT2sMachine &m,
                    std::size_t depth) -> std::optional<T2sExprInfo> {
                   auto lhs = m.generate(depth - 1);
                   auto rhs = m.generate(depth - 1);
                   return T2sExprInfo{
                       lhs.expr + rhs.expr,
                       Base::merge_vars(lhs.used_vars, rhs.used_vars)};
                 });

    this->add_op("t2s_mul", 12,
                 [](FuzzyT2sMachine &m,
                    std::size_t depth) -> std::optional<T2sExprInfo> {
                   auto lhs = m.generate(depth - 1);
                   auto rhs = m.generate(depth - 1);
                   return T2sExprInfo{
                       lhs.expr * rhs.expr,
                       Base::merge_vars(lhs.used_vars, rhs.used_vars)};
                 });

    this->add_op("t2s_neg", 5,
                 [](FuzzyT2sMachine &m,
                    std::size_t depth) -> std::optional<T2sExprInfo> {
                   auto sub = m.generate(depth - 1);
                   return T2sExprInfo{-sub.expr, sub.used_vars};
                 });

    this->add_op("t2s_const_mul", 5,
                 [](FuzzyT2sMachine &m,
                    std::size_t depth) -> std::optional<T2sExprInfo> {
                   auto sub = m.generate(depth - 1);
                   int sv = m.pick_nonzero_scalar();
                   auto sc = make_scalar_constant(sv);
                   auto wrapped =
                       make_expression<tensor_to_scalar_scalar_wrapper>(sc);
                   return T2sExprInfo{wrapped * sub.expr, sub.used_vars};
                 });

    this->add_op("t2s_pow", 8,
                 [](FuzzyT2sMachine &m,
                    std::size_t depth) -> std::optional<T2sExprInfo> {
                   auto sub = m.generate(depth - 1);
                   std::uniform_int_distribution<int> exp_dist(2, 3);
                   int exponent = exp_dist(m.rng());
                   auto expr = pow(sub.expr, exponent);
                   return T2sExprInfo{expr, sub.used_vars};
                 });

    this->add_op("t2s_exp", 3,
                 [](FuzzyT2sMachine &m,
                    std::size_t depth) -> std::optional<T2sExprInfo> {
                   auto sub = m.generate(depth - 1);
                   return T2sExprInfo{exp(sub.expr), sub.used_vars};
                 });

    this->add_op("t2s_sqrt", 4,
                 [](FuzzyT2sMachine &m,
                    std::size_t depth) -> std::optional<T2sExprInfo> {
                   auto sub = m.generate(depth - 1);
                   auto one = make_expression<tensor_to_scalar_one>();
                   auto guarded = sub.expr * sub.expr + one;
                   return T2sExprInfo{sqrt(guarded), sub.used_vars};
                 });

    if (m_combined && !m_scalar_vars.empty()) {
      this->add_op(
          "scalar_wrap", 10,
          [](FuzzyT2sMachine &m, std::size_t) -> std::optional<T2sExprInfo> {
            std::uniform_int_distribution<std::size_t> dist(
                0, m.m_scalar_vars.size() - 1);
            auto idx = dist(m.rng());
            auto const &sv = m.m_scalar_vars[idx];
            auto wrapped =
                make_expression<tensor_to_scalar_scalar_wrapper>(sv.expr);
            return T2sExprInfo{wrapped, {sv.name}};
          });

      this->add_op("scalar_times_t2s", 8,
                   [](FuzzyT2sMachine &m,
                      std::size_t depth) -> std::optional<T2sExprInfo> {
                     std::uniform_int_distribution<std::size_t> dist(
                         0, m.m_scalar_vars.size() - 1);
                     auto idx = dist(m.rng());
                     auto const &sv = m.m_scalar_vars[idx];
                     auto t2s_sub = m.generate(depth - 1);
                     auto expr = sv.expr * t2s_sub.expr;
                     auto vars = t2s_sub.used_vars;
                     vars.insert(sv.name);
                     return T2sExprInfo{expr, vars};
                   });

      this->add_op("scalar_plus_t2s", 8,
                   [](FuzzyT2sMachine &m,
                      std::size_t depth) -> std::optional<T2sExprInfo> {
                     std::uniform_int_distribution<std::size_t> dist(
                         0, m.m_scalar_vars.size() - 1);
                     auto idx = dist(m.rng());
                     auto const &sv = m.m_scalar_vars[idx];
                     auto t2s_sub = m.generate(depth - 1);
                     auto expr = sv.expr + t2s_sub.expr;
                     auto vars = t2s_sub.used_vars;
                     vars.insert(sv.name);
                     return T2sExprInfo{expr, vars};
                   });
    }
  }

  // -----------------------------------------------------------------------
  // Verification
  // -----------------------------------------------------------------------
  VerifyResult verify(T2sExprInfo const &info, T2sVarEntry const &var,
                      expression_holder<tensor_expression> const &d) {
    std::mt19937 data_rng(this->m_seed + 0x9e3779b9u);

    tensor_to_scalar_evaluator<double> t2s_ev;
    tensor_evaluator<double> tensor_ev;

    struct TensorVarData {
      std::string name;
      std::shared_ptr<tensor_data_base<double>> ptr;
    };
    std::vector<TensorVarData> tensor_var_data;

    for (auto const &tv : m_tensor_vars) {
      tmech::tensor<double, FDIM, 2> t;
      fill_random(t, data_rng);
      auto ptr = std::make_shared<tensor_data<double, FDIM, 2>>(t);
      t2s_ev.set(tv.expr, ptr);
      tensor_ev.set(tv.expr, ptr);
      tensor_var_data.push_back({tv.name, ptr});
    }

    std::uniform_real_distribution<double> sval_dist(0.5, 2.0);
    for (auto const &sv : m_scalar_vars) {
      double value = sval_dist(data_rng);
      t2s_ev.set_scalar(sv.expr, value);
      tensor_ev.set_scalar(sv.expr, value);
    }

    auto sym_result = tensor_ev.apply(d);
    if (!sym_result)
      return {false, "evaluator returned nullptr for derivative"};
    if (sym_result->rank() != var.rank) {
      std::ostringstream oss;
      oss << "expected rank " << var.rank << " got " << sym_result->rank();
      return {false, oss.str()};
    }
    auto const &sym_tensor = fuzzy_as_tmech<FDIM, 2>(*sym_result);

    std::shared_ptr<tensor_data_base<double>> diff_var_ptr;
    for (auto const &tvd : tensor_var_data) {
      if (tvd.name == var.name) {
        diff_var_ptr = tvd.ptr;
        break;
      }
    }
    if (!diff_var_ptr)
      return {false, "diff variable data not found"};

    auto &var_tmech =
        static_cast<tensor_data<double, FDIM, 2> &>(*diff_var_ptr).data();

    constexpr std::size_t n_components = FDIM * FDIM;
    constexpr double h = 1e-5;
    tmech::tensor<double, FDIM, 2> num_gradient;

    auto *var_ptr = var_tmech.raw_data();
    auto *grad_ptr = num_gradient.raw_data();

    double max_fval = 0;
    for (std::size_t k = 0; k < n_components; ++k) {
      double original = var_ptr[k];
      var_ptr[k] = original + h;
      double f_plus = t2s_ev.apply(info.expr);
      var_ptr[k] = original - h;
      double f_minus = t2s_ev.apply(info.expr);
      var_ptr[k] = original;

      if (!std::isfinite(f_plus) || !std::isfinite(f_minus))
        return {true, {}};

      max_fval =
          std::max(max_fval, std::max(std::abs(f_plus), std::abs(f_minus)));
      grad_ptr[k] = (f_plus - f_minus) / (2.0 * h);
    }

    auto const *sym_ptr = sym_tensor.raw_data();
    auto const *num_ptr = num_gradient.raw_data();

    // Cancellation noise: when |f| >> |f'|*h, subtracting two large
    // values loses precision. The noise floor from cancellation is
    // approximately eps * |f| / h.
    constexpr double eps = std::numeric_limits<double>::epsilon();
    double cancellation_noise = eps * max_fval / h;

    for (std::size_t i = 0; i < n_components; ++i) {
      if (!std::isfinite(sym_ptr[i]))
        return {true, {}};
    }

    auto cmp = compare_arrays(sym_ptr, num_ptr, n_components, 5e-6, 1e-4,
                              cancellation_noise);
    if (!cmp.ok) {
      double rel_err =
          cmp.max_abs > 0 ? cmp.max_err / cmp.max_abs : cmp.max_err;
      std::ostringstream oss;
      oss << "symbolic vs numerical mismatch: max_err=" << cmp.max_err
          << " rel_err=" << rel_err;
      return {false, oss.str()};
    }
    return {true, {}};
  }

  // -----------------------------------------------------------------------
  // Failure reporting
  // -----------------------------------------------------------------------
  void
  report_failure(std::string const &diagnostic, T2sExprInfo const &info,
                 T2sVarEntry const *var = nullptr,
                 expression_holder<tensor_expression> const *deriv = nullptr) {
    std::ostringstream oss;
    oss << "seed=" << this->m_seed << " depth=" << this->m_depth
        << " mode=" << (m_combined ? "combined" : "pure");
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
    if (deriv)
      oss << "\n  deriv: " << tensor_expr_string(*deriv);
    if (!this->op_trace().empty())
      oss << "\n  ops: " << join(this->op_trace(), " -> ");
    oss << "\n  " << reproduction_command();
    ADD_FAILURE() << oss.str();
  }

  std::string reproduction_command() const {
    std::string filter_prefix =
        m_combined ? "FuzzyT2sCombinedSeeds" : "FuzzyT2sPureSeeds";
    std::string test_prefix = m_combined ? "CombinedDepth" : "PureT2sDepth";
    unsigned base_offset;
    if (m_combined)
      base_offset = (this->m_depth == 4) ? 60000u : 40000u;
    else
      base_offset = (this->m_depth == 4) ? 50000u : 30000u;
    unsigned param = this->m_seed - base_offset;
    return "To reproduce: ./numsim_cas_fuzz_test "
           "--gtest_filter='" +
           filter_prefix + "/FuzzyT2sDiffTest." + test_prefix +
           std::to_string(this->m_depth) + "/" + std::to_string(param) + "'";
  }
};

} // namespace fuzzy_detail

namespace {

// ===========================================================================
// Value-parameterized test suites
// ===========================================================================

// --- Pure T2s ---
class FuzzyT2sPureDiffTest : public ::testing::TestWithParam<unsigned> {};

FUZZY_DIFF_TEST_P(FuzzyT2sPureDiffTest, PureT2sDepth3, FuzzyT2sMachine, 30000,
                  3, false)
FUZZY_DIFF_TEST_P(FuzzyT2sPureDiffTest, PureT2sDepth4, FuzzyT2sMachine, 50000,
                  4, false)

INSTANTIATE_TEST_SUITE_P(FuzzyT2sPureSeeds, FuzzyT2sPureDiffTest,
                         ::testing::Range(1u, 101u));

// --- Combined T2s ---
class FuzzyT2sCombinedDiffTest : public ::testing::TestWithParam<unsigned> {};

FUZZY_DIFF_TEST_P(FuzzyT2sCombinedDiffTest, CombinedDepth3, FuzzyT2sMachine,
                  40000, 3, true)
FUZZY_DIFF_TEST_P(FuzzyT2sCombinedDiffTest, CombinedDepth4, FuzzyT2sMachine,
                  60000, 4, true)

INSTANTIATE_TEST_SUITE_P(FuzzyT2sCombinedSeeds, FuzzyT2sCombinedDiffTest,
                         ::testing::Range(1u, 101u));

} // namespace
} // namespace numsim::cas

#endif // FUZZYT2SDIFFTEST_H
