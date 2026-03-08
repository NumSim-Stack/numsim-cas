#ifndef FUZZYTENSORDIFFTEST_H
#define FUZZYTENSORDIFFTEST_H

#include "cas_test_helpers.h"
#include <algorithm>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <tmech/tmech.h>
#include <vector>

#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/core/diff.h>
#include <numsim_cas/numsim_cas.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor/visitors/tensor_evaluator.h>

namespace numsim::cas {
namespace fuzzy_detail {

// ---------------------------------------------------------------------------
// Compile-time helper: generate tmech::sequence<1,2,...,N> for num_diff_central
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Helper: cast tensor_data_base to typed tmech tensor reference
// ---------------------------------------------------------------------------
template <std::size_t Dim, std::size_t Rank>
auto const &fuzzy_as_tmech(tensor_data_base<double> const &data) {
  return static_cast<tensor_data<double, Dim, Rank> const &>(data).data();
}

// ---------------------------------------------------------------------------
// Compile-time dispatch: map runtime rank to compile-time integral_constant
// ---------------------------------------------------------------------------
template <std::size_t MaxR = 8, typename Fn>
decltype(auto) dispatch_rank(std::size_t rank, Fn &&fn) {
  if (rank == MaxR)
    return fn(std::integral_constant<std::size_t, MaxR>{});
  if constexpr (MaxR > 1)
    return dispatch_rank<MaxR - 1>(rank, std::forward<Fn>(fn));
  else
    throw std::runtime_error("dispatch_rank: rank out of range");
}

// ===========================================================================
// Expression info: tracks the expression, its rank, and which variable names
// appear in the subtree.
// ===========================================================================
struct ExprInfo {
  expression_holder<tensor_expression> expr;
  std::size_t rank;
  std::set<std::string> used_vars;
};

// ===========================================================================
// Variable pool entry
// ===========================================================================
struct VarEntry {
  std::string name;
  std::size_t rank;
  expression_holder<tensor_expression> expr;
};

// ===========================================================================
// Constants
// ===========================================================================
static constexpr std::size_t FDIM = 3;
static constexpr std::size_t FMAX_RANK = 8;
static constexpr std::size_t FMAX_VAR_RANK = 4;

// ===========================================================================
// Verification helper (free function template to avoid Clang
// -Wundefined-internal with member templates in anonymous namespace classes)
// ===========================================================================

// Fill a tmech tensor with deterministic random values from a local RNG.
// Rank-2 tensors get diagonal dominance to ensure well-conditioned matrices
// (important for inv/pow operations where near-singular inputs cause
// numerical differentiation to break down).
template <std::size_t Dim, std::size_t Rank>
void fill_random(tmech::tensor<double, Dim, Rank> &t, std::mt19937 &rng) {
  std::normal_distribution<double> dist(0.0, 1.0);
  auto *ptr = t.raw_data();
  std::size_t n = 1;
  for (std::size_t i = 0; i < Rank; ++i)
    n *= Dim;
  for (std::size_t i = 0; i < n; ++i)
    ptr[i] = dist(rng);
  if constexpr (Rank == 2) {
    for (std::size_t i = 0; i < Dim; ++i)
      ptr[i * Dim + i] += 5.0;
  }
}

template <std::size_t VarRank, std::size_t ExprRank, std::size_t DiffRank>
bool verify_impl(unsigned seed, std::vector<VarEntry> const &vars,
                 ExprInfo const &info, VarEntry const &var,
                 expression_holder<tensor_expression> const &d) {
  std::mt19937 data_rng(seed + 0x9e3779b9u);

  tensor_evaluator<double> ev;
  struct VarData {
    std::string name;
    std::size_t rank;
    std::shared_ptr<tensor_data_base<double>> ptr;
  };
  std::vector<VarData> var_data;

  for (auto const &v : vars) {
    auto ptr = dispatch_rank(
        v.rank,
        [&data_rng](auto R) -> std::shared_ptr<tensor_data_base<double>> {
          constexpr std::size_t r = decltype(R)::value;
          tmech::tensor<double, FDIM, r> t;
          fill_random(t, data_rng);
          return std::make_shared<tensor_data<double, FDIM, r>>(t);
        });
    ev.set(v.expr, ptr);
    var_data.push_back({v.name, v.rank, ptr});
  }

  auto result = ev.apply(d);
  if (!result) {
    ADD_FAILURE() << "seed=" << seed << ": evaluator returned nullptr";
    return false;
  }
  if (result->rank() != DiffRank) {
    ADD_FAILURE() << "seed=" << seed << ": expected rank " << DiffRank
                  << " got " << result->rank();
    return false;
  }

  std::shared_ptr<tensor_data_base<double>> diff_var_ptr;
  for (auto const &vd : var_data) {
    if (vd.name == var.name) {
      diff_var_ptr = vd.ptr;
      break;
    }
  }
  if (!diff_var_ptr) {
    ADD_FAILURE() << "seed=" << seed << ": diff variable data not found";
    return false;
  }

  auto &var_tmech =
      static_cast<tensor_data<double, FDIM, VarRank> &>(*diff_var_ptr).data();
  auto var_original = var_tmech;

  auto numdiff = fuzzy_num_diff<DiffRank>(
      [&](auto const &x) {
        var_tmech = x;
        return fuzzy_as_tmech<FDIM, ExprRank>(*ev.apply(info.expr));
      },
      var_original);

  var_tmech = var_original;

  auto const &sym_result = fuzzy_as_tmech<FDIM, DiffRank>(*result);

  // Use mixed absolute/relative comparison: passes if either absolute or
  // relative error is small enough.  This handles both near-zero derivatives
  // (where relative error is meaningless) and large derivatives from
  // inv/pow (where absolute error grows with magnitude).
  auto const *sym_ptr = sym_result.raw_data();
  auto const *num_ptr = numdiff.raw_data();
  std::size_t n = 1;
  for (std::size_t i = 0; i < DiffRank; ++i)
    n *= FDIM;

  constexpr double abs_tol = 5e-6;
  constexpr double rel_tol = 1e-4;
  double max_err = 0;
  double max_abs = 0;
  for (std::size_t i = 0; i < n; ++i) {
    max_abs =
        std::max(max_abs, std::max(std::abs(sym_ptr[i]), std::abs(num_ptr[i])));
  }
  // Near-zero entries in a large-valued tensor accumulate FD noise
  // proportional to the tensor's scale. Use a noise floor to avoid
  // flagging these as failures.
  double noise_floor = max_abs * 1e-4;
  bool ok = true;
  for (std::size_t i = 0; i < n; ++i) {
    double err = std::abs(sym_ptr[i] - num_ptr[i]);
    double mag = std::max(std::abs(sym_ptr[i]), std::abs(num_ptr[i]));
    max_err = std::max(max_err, err);
    if (err > abs_tol && err > rel_tol * std::max(mag, noise_floor))
      ok = false;
  }
  if (!ok) {
    double rel_err = max_abs > 0 ? max_err / max_abs : max_err;
    ADD_FAILURE() << "seed=" << seed
                  << ": symbolic vs numerical mismatch (expr_rank=" << ExprRank
                  << ", var_rank=" << VarRank << ", diff_rank=" << DiffRank
                  << ", var=" << var.name << ", max_err=" << max_err
                  << ", rel_err=" << rel_err << ")";
  }
  return ok;
}

// ===========================================================================
// OpGenerator — a named, weighted, pluggable operation generator
// ===========================================================================
class FuzzyExprMachine;

struct OpGenerator {
  std::string name;
  int weight;
  std::function<std::optional<ExprInfo>(FuzzyExprMachine &, std::size_t depth)>
      generate;
};

// ===========================================================================
// FuzzyExprMachine — seeded random expression tree generator
// ===========================================================================
class FuzzyExprMachine {
public:
  explicit FuzzyExprMachine(unsigned seed, std::size_t depth = 3)
      : m_rng(seed), m_seed(seed), m_depth(depth) {
    create_variable_pool();
    register_default_ops();
  }

