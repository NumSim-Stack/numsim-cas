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
// 1.0-β: piecewise / clamp helpers (max, min, if_then_else),
// constitutive primitives (macauley_plus, macauley_minus, heaviside,
// smoothed_macauley), rank-2 projectors (dev, sym, vol, skew), and
// the 2-arg outer product (`otimes`, aliased as `outer_product`).
//
// Overload resolution note: the registry keys on name only, so each
// name binds to ONE dispatch entry. `if_then_else` registers the
// 3-scalar form; the (scalar, tensor, tensor) and (scalar, t2s, t2s)
// overloads — whose C++ nodes (`tensor_if_then_else_scalar`,
// `tensor_to_scalar_if_then_else`) already exist on main — need
// either dispatch-on-arg-kinds (multi-entry per name) or separately
// named entries. The 4-arg index-list form of `outer_product`
// likewise needs bracket-list grammar support and is deferred.
//
// Aliasing policy (#229): this PR introduces the first *synonym
// pair* in the registry — `outer_product` and `otimes` mapping to
// the same dispatch lambda. (Other long-form-only names like
// `macauley_plus`, `heaviside`, `if_then_else` are registered once
// under their long form; `dot` and `dot_product` share an English
// root but have different arities — they are not synonyms.) Policy
// chosen here, applied going forward:
// aliases are added only when the C++ name is a domain-specific
// abbreviation that non-domain users wouldn't recognize (`otimes`
// → tensor-product notation from differential geometry). Names like
// `asin`, `acos`, `log` etc. that come from std::math do NOT get
// long-form aliases — those are already universal. Functions whose
// C++ name is itself the long form (`macauley_plus`,
// `smoothed_macauley`, `heaviside`, `if_then_else`) are registered
// once under that name — the policy speaks to alias *creation*,
// not to renaming. A future name-vs-alias question should be
// decided against this rule before adding to the registry.
//
// Note: `dot` and `dot_product` are both registered but are NOT
// aliases of each other — `dot` is the 1-arg tensor→t2s norm
// (`dot(A) = A:A`), `dot_product` is the 4-arg index-list
// contraction (`dot_product(A, [i…], B, [j…])`). They share an
// English root, not a dispatch.
//
// Round-trip (β-2d) caveat: several registered names construct
// compound expressions out of existing AST nodes rather than
// producing a dedicated node of their own. Those are:
//   sinh, cosh, tanh, asinh, acosh, atanh, log10 (pre-existing),
//   macauley_plus, macauley_minus, heaviside, smoothed_macauley.
// Their printed form is the LOWERED expression (e.g. macauley_plus(x)
// prints as `max(x, 0)`), so parse→print→parse is SEMANTICALLY
// round-trip (hash-equal — locked in by the *LowersTo* tests in
// ParserTest.h) but NOT SYNTACTICALLY round-trip (the source name is
// irrecoverable from the printed form). β-2d should pick a stance
// (semantic-only vs. add dedicated AST nodes for these names) — this
// PR locks in the current state so the choice is visible.

