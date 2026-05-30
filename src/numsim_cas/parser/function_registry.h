#ifndef NUMSIM_CAS_PARSER_FUNCTION_REGISTRY_H
#define NUMSIM_CAS_PARSER_FUNCTION_REGISTRY_H

// Static registry mapping function names to arity + polymorphic
// dispatch. SRC-PRIVATE.
//
// Phase 2c: scalar→scalar functions (trig family, hyperbolic family,
// exp/log/sqrt/abs/sign, pow, six comparisons).
// Phase 2d: tensor→tensor (trans, inv) and tensor→t2s (trace, det,
// norm, dot) added to the same table. Entries now return
// `parsed_expression` and type-check their args against `arg_kinds`.
//
// Not yet registered (still on unmerged PRs): max, min, if_then_else,
// macauley_plus, macauley_minus, heaviside, smoothed_macauley
// (#207/#208/#209). dev/sym/vol/skew projectors and inner_product /
// outer_product / dot_product also still missing — the latter need
// bracket-list index notation in the grammar, which is the next
// integration after #207-#209 land. Calls to any of those names
// fire `unknown_function_error`.

#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/parser/parser.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/tensor/sequence.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_functions.h>

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace numsim::cas::parser::registry {

using scalar_expr = expression_holder<scalar_expression>;
using tensor_expr = expression_holder<tensor_expression>;
using t2s_expr = expression_holder<tensor_to_scalar_expression>;

/// A bracket-list literal like `[1, 2]` as an argument to a
/// contraction function. Carries 1-based indices as the user wrote
/// them; the dispatch converts to numsim_cas's 0-based `sequence`.
struct index_list_value {
  std::vector<std::size_t> indices; // 1-based, as parsed
};

/// Parser-internal value stack alternative. Public `parsed_expression`
/// is the narrower subset returned by `parse()`; this richer variant
/// lets index-list literals flow through the value stack to the
/// function-call action that consumes them. At parse end, the final
/// value must be one of the three expression alternatives —
/// index_list_value at the top is a syntax error (caught explicitly
/// in parser.cpp).
using parser_value =
    std::variant<scalar_expr, tensor_expr, t2s_expr, index_list_value>;
using arg_vec = std::vector<parser_value>;

/// Which kind each positional arg must be in. Checked by the
/// `function_call` action before calling `dispatch` — wrong kind
/// raises `type_mismatch_error` with the call's position.
enum class arg_kind { scalar, tensor, index_list };

struct function_entry {
  // arg_kinds.size() == arity.
  std::vector<arg_kind> arg_kinds;
  std::function<parsed_expression(arg_vec)> dispatch;
};

namespace detail {

inline function_entry scalar_unary(auto fn) {
  return {{arg_kind::scalar},
          [fn = std::move(fn)](arg_vec a) -> parsed_expression {
            auto &s = std::get<scalar_expr>(a[0]);
            return fn(std::move(s));
          }};
}
inline function_entry scalar_binary(auto fn) {
  return {{arg_kind::scalar, arg_kind::scalar},
          [fn = std::move(fn)](arg_vec a) -> parsed_expression {
            auto &l = std::get<scalar_expr>(a[0]);
            auto &r = std::get<scalar_expr>(a[1]);
            return fn(std::move(l), std::move(r));
          }};
}
inline function_entry tensor_unary(auto fn) {
  return {{arg_kind::tensor},
          [fn = std::move(fn)](arg_vec a) -> parsed_expression {
            auto &t = std::get<tensor_expr>(a[0]);
            return fn(std::move(t));
          }};
}
inline function_entry tensor_to_scalar_unary(auto fn) {
  return {{arg_kind::tensor},
          [fn = std::move(fn)](arg_vec a) -> parsed_expression {
            auto &t = std::get<tensor_expr>(a[0]);
            return fn(std::move(t));
          }};
}

// Convert a parsed index_list_value (1-based) to numsim_cas's
// `sequence` (0-based internally; sequence accepts a count then
// per-element writes via `operator[]`).
inline sequence to_sequence(index_list_value const &iv) {
  sequence s(iv.indices.size());
  for (std::size_t i = 0; i < iv.indices.size(); ++i) {
    // 1-based input → 0-based storage, matching sequence's
    // initializer_list ctor semantics. The parser already
    // validated that indices are >= 1 — see the index_list_literal
    // action in actions.h.
    s[i] = iv.indices[i] - 1;
  }
  return s;
}

// Contraction helpers: (tensor, index_list, tensor, index_list).
// The first 4-arg variant returns a tensor (inner_product), the
// second returns a t2s (dot_product). Same arg-kind table.
inline function_entry inner_product_entry() {
  return {{arg_kind::tensor, arg_kind::index_list, arg_kind::tensor,
           arg_kind::index_list},
          [](arg_vec a) -> parsed_expression {
            auto &lhs = std::get<tensor_expr>(a[0]);
            auto lhs_seq = to_sequence(std::get<index_list_value>(a[1]));
            auto &rhs = std::get<tensor_expr>(a[2]);
            auto rhs_seq = to_sequence(std::get<index_list_value>(a[3]));
            return inner_product(std::move(lhs), std::move(lhs_seq),
                                 std::move(rhs), std::move(rhs_seq));
          }};
}

inline function_entry dot_product_entry() {
  return {{arg_kind::tensor, arg_kind::index_list, arg_kind::tensor,
           arg_kind::index_list},
          [](arg_vec a) -> parsed_expression {
            auto &lhs = std::get<tensor_expr>(a[0]);
            auto lhs_seq = to_sequence(std::get<index_list_value>(a[1]));
            auto &rhs = std::get<tensor_expr>(a[2]);
            auto rhs_seq = to_sequence(std::get<index_list_value>(a[3]));
            return dot_product(lhs, std::move(lhs_seq), rhs,
                               std::move(rhs_seq));
          }};
}

} // namespace detail

inline std::unordered_map<std::string, function_entry> const &
function_registry() {
  static auto const r = [] {
    std::unordered_map<std::string, function_entry> m;
    using detail::scalar_binary;
    using detail::scalar_unary;
    using detail::tensor_to_scalar_unary;
    using detail::tensor_unary;

    // ─── Scalar unary ──────────────────────────────────────────
    m.emplace("sin", scalar_unary([](auto x) { return sin(x); }));
    m.emplace("cos", scalar_unary([](auto x) { return cos(x); }));
    m.emplace("tan", scalar_unary([](auto x) { return tan(x); }));
    m.emplace("asin", scalar_unary([](auto x) { return asin(x); }));
    m.emplace("acos", scalar_unary([](auto x) { return acos(x); }));
    m.emplace("atan", scalar_unary([](auto x) { return atan(x); }));
    m.emplace("sinh", scalar_unary([](auto x) { return sinh(x); }));
    m.emplace("cosh", scalar_unary([](auto x) { return cosh(x); }));
    m.emplace("tanh", scalar_unary([](auto x) { return tanh(x); }));
    m.emplace("asinh", scalar_unary([](auto x) { return asinh(x); }));
    m.emplace("acosh", scalar_unary([](auto x) { return acosh(x); }));
    m.emplace("atanh", scalar_unary([](auto x) { return atanh(x); }));
    m.emplace("exp", scalar_unary([](auto x) { return exp(x); }));
    m.emplace("log", scalar_unary([](auto x) { return log(x); }));
    m.emplace("log10", scalar_unary([](auto x) { return log10(x); }));
    m.emplace("sqrt", scalar_unary([](auto x) { return sqrt(x); }));
    m.emplace("abs", scalar_unary([](auto x) { return abs(x); }));
    m.emplace("sign", scalar_unary([](auto x) { return sign(x); }));

    // ─── Scalar binary ─────────────────────────────────────────
    m.emplace("pow", scalar_binary([](auto a, auto b) { return pow(a, b); }));
    m.emplace("lt", scalar_binary([](auto a, auto b) { return lt(a, b); }));
    m.emplace("le", scalar_binary([](auto a, auto b) { return le(a, b); }));
    m.emplace("gt", scalar_binary([](auto a, auto b) { return gt(a, b); }));
    m.emplace("ge", scalar_binary([](auto a, auto b) { return ge(a, b); }));
    m.emplace("eq", scalar_binary([](auto a, auto b) { return eq(a, b); }));
    m.emplace("ne", scalar_binary([](auto a, auto b) { return ne(a, b); }));

    // ─── Tensor → tensor ───────────────────────────────────────
    m.emplace("trans", tensor_unary([](auto t) { return trans(t); }));
    m.emplace("inv", tensor_unary([](auto t) { return inv(t); }));

    // ─── Tensor → t2s ──────────────────────────────────────────
    m.emplace("trace", tensor_to_scalar_unary([](auto t) { return trace(t); }));
    m.emplace("det", tensor_to_scalar_unary([](auto t) { return det(t); }));
    m.emplace("norm", tensor_to_scalar_unary([](auto t) { return norm(t); }));
    m.emplace("dot", tensor_to_scalar_unary([](auto t) { return dot(t); }));

    // ─── Contraction (tensor, [idx], tensor, [idx]) ────────────
    m.emplace("inner_product", detail::inner_product_entry());
    m.emplace("dot_product", detail::dot_product_entry());

    return m;
  }();
  return r;
}

} // namespace numsim::cas::parser::registry

#endif // NUMSIM_CAS_PARSER_FUNCTION_REGISTRY_H