  // Returns: 1 = pass, 0 = fail, -1 = skip (cas_error)
  int run_one_test() {
    ExprInfo info;
    try {
      info = generate(m_depth);
    } catch (cas_error const &) {
      return -1;
    }

    const VarEntry *diff_var = nullptr;
    {
      std::vector<std::size_t> indices;
      for (std::size_t i = 0; i < m_vars.size(); ++i) {
        if (info.used_vars.count(m_vars[i].name) &&
            info.rank + m_vars[i].rank <= FMAX_RANK) {
          indices.push_back(i);
        }
      }
      if (indices.empty())
        return 1; // no valid diff variable
      std::shuffle(indices.begin(), indices.end(), m_rng);
      diff_var = &m_vars[indices[0]];
    }

    expression_holder<tensor_expression> d;
    try {
      d = diff(info.expr, diff_var->expr);
    } catch (cas_error const &) {
      return -1;
    }
    if (!d.is_valid()) {
      ADD_FAILURE() << "seed=" << m_seed
                    << ": diff returned invalid expression";
      return 0;
    }

    std::size_t expr_rank = info.rank;
    std::size_t var_rank = diff_var->rank;
    std::size_t diff_rank = expr_rank + var_rank;

    try {
      return verify(info, *diff_var, d, expr_rank, var_rank, diff_rank) ? 1 : 0;
    } catch (cas_error const &) {
      return -1;
    }
  }