#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/parser/parse_error.h>
#include <numsim_cas/parser/parser.h>
#include <numsim_cas/scalar/scalar_constant.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/scalar/scalar_functions.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/tensor/identity_tensor.h>
#include <numsim_cas/tensor/levi_civita_tensor.h>
#include <numsim_cas/tensor/sequence.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor/tensor_zero.h>
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
inline function_entry scalar_ternary(auto fn) {
  return {{arg_kind::scalar, arg_kind::scalar, arg_kind::scalar},
          [fn = std::move(fn)](arg_vec a) -> parsed_expression {
            auto &x = std::get<scalar_expr>(a[0]);
            auto &y = std::get<scalar_expr>(a[1]);
            auto &z = std::get<scalar_expr>(a[2]);
            return fn(std::move(x), std::move(y), std::move(z));
          }};
}
inline function_entry tensor_binary(auto fn) {
  return {{arg_kind::tensor, arg_kind::tensor},
          [fn = std::move(fn)](arg_vec a) -> parsed_expression {
            auto &l = std::get<tensor_expr>(a[0]);
            auto &r = std::get<tensor_expr>(a[1]);
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

// Extract a positive size_t from a scalar literal argument. Used by
// the tensor-constant factories (zero_tensor / identity_tensor /
// levi_civita) where `dim` and `rank` must be compile-time-known
// positive integers.
//
// Delegates to the existing `try_int_constant` predicate, which
// already handles the four-case singleton trap: literal `0` and `1`
// are scalar_zero and scalar_one singletons, NOT scalar_constant{0/1}
// (see scalar_make_constant.h), and a literal negation produces a
// scalar_negative wrapping a scalar_constant. A bare
// `is_same<scalar_constant>(e)` check would mishandle all three.
// Pass-3 of #281 surfaced this trap via a `levi_civita(1)` test
// that was rejected with "non-constant expression"; pass-4
// (architect review) noted this is a recurring bug class — see
// scalar_functions.cpp:try_int_constant for the canonical
// singleton-aware integer-literal predicate that THIS function now
// uses, and #284 for the audit of other bare-is_same<scalar_constant>
// callers.
//
// On failure, raises type_mismatch_error with an empty source/zero
// offset per parse_error's documented idiom for "errors outside the
// parser"; position-threading is tracked in #282.
inline std::size_t to_positive_size_t(scalar_expr const &e,
                                      std::string_view fn_name,
                                      std::string_view arg_name) {
  // Defense against 32-bit size_t: try_int_constant returns long long
  // (64-bit on all current targets). On 64-bit platforms size_t is
  // also 64-bit and the cast at the end is exact. Pass-5 review:
  // pin this contract so a hypothetical 32-bit build doesn't silently
  // truncate huge literals.
  static_assert(sizeof(std::size_t) >= sizeof(long long),
                "to_positive_size_t assumes size_t fits long long; add a "
                "range check at the cast site if this no longer holds");
  constexpr std::size_t no_pos = 0;
  auto val = try_int_constant(e);
  if (!val) {
    throw type_mismatch_error(std::string{fn_name} + ": " +
                                  std::string{arg_name} +
                                  " must be a positive integer literal",
                              no_pos, /*source=*/"");
  }
  if (*val <= 0) {
    throw type_mismatch_error(
        std::string{fn_name} + ": " + std::string{arg_name} +
            " must be positive (got " + std::to_string(*val) + ")",
        no_pos, /*source=*/"");
  }
  return static_cast<std::size_t>(*val);
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

// Tensor-constant factories. Function-form: `zero_tensor(dim, rank)`,
// `identity_tensor(dim, rank)`, `levi_civita(dim)`. The plan
// originally considered literal-form (`0{rank=R, dim=D}`,
// `I{rank=R, dim=D}`, `eps{dim=D}`) matching the existing
// tensor_decl syntax, but those shapes either need new grammar rules
// (number-literal followed by `{`) or shadow user variable names
// (`I`, `eps` are common scalar identifiers). Function-form is
// unambiguous, reuses existing dispatch, and is consistent with
// `permute_indices`. The literal form is a separate design decision
// deferred to a follow-up.
//
// Naming: `levi_civita` matches the C++ free function in
// tensor_std.h rather than the printer's short `eps{N}` form
// (#281 review M1). `eps` is a common scalar variable name (machine
// epsilon, Newton tolerance) so registering it as a function would
// shadow user programs that bind it — long-form registration only
// per the aliasing policy at the top of this file.
inline function_entry zero_tensor_entry() {
  return {{arg_kind::scalar, arg_kind::scalar},
          [](arg_vec a) -> parsed_expression {
            auto const &dim_arg = std::get<scalar_expr>(a[0]);
            auto const &rank_arg = std::get<scalar_expr>(a[1]);
            auto dim = to_positive_size_t(dim_arg, "zero_tensor", "dim");
            auto rank = to_positive_size_t(rank_arg, "zero_tensor", "rank");
            return make_expression<tensor_zero>(dim, rank);
          }};
}
inline function_entry identity_tensor_entry() {
  return {{arg_kind::scalar, arg_kind::scalar},
          [](arg_vec a) -> parsed_expression {
            auto const &dim_arg = std::get<scalar_expr>(a[0]);
            auto const &rank_arg = std::get<scalar_expr>(a[1]);
            auto dim = to_positive_size_t(dim_arg, "identity_tensor", "dim");
            auto rank = to_positive_size_t(rank_arg, "identity_tensor", "rank");
            return make_expression<identity_tensor>(dim, rank);
          }};
}
inline function_entry levi_civita_entry() {
  return {{arg_kind::scalar}, [](arg_vec a) -> parsed_expression {
            auto const &dim_arg = std::get<scalar_expr>(a[0]);
            auto dim = to_positive_size_t(dim_arg, "levi_civita", "dim");
            // Qualified call: dim is std::size_t which provides no ADL
            // hook into numsim::cas.
            return ::numsim::cas::levi_civita(dim);
          }};
}

// permute_indices(tensor, [i1, i2, …]) → tensor. The grammar's
// arg_item already accepts index_list_literal anywhere in a function
// call arg list (from the contraction work in β-2a), so no grammar
// change is needed. Rank-vs-indices-size validation happens in
// `permute_indices()` itself (rank-size, range, and uniqueness
// gates) and raises invalid_expression_error.
inline function_entry permute_indices_entry() {
  return {{arg_kind::tensor, arg_kind::index_list},
          [](arg_vec a) -> parsed_expression {
            auto &t = std::get<tensor_expr>(a[0]);
            auto idx = to_sequence(std::get<index_list_value>(a[1]));
            return permute_indices(std::move(t), std::move(idx));
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
    using detail::scalar_ternary;
    using detail::scalar_unary;
    using detail::tensor_binary;
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
    m.emplace("max", scalar_binary([](auto a, auto b) { return max(a, b); }));
    m.emplace("min", scalar_binary([](auto a, auto b) { return min(a, b); }));
    m.emplace("smoothed_macauley", scalar_binary([](auto e, auto eps) {
                return smoothed_macauley(e, eps);
              }));

    // ─── Scalar unary (piecewise / constitutive) ───────────────
    m.emplace("macauley_plus",
              scalar_unary([](auto x) { return macauley_plus(x); }));
    m.emplace("macauley_minus",
              scalar_unary([](auto x) { return macauley_minus(x); }));
    m.emplace("heaviside", scalar_unary([](auto x) { return heaviside(x); }));

    // ─── Scalar ternary (piecewise) ────────────────────────────
    // Registers the (scalar, scalar, scalar) form only. The
    // (scalar cond, tensor then, tensor else) overload from
    // tensor_std.h needs a separately-named entry — the registry
    // is keyed on name alone.
    // (IfThenElseTensorBranchesRaiseTypeMismatch in ParserTest.h
    //  pins the rejection so a future fix that broadens the entry
    //  without updating this comment fails the test.)
    m.emplace("if_then_else", scalar_ternary([](auto c, auto t, auto e) {
                return if_then_else(c, t, e);
              }));

    // ─── Tensor → tensor ───────────────────────────────────────
    m.emplace("trans", tensor_unary([](auto t) { return trans(t); }));
    m.emplace("inv", tensor_unary([](auto t) { return inv(t); }));
    m.emplace("sym", tensor_unary([](auto t) { return sym(t); }));
    m.emplace("dev", tensor_unary([](auto t) { return dev(t); }));
    m.emplace("vol", tensor_unary([](auto t) { return vol(t); }));
    m.emplace("skew", tensor_unary([](auto t) { return skew(t); }));

    // 2-arg outer product. The 4-arg index-list variant
    // (otimes(A, [i...], B, [j...])) is deferred until the grammar
    // grows bracket-list literals.
    //
    // Two registered names: `otimes` matches the C++ API name (used
    // throughout the library) and `outer_product` is the long-form
    // alias for users following the longer-name convention from
    // SymPy / NumPy. The aliases are separate registry entries with
    // their own dispatch lambdas; `OuterProductAliasProducesIdentical
    // Expression` in ParserTest.h locks the equivalence via hash so a
    // future divergence is caught.
    m.emplace("otimes",
              tensor_binary([](auto a, auto b) { return otimes(a, b); }));
    m.emplace("outer_product",
              tensor_binary([](auto a, auto b) { return otimes(a, b); }));

    // ─── Tensor → t2s ──────────────────────────────────────────
    m.emplace("trace", tensor_to_scalar_unary([](auto t) { return trace(t); }));
    m.emplace("det", tensor_to_scalar_unary([](auto t) { return det(t); }));
    m.emplace("norm", tensor_to_scalar_unary([](auto t) { return norm(t); }));
    m.emplace("dot", tensor_to_scalar_unary([](auto t) { return dot(t); }));

    // ─── Contraction (tensor, [idx], tensor, [idx]) ────────────
    m.emplace("inner_product", detail::inner_product_entry());
    m.emplace("dot_product", detail::dot_product_entry());

    // ─── Tensor constants (function-form; β-2c) ────────────────
    m.emplace("zero_tensor", detail::zero_tensor_entry());
    m.emplace("identity_tensor", detail::identity_tensor_entry());
    m.emplace("levi_civita", detail::levi_civita_entry());

    // ─── Index permutation (tensor, [idx]) ─────────────────────
    m.emplace("permute_indices", detail::permute_indices_entry());

    return m;
  }();
  return r;
}

} // namespace numsim::cas::parser::registry

#endif // NUMSIM_CAS_PARSER_FUNCTION_REGISTRY_H
