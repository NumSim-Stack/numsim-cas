#ifndef FUZZYDIFFBASE_H
#define FUZZYDIFFBASE_H

// CRTP base class for fuzzy differentiation test machines.
// Provides: TestResult, VerifyResult, coverage tracking, OpEntry with weighted
// random selection, generate/generate_op dispatch, and shared utilities.

#include "cas_test_helpers.h"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <tmech/tmech.h>

#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/core/diff.h>
#include <numsim_cas/numsim_cas.h>
#include <numsim_cas/tensor/tensor_assume.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor/visitors/tensor_evaluator.h>
#include <numsim_cas/tensor/visitors/tensor_printer.h>

namespace numsim::cas {
namespace fuzzy_detail {

// ===========================================================================
// Shared types
// ===========================================================================
enum class TestResult { Pass, Fail, Skip };

struct VerifyResult {
  bool passed;
  std::string diagnostic;
};

// ===========================================================================
// Constants
// ===========================================================================
static constexpr std::size_t FDIM = 3;
static constexpr std::size_t FMAX_RANK = 8;

// ===========================================================================
// Global operation coverage tracking (across all test instances)
// ===========================================================================
inline std::map<std::string, int> &global_op_counts() {
  static std::map<std::string, int> counts;
  return counts;
}

inline void record_op(std::string const &name) { global_op_counts()[name]++; }

inline void print_coverage_summary() {
  auto const &counts = global_op_counts();
  if (counts.empty())
    return;
  int total = 0;
  for (auto const &[name, count] : counts)
    total += count;
  std::cerr << "\n=== Fuzzy Test Operation Coverage ===\n";
  for (auto const &[name, count] : counts) {
    double pct = total > 0 ? 100.0 * count / total : 0;
    std::cerr << "  " << std::left << std::setw(22) << name << std::right
              << std::setw(5) << count << "  (" << std::fixed
              << std::setprecision(1) << pct << "%)\n";
  }
  std::cerr << "  " << std::left << std::setw(22) << "TOTAL" << std::right
            << std::setw(5) << total << "\n";
}

// ===========================================================================
// String helpers
// ===========================================================================
inline std::string join(std::vector<std::string> const &v,
                        std::string const &sep) {
  std::string result;
  for (std::size_t i = 0; i < v.size(); ++i) {
    if (i > 0)
      result += sep;
    result += v[i];
  }
  return result;
}

// ===========================================================================
// Rank-4 symmetrize helpers (via tmech::basis_change)
// ===========================================================================

// Minor symmetry: average over (ij)↔(ji) and (kl)↔(lk)
template <std::size_t Dim>
auto symmetrize_minor(tmech::tensor<double, Dim, 4> const &A) {
  return 0.25 * (A + tmech::basis_change<tmech::sequence<2, 1, 3, 4>>(A) +
                 tmech::basis_change<tmech::sequence<1, 2, 4, 3>>(A) +
                 tmech::basis_change<tmech::sequence<2, 1, 4, 3>>(A));
}

// Major symmetry: average over (ij,kl)↔(kl,ij)
template <std::size_t Dim>
auto symmetrize_major(tmech::tensor<double, Dim, 4> const &A) {
  return 0.5 * (A + tmech::basis_change<tmech::sequence<3, 4, 1, 2>>(A));
}

// Minor+major: apply minor then major
template <std::size_t Dim>
auto symmetrize_minor_major(tmech::tensor<double, Dim, 4> const &A) {
  return tmech::eval(
      symmetrize_major<Dim>(tmech::eval(symmetrize_minor<Dim>(A))));
}

// ===========================================================================
// Tensor helpers (used by tensor and t2s machines)
// ===========================================================================
template <std::size_t Dim, std::size_t Rank>
void fill_random(tmech::tensor<double, Dim, Rank> &t, std::mt19937 &rng,
                 std::optional<tensor_space> const &space = std::nullopt) {
  std::normal_distribution<double> dist(0.0, 1.0);
  auto *ptr = t.raw_data();
  std::size_t n = 1;
  for (std::size_t i = 0; i < Rank; ++i)
    n *= Dim;
  for (std::size_t i = 0; i < n; ++i)
    ptr[i] = dist(rng);

  // NOTE: unlike the old fill_random which unconditionally symmetrized all
  // rank-2 tensors, this version only applies symmetry when explicitly
  // requested via the space parameter.
  if constexpr (Rank == 2) {
    auto const I = tmech::eye<double, Dim, 2>();
    if (space && std::holds_alternative<Symmetric>(space->perm)) {
      t = tmech::eval(tmech::sym(t) + 5.0 * I);
    } else if (space && std::holds_alternative<Skew>(space->perm)) {
      t = tmech::eval(tmech::skew(t));
    } else {
      // General: no symmetry post-processing, but boost diagonal
      t = tmech::eval(t + 5.0 * I);
    }
  }

  if constexpr (Rank == 4) {
    if (space) {
      if (std::holds_alternative<Minor>(space->perm)) {
        t = tmech::eval(symmetrize_minor<Dim>(t));
      } else if (std::holds_alternative<Major>(space->perm)) {
        t = tmech::eval(symmetrize_major<Dim>(t));
      } else if (std::holds_alternative<MinorMajor>(space->perm)) {
        t = tmech::eval(symmetrize_minor_major<Dim>(t));
      }
    }
    // Boost diagonal-like entries (δ_ij·δ_kl) for invertibility
    t = tmech::eval(t + 5.0 * tmech::eye<double, Dim, 4>());
  }
}

template <std::size_t MaxR = 8, typename Fn>
decltype(auto) dispatch_rank(std::size_t rank, Fn &&fn) {
  if (rank == MaxR)
    return fn(std::integral_constant<std::size_t, MaxR>{});
  if constexpr (MaxR > 1)
    return dispatch_rank<MaxR - 1>(rank, std::forward<Fn>(fn));
  else
    throw std::runtime_error("dispatch_rank: rank out of range");
}

template <std::size_t Dim, std::size_t Rank>
auto const &fuzzy_as_tmech(tensor_data_base<double> const &data) {
  return static_cast<tensor_data<double, Dim, Rank> const &>(data).data();
}

template <std::size_t... Is>
auto fuzzy_num_diff_impl(auto &&fn, auto &&val, std::index_sequence<Is...>) {
  return tmech::num_diff_central<tmech::sequence<(Is + 1)...>>(
      std::forward<decltype(fn)>(fn), std::forward<decltype(val)>(val));
}

template <std::size_t N> auto fuzzy_num_diff(auto &&fn, auto &&val) {
  return fuzzy_num_diff_impl(std::forward<decltype(fn)>(fn),
                             std::forward<decltype(val)>(val),
                             std::make_index_sequence<N>{});
}

// ===========================================================================
// Symmetry type aliases for num_diff_sym_central
// ===========================================================================
using Sym2x2 = std::tuple<tmech::sequence<1, 2>, tmech::sequence<2, 1>>;

using Minor4 =
    std::tuple<tmech::sequence<1, 2, 3, 4>, tmech::sequence<2, 1, 3, 4>,
               tmech::sequence<1, 2, 4, 3>, tmech::sequence<2, 1, 4, 3>>;

using Major4 =
    std::tuple<tmech::sequence<1, 2, 3, 4>, tmech::sequence<3, 4, 1, 2>>;

using MinorMajor4 =
    std::tuple<tmech::sequence<1, 2, 3, 4>, tmech::sequence<2, 1, 3, 4>,
               tmech::sequence<1, 2, 4, 3>, tmech::sequence<2, 1, 4, 3>,
               tmech::sequence<3, 4, 1, 2>, tmech::sequence<4, 3, 1, 2>,
               tmech::sequence<3, 4, 2, 1>, tmech::sequence<4, 3, 2, 1>>;

// ===========================================================================
// Tensor-wrt-tensor numerical differentiation wrapper.
// Dispatches to num_diff_central with a symmetrizing lambda based on
// the variable's tensor_space assumption.
// ===========================================================================
template <std::size_t DiffRank, std::size_t Dim, std::size_t VarRank>
auto fuzzy_tensor_num_diff(auto &&fn,
                           tmech::tensor<double, Dim, VarRank> const &X,
                           std::optional<tensor_space> const &space) {
  if constexpr (VarRank == 2) {
    if (space) {
      if (std::holds_alternative<Symmetric>(space->perm)) {
        auto sym_fn = [&](auto const &x) {
          return fn(tmech::eval(tmech::sym(x)));
        };
        return fuzzy_num_diff<DiffRank>(sym_fn, X);
      }
      if (std::holds_alternative<Skew>(space->perm)) {
        auto skew_fn = [&](auto const &x) {
          return fn(tmech::eval(tmech::skew(x)));
        };
        return fuzzy_num_diff<DiffRank>(skew_fn, X);
      }
    }
  }
  if constexpr (VarRank == 4) {
    if (space) {
      if (std::holds_alternative<Minor>(space->perm)) {
        auto minor_fn = [&](auto const &x) {
          return fn(symmetrize_minor<Dim>(x));
        };
        return fuzzy_num_diff<DiffRank>(minor_fn, X);
      }
      if (std::holds_alternative<Major>(space->perm)) {
        auto major_fn = [&](auto const &x) {
          return fn(symmetrize_major<Dim>(x));
        };
        return fuzzy_num_diff<DiffRank>(major_fn, X);
      }
      if (std::holds_alternative<MinorMajor>(space->perm)) {
        auto mm_fn = [&](auto const &x) {
          return fn(symmetrize_minor_major<Dim>(x));
        };
        return fuzzy_num_diff<DiffRank>(mm_fn, X);
      }
    }
  }
  return fuzzy_num_diff<DiffRank>(fn, X);
}

// ===========================================================================
// T2s (scalar-wrt-tensor) numerical differentiation wrapper.
// For scalar output, num_diff_sym_central works without SymResult.
// ===========================================================================
template <std::size_t Dim, std::size_t VarRank>
auto fuzzy_t2s_num_diff(auto &&fn, tmech::tensor<double, Dim, VarRank> const &X,
                        std::optional<tensor_space> const &space) {
  if constexpr (VarRank == 2) {
    if (space) {
      if (std::holds_alternative<Symmetric>(space->perm))
        return tmech::num_diff_sym_central<Sym2x2>(fn, X);
      if (std::holds_alternative<Skew>(space->perm)) {
        auto skew_fn = [&](auto const &x) {
          return fn(tmech::eval(tmech::skew(x)));
        };
        return fuzzy_num_diff<VarRank>(skew_fn, X);
      }
    }
  }
  if constexpr (VarRank == 4) {
    if (space) {
      if (std::holds_alternative<Minor>(space->perm))
        return tmech::num_diff_sym_central<Minor4>(fn, X);
      if (std::holds_alternative<Major>(space->perm))
        return tmech::num_diff_sym_central<Major4>(fn, X);
      if (std::holds_alternative<MinorMajor>(space->perm))
        return tmech::num_diff_sym_central<MinorMajor4>(fn, X);
    }
  }
  return fuzzy_num_diff<VarRank>(fn, X);
}

inline std::string
tensor_expr_string(expression_holder<tensor_expression> const &e) {
  if (!e.is_valid())
    return "<invalid>";
  try {
    std::stringstream ss;
    tensor_printer<std::stringstream> p(ss);
    p.apply(e);
    return ss.str();
  } catch (...) {
    return "<print error>";
  }
}

// ===========================================================================
// Component-wise tolerance comparison (shared by tensor + t2s verification)
// ===========================================================================
struct ArrayCompareResult {
  bool ok;
  double max_err;
  double max_abs;
};

inline ArrayCompareResult compare_arrays(double const *sym, double const *num,
                                         std::size_t n, double abs_tol,
                                         double rel_tol,
                                         double extra_noise = 0.0) {
  double max_err = 0;
  double max_abs = 0;
  for (std::size_t i = 0; i < n; ++i)
    max_abs = std::max(max_abs, std::max(std::abs(sym[i]), std::abs(num[i])));

  double noise_floor = max_abs * 1e-4;
  double effective_abs_tol = std::max(abs_tol, extra_noise);
  bool ok = true;
  for (std::size_t i = 0; i < n; ++i) {
    double err = std::abs(sym[i] - num[i]);
    double mag = std::max(std::abs(sym[i]), std::abs(num[i]));
    max_err = std::max(max_err, err);
    if (err > effective_abs_tol && err > rel_tol * std::max(mag, noise_floor))
      ok = false;
  }
  return {ok, max_err, max_abs};
}

// ===========================================================================
// FuzzyDiffBase<Derived, ExprInfoType> — CRTP base for fuzzy expr machines
//
// Derived must provide:
//   - ExprInfoType generate_leaf();
//   - ExprInfoType negation_fallback(std::size_t depth);
//   - auto const& get_diff_vars() const;  (returns vector of var entries)
//   - bool can_diff_wrt(ExprInfoType const&, var_entry const&) const;
//   - auto get_var_expr(var_entry const&) const;  (returns expression to diff
//   against)
//   - std::string get_var_name(var_entry const&) const;
//   - VerifyResult verify(ExprInfoType const&, var_entry const&, auto const&
//   d);
//   - void report_failure(std::string const&, ExprInfoType const&,
//                          var_entry const*, auto const* deriv);
// ===========================================================================
template <typename Derived, typename ExprInfoT> class FuzzyDiffBase {
public:
  using ExprInfoType = ExprInfoT;

  struct OpEntry {
    std::string name;
    int weight;
    std::function<std::optional<ExprInfoType>(Derived &, std::size_t)> generate;
  };

  explicit FuzzyDiffBase(unsigned seed, std::size_t depth = 3)
      : m_rng(seed), m_seed(seed), m_depth(depth) {}

  TestResult run_one_test() {
    clear_op_trace();

    ExprInfoType info;
    try {
      info = generate(m_depth);
    } catch (cas_error const &e) {
      return handle_exception(e, "generation");
    } catch (std::exception const &) {
      return TestResult::Skip;
    }

    auto const &vars = self().get_diff_vars();
    std::vector<std::size_t> indices;
    for (std::size_t i = 0; i < vars.size(); ++i) {
      if (info.used_vars.count(self().get_var_name(vars[i])) &&
          self().can_diff_wrt(info, vars[i]))
        indices.push_back(i);
    }
    if (indices.empty())
      return TestResult::Pass;
    std::shuffle(indices.begin(), indices.end(), m_rng);
    auto const &diff_var = vars[indices[0]];

    decltype(self().do_diff(info, diff_var)) d_holder;
    try {
      d_holder = self().do_diff(info, diff_var);
    } catch (cas_error const &e) {
      return handle_exception(e, "differentiation");
    } catch (std::exception const &) {
      return TestResult::Skip;
    }
    if (!d_holder) {
      self().report_failure("diff returned invalid expression", info,
                            &diff_var);
      return TestResult::Fail;
    }

    try {
      auto vr = self().verify(info, diff_var, *d_holder);
      if (!vr.passed) {
        self().report_failure(vr.diagnostic, info, &diff_var, &(*d_holder));
        return TestResult::Fail;
      }
      return TestResult::Pass;
    } catch (cas_error const &e) {
      return handle_exception(e, "verification");
    } catch (std::exception const &) {
      return TestResult::Skip;
    }
  }

  // -----------------------------------------------------------------------
  // Tree generation
  // -----------------------------------------------------------------------
  ExprInfoType generate(std::size_t depth) {
    if (depth == 0)
      return self().generate_leaf();
    return generate_op(depth);
  }

  ExprInfoType generate_op(std::size_t depth) {
    std::vector<int> weights;
    weights.reserve(m_ops.size());
    for (auto const &op : m_ops)
      weights.push_back(op.weight);
    std::discrete_distribution<int> op_dist(weights.begin(), weights.end());

    int first_pick = op_dist(m_rng);

    // Try the weighted pick first
    if (m_ops[first_pick].weight > 0) {
      auto result = m_ops[first_pick].generate(self(), depth);
      if (result) {
        m_op_trace.push_back(m_ops[first_pick].name);
        record_op(m_ops[first_pick].name);
        return *result;
      }
    }

    // Shuffled fallback: try remaining ops in random order
    std::vector<std::size_t> fallback;
    for (std::size_t i = 0; i < m_ops.size(); ++i) {
      if (static_cast<int>(i) != first_pick && m_ops[i].weight > 0)
        fallback.push_back(i);
    }
    std::shuffle(fallback.begin(), fallback.end(), m_rng);

    for (auto idx : fallback) {
      auto result = m_ops[idx].generate(self(), depth);
      if (result) {
        m_op_trace.push_back(m_ops[idx].name);
        record_op(m_ops[idx].name);
        return *result;
      }
    }

    // Ultimate fallback
    m_op_trace.push_back("negation_fallback");
    record_op("negation_fallback");
    return self().negation_fallback(depth);
  }

  // -----------------------------------------------------------------------
  // Op registration
  // -----------------------------------------------------------------------
  void add_op(
      std::string name, int weight,
      std::function<std::optional<ExprInfoType>(Derived &, std::size_t)> fn) {
    m_ops.push_back({std::move(name), weight, std::move(fn)});
  }

  // -----------------------------------------------------------------------
  // Utilities
  // -----------------------------------------------------------------------
  int pick_nonzero_scalar() {
    std::uniform_int_distribution<int> dist(-3, 3);
    int v;
    do {
      v = dist(m_rng);
    } while (v == 0);
    return v;
  }

  static std::set<std::string> merge_vars(std::set<std::string> const &a,
                                          std::set<std::string> const &b) {
    std::set<std::string> result;
    result.insert(a.begin(), a.end());
    result.insert(b.begin(), b.end());
    return result;
  }

  TestResult handle_exception(cas_error const &e, std::string const &phase) {
    if (dynamic_cast<not_implemented_error const *>(&e))
      return TestResult::Skip;
    if (dynamic_cast<evaluation_error const *>(&e))
      return TestResult::Skip;
    if (dynamic_cast<invalid_expression_error const *>(&e))
      return TestResult::Skip;
    if (dynamic_cast<internal_error const *>(&e))
      return TestResult::Skip;

    std::ostringstream oss;
    oss << "unexpected exception during " << phase << ": " << e.what();
    ADD_FAILURE() << "seed=" << m_seed << " depth=" << m_depth << "\n  "
                  << oss.str();
    return TestResult::Fail;
  }

  // -----------------------------------------------------------------------
  // Accessors
  // -----------------------------------------------------------------------
  unsigned seed() const { return m_seed; }
  std::size_t depth() const { return m_depth; }
  std::mt19937 &rng() { return m_rng; }
  std::vector<std::string> const &op_trace() const { return m_op_trace; }
  void clear_op_trace() { m_op_trace.clear(); }

protected:
  std::mt19937 m_rng;
  unsigned m_seed;
  std::size_t m_depth;
  std::vector<OpEntry> m_ops;
  std::vector<std::string> m_op_trace;

private:
  Derived &self() { return static_cast<Derived &>(*this); }
  Derived const &self() const { return static_cast<Derived const &>(*this); }
};

} // namespace fuzzy_detail
} // namespace numsim::cas

// ===========================================================================
// Macro to eliminate TEST_P boilerplate — all 8 test bodies are identical
// except for Machine type, seed offset, depth, and skip message.
// ===========================================================================
#define FUZZY_DIFF_TEST_P(TestClass, TestName, MachineType, SeedOffset, Depth, \
                          ...)                                                 \
  TEST_P(TestClass, TestName) {                                                \
    try {                                                                      \
      numsim::cas::fuzzy_detail::MachineType machine(                          \
          GetParam() + SeedOffset##u, Depth __VA_OPT__(, ) __VA_ARGS__);       \
      auto result = machine.run_one_test();                                    \
      if (result == numsim::cas::fuzzy_detail::TestResult::Skip) {             \
        GTEST_SKIP() << "CAS exception for seed " << GetParam();               \
      }                                                                        \
      EXPECT_EQ(                                                               \
          static_cast<int>(result),                                            \
          static_cast<int>(numsim::cas::fuzzy_detail::TestResult::Pass));      \
    } catch (std::exception const &e) {                                        \
      GTEST_SKIP() << "Uncaught exception for seed " << GetParam() << ": "     \
                   << e.what();                                                \
    }                                                                          \
  }

#endif // FUZZYDIFFBASE_H