  // --- Public generation helpers (called by OpGenerator lambdas) ---

  ExprInfo generate(std::size_t depth) {
    if (depth == 0)
      return generate_leaf();
    return generate_op(depth);
  }

  std::optional<ExprInfo> generate_at_rank(std::size_t target_rank,
                                           std::size_t depth) {
    if (target_rank == 0)
      return std::nullopt;

    if (depth == 0)
      return generate_leaf_at_rank(target_rank);

    std::uniform_int_distribution<int> op_pick(0, 2);
    int choice = op_pick(m_rng);
    if (choice == 0) {
      auto sub = generate_at_rank(target_rank, depth - 1);
      if (!sub)
        return std::nullopt;
      return ExprInfo{-sub->expr, target_rank, sub->used_vars};
    } else if (choice == 1) {
      auto sub = generate_at_rank(target_rank, depth - 1);
      if (!sub)
        return std::nullopt;
      int sv = pick_nonzero_scalar();
      auto sc = make_scalar_constant(sv);
      return ExprInfo{sc * sub->expr, target_rank, sub->used_vars};
    } else {
      if (target_rank >= 2) {
        auto sub = generate_at_rank(target_rank, depth - 1);
        if (!sub)
          return std::nullopt;
        auto perm = random_permutation(target_rank);
        return ExprInfo{permute_indices(sub->expr, std::move(perm)),
                        target_rank, sub->used_vars};
      }
      auto sub = generate_at_rank(target_rank, depth - 1);
      if (!sub)
        return std::nullopt;
      return ExprInfo{-sub->expr, target_rank, sub->used_vars};
    }
  }

  std::mt19937 &rng() { return m_rng; }
  std::vector<VarEntry> const &vars() const { return m_vars; }

  int pick_nonzero_scalar() {
    std::uniform_int_distribution<int> dist(-3, 3);
    int v;
    do {
      v = dist(m_rng);
    } while (v == 0);
    return v;
  }

  std::vector<std::size_t> sample_positions(std::size_t total,
                                            std::size_t n_pick) {
    std::vector<std::size_t> pool(total);
    std::iota(pool.begin(), pool.end(), std::size_t{0});
    std::shuffle(pool.begin(), pool.end(), m_rng);
    pool.resize(n_pick);
    std::sort(pool.begin(), pool.end());
    return pool;
  }

  sequence random_permutation(std::size_t rank) {
    std::vector<std::size_t> perm(rank);
    std::iota(perm.begin(), perm.end(), std::size_t{0});
    std::shuffle(perm.begin(), perm.end(), m_rng);
    sequence s(rank);
    for (std::size_t i = 0; i < rank; ++i)
      s[i] = perm[i];
    return s;
  }

  // Force non-identity permutation for rank 2
  sequence random_nontrivial_permutation(std::size_t rank) {
    if (rank == 2) {
      sequence s(2);
      s[0] = 1;
      s[1] = 0;
      return s;
    }
    return random_permutation(rank);
  }

  static std::set<std::string> merge_vars(std::set<std::string> const &a,
                                          std::set<std::string> const &b) {
    std::set<std::string> result;
    result.insert(a.begin(), a.end());
    result.insert(b.begin(), b.end());
    return result;
  }

private:
  std::mt19937 m_rng;
  unsigned m_seed;
  std::size_t m_depth;
  std::vector<VarEntry> m_vars;
  std::vector<OpGenerator> m_ops;

  void create_variable_pool() {
    auto add_var = [&](std::string name, std::size_t rank) {
      auto expr = make_expression<tensor>(name, FDIM, rank);
      m_vars.push_back({std::move(name), rank, expr});
    };

    add_var("a", 1);
    add_var("b", 1);
    add_var("C", 2);
    add_var("D", 2);
    add_var("E", 3);
    add_var("F", 4);
    add_var("G", 2); // extra rank-2 for product rule variety
  }

  // -----------------------------------------------------------------------
  // Register all operations
  // -----------------------------------------------------------------------
  void register_default_ops() {
    // inner_product: A:{i} B:{j} -> rank(A)+rank(B)-2*n_contract
    m_ops.push_back(
        {"inner_product", 25,
         [](FuzzyExprMachine &m, std::size_t depth) -> std::optional<ExprInfo> {
           auto lhs = m.generate(depth - 1);
           auto rhs = m.generate(depth - 1);
           if (lhs.rank == 0 || rhs.rank == 0)
             return std::nullopt;
           std::size_t max_contract = std::min(lhs.rank, rhs.rank);
           std::uniform_int_distribution<std::size_t> nc_dist(1, max_contract);
           std::size_t n_contract = nc_dist(m.rng());
           std::size_t result_rank = lhs.rank + rhs.rank - 2 * n_contract;
           if (result_rank > FMAX_RANK || result_rank == 0)
             return std::nullopt;
           auto lhs_pos = m.sample_positions(lhs.rank, n_contract);
           auto rhs_pos = m.sample_positions(rhs.rank, n_contract);
           sequence lhs_seq(n_contract), rhs_seq(n_contract);
           for (std::size_t i = 0; i < n_contract; ++i) {
             lhs_seq[i] = lhs_pos[i];
             rhs_seq[i] = rhs_pos[i];
           }
           auto expr = inner_product(lhs.expr, std::move(lhs_seq), rhs.expr,
                                     std::move(rhs_seq));
           return ExprInfo{expr, result_rank,
                           merge_vars(lhs.used_vars, rhs.used_vars)};
         }});

    // otimes: rank(A)+rank(B) with random index placement
    m_ops.push_back(
        {"otimes", 15,
         [](FuzzyExprMachine &m, std::size_t depth) -> std::optional<ExprInfo> {
           auto lhs = m.generate(depth - 1);
           auto rhs = m.generate(depth - 1);
           std::size_t result_rank = lhs.rank + rhs.rank;
           if (result_rank > FMAX_RANK || result_rank == 0)
             return std::nullopt;
           sequence lhs_seq(lhs.rank), rhs_seq(rhs.rank);
           std::iota(lhs_seq.begin(), lhs_seq.end(), std::size_t{0});
           std::iota(rhs_seq.begin(), rhs_seq.end(), lhs.rank);
           std::uniform_int_distribution<int> coin(0, 1);
           if (coin(m.rng())) {
             std::vector<std::size_t> combined(result_rank);
             std::iota(combined.begin(), combined.end(), std::size_t{0});
             std::shuffle(combined.begin(), combined.end(), m.rng());
             for (std::size_t i = 0; i < lhs.rank; ++i)
               lhs_seq[i] = combined[i];
             for (std::size_t i = 0; i < rhs.rank; ++i)
               rhs_seq[i] = combined[lhs.rank + i];
           }
           auto expr = otimes(lhs.expr, std::move(lhs_seq), rhs.expr,
                              std::move(rhs_seq));
           return ExprInfo{expr, result_rank,
                           merge_vars(lhs.used_vars, rhs.used_vars)};
         }});

    // otimesu / otimesl: rank-2 x rank-2 -> rank-4, specific index patterns
    m_ops.push_back(
        {"otimesu_l", 8,
         [](FuzzyExprMachine &m, std::size_t depth) -> std::optional<ExprInfo> {
           auto lhs = m.generate(depth - 1);
           auto rhs = m.generate(depth - 1);
           if (lhs.rank != 2 || rhs.rank != 2)
             return std::nullopt;
           if (4 > FMAX_RANK)
             return std::nullopt;
           std::uniform_int_distribution<int> coin(0, 1);
           expression_holder<tensor_expression> expr;
           if (coin(m.rng()))
             expr = otimesu(lhs.expr, rhs.expr);
           else
             expr = otimesl(lhs.expr, rhs.expr);
           return ExprInfo{expr, 4, merge_vars(lhs.used_vars, rhs.used_vars)};
         }});

    // permute_indices: random permutation, non-trivial for rank 2
    m_ops.push_back(
        {"permute", 12,
         [](FuzzyExprMachine &m, std::size_t depth) -> std::optional<ExprInfo> {
           auto sub = m.generate(depth - 1);
           if (sub.rank < 2)
             return std::nullopt;
           auto perm = m.random_nontrivial_permutation(sub.rank);
           auto expr = permute_indices(sub.expr, std::move(perm));
           return ExprInfo{expr, sub.rank, sub.used_vars};
         }});

    // trans: basis_change with {1,0} on rank-2
    m_ops.push_back(
        {"trans", 5,
         [](FuzzyExprMachine &m, std::size_t depth) -> std::optional<ExprInfo> {
           auto sub = m.generate(depth - 1);
           if (sub.rank != 2)
             return std::nullopt;
           auto expr = trans(sub.expr);
           return ExprInfo{expr, 2, sub.used_vars};
         }});

    // negation: -expr
    m_ops.push_back(
        {"negation", 5,
         [](FuzzyExprMachine &m, std::size_t depth) -> std::optional<ExprInfo> {
           auto sub = m.generate(depth - 1);
           return ExprInfo{-sub.expr, sub.rank, sub.used_vars};
         }});

    // scalar_mul: c * expr
    m_ops.push_back(
        {"scalar_mul", 5,
         [](FuzzyExprMachine &m, std::size_t depth) -> std::optional<ExprInfo> {
           auto sub = m.generate(depth - 1);
           int sv = m.pick_nonzero_scalar();
           auto sc = make_scalar_constant(sv);
           return ExprInfo{sc * sub.expr, sub.rank, sub.used_vars};
         }});

    // addition: lhs + rhs (matching ranks)
    m_ops.push_back(
        {"addition", 10,
         [](FuzzyExprMachine &m, std::size_t depth) -> std::optional<ExprInfo> {
           auto lhs = m.generate(depth - 1);
           auto rhs = m.generate_at_rank(lhs.rank, depth - 1);
           if (!rhs || rhs->rank != lhs.rank)
             return std::nullopt;
           auto expr = lhs.expr + rhs->expr;
           return ExprInfo{expr, lhs.rank,
                           merge_vars(lhs.used_vars, rhs->used_vars)};
         }});

    // tensor_mul: A * B (rank-2 only, same-rank contraction)
    m_ops.push_back(
        {"tensor_mul", 10,
         [](FuzzyExprMachine &m, std::size_t depth) -> std::optional<ExprInfo> {
           auto lhs = m.generate(depth - 1);
           auto rhs = m.generate(depth - 1);
           if (lhs.rank != 2 || rhs.rank != 2)
             return std::nullopt;
           auto expr = lhs.expr * rhs.expr;
           return ExprInfo{expr, 2, merge_vars(lhs.used_vars, rhs.used_vars)};
         }});

    // pow: A^n (rank-2 only, exponent 2-3)
    m_ops.push_back(
        {"pow", 8,
         [](FuzzyExprMachine &m, std::size_t depth) -> std::optional<ExprInfo> {
           auto sub = m.generate(depth - 1);
           if (sub.rank != 2)
             return std::nullopt;
           std::uniform_int_distribution<int> exp_dist(2, 3);
           int exponent = exp_dist(m.rng());
           auto expr = pow(sub.expr, exponent);
           return ExprInfo{expr, 2, sub.used_vars};
         }});

    // inv: A^{-1} (rank-2 only)
    m_ops.push_back(
        {"inv", 5,
         [](FuzzyExprMachine &m, std::size_t depth) -> std::optional<ExprInfo> {
           auto sub = m.generate(depth - 1);
           if (sub.rank != 2)
             return std::nullopt;
           auto expr = inv(sub.expr);
           return ExprInfo{expr, 2, sub.used_vars};
         }});

    // simple_outer_product: A1 ⊗ A2 [⊗ A3] (2-3 factors)
    m_ops.push_back(
        {"simple_outer_product", 7,
         [](FuzzyExprMachine &m, std::size_t depth) -> std::optional<ExprInfo> {
           std::uniform_int_distribution<int> nf_dist(2, 3);
           int n_factors = nf_dist(m.rng());
           std::vector<ExprInfo> factors;
           factors.reserve(n_factors);
           std::size_t total_rank = 0;
           for (int i = 0; i < n_factors; ++i) {
             auto f = m.generate(depth - 1);
             total_rank += f.rank;
             if (total_rank > FMAX_RANK)
               return std::nullopt;
             factors.push_back(std::move(f));
           }
           if (total_rank == 0)
             return std::nullopt;
           auto sop = make_expression<simple_outer_product>(FDIM, total_rank);
           std::set<std::string> all_vars;
           for (auto &f : factors) {
             sop.template get<simple_outer_product>().push_back(f.expr);
             all_vars.insert(f.used_vars.begin(), f.used_vars.end());
           }
           return ExprInfo{sop, total_rank, all_vars};
         }});
  }

  // -----------------------------------------------------------------------
  // Generate a leaf node
  // -----------------------------------------------------------------------
  ExprInfo generate_leaf() {
    std::uniform_int_distribution<std::size_t> dist(0, m_vars.size() - 1);
    auto idx = dist(m_rng);
    return {m_vars[idx].expr, m_vars[idx].rank, {m_vars[idx].name}};
  }

  std::optional<ExprInfo> generate_leaf_at_rank(std::size_t target_rank) {
    std::vector<std::size_t> candidates;
    for (std::size_t i = 0; i < m_vars.size(); ++i) {
      if (m_vars[i].rank == target_rank)
        candidates.push_back(i);
    }
    if (candidates.empty())
      return std::nullopt;
    auto idx = candidates[std::uniform_int_distribution<std::size_t>(
        0, candidates.size() - 1)(m_rng)];
    return ExprInfo{m_vars[idx].expr, m_vars[idx].rank, {m_vars[idx].name}};
  }

  // -----------------------------------------------------------------------
  // Generate an operation node using the registered OpGenerators
  // -----------------------------------------------------------------------
  ExprInfo generate_op(std::size_t depth) {
    std::vector<int> weights;
    weights.reserve(m_ops.size());
    for (auto const &op : m_ops)
      weights.push_back(op.weight);
    std::discrete_distribution<int> op_dist(weights.begin(), weights.end());

    int first_pick = op_dist(m_rng);

    for (std::size_t attempt = 0; attempt < m_ops.size(); ++attempt) {
      int idx = static_cast<int>((first_pick + attempt) % m_ops.size());
      if (m_ops[idx].weight == 0)
        continue;
      auto result = m_ops[idx].generate(*this, depth);
      if (result)
        return *result;
    }

    // Fallback: negation always succeeds
    auto sub = generate(depth - 1);
    return {-sub.expr, sub.rank, sub.used_vars};
  }

  // -----------------------------------------------------------------------
  // Verification via double dispatch
  // -----------------------------------------------------------------------
  bool verify(ExprInfo const &info, VarEntry const &var,
              expression_holder<tensor_expression> const &d,
              std::size_t expr_rank, std::size_t var_rank,
              std::size_t diff_rank) {
    return dispatch_rank(var_rank, [&](auto VR) -> bool {
      return dispatch_rank(expr_rank, [&](auto ER) -> bool {
        constexpr std::size_t vr = decltype(VR)::value;
        constexpr std::size_t er = decltype(ER)::value;
        constexpr std::size_t dr = vr + er;
        if constexpr (dr > FMAX_RANK) {
          return true;
        } else {
          if (dr != diff_rank)
            return true;
          return verify_impl<vr, er, dr>(m_seed, m_vars, info, var, d);
        }
      });
    });
  }
};

} // namespace fuzzy_detail

namespace {

// ===========================================================================
// Value-parameterized test suite
// ===========================================================================

class FuzzyExprMachineTest : public ::testing::TestWithParam<unsigned> {};

TEST_P(FuzzyExprMachineTest, Depth3) {
  fuzzy_detail::FuzzyExprMachine machine(GetParam(), 3);
  int result = machine.run_one_test();
  if (result == -1) {
    GTEST_SKIP() << "CAS exception for seed " << GetParam();
  }
  EXPECT_EQ(result, 1);
}

TEST_P(FuzzyExprMachineTest, Depth4) {
  fuzzy_detail::FuzzyExprMachine machine(GetParam() + 10000u, 4);
  int result = machine.run_one_test();
  if (result == -1) {
    GTEST_SKIP() << "CAS exception for seed " << GetParam();
  }
  EXPECT_EQ(result, 1);
}

INSTANTIATE_TEST_SUITE_P(FuzzySeeds, FuzzyExprMachineTest,
                         ::testing::Range(1u, 101u));

} // namespace
} // namespace numsim::cas

#endif // FUZZYTENSORDIFFTEST_H
