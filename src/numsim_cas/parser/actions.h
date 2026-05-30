#ifndef NUMSIM_CAS_PARSER_ACTIONS_H
#define NUMSIM_CAS_PARSER_ACTIONS_H

// PEGTL actions for the numsim-cas expression parser (issue #214).
//
// SRC-PRIVATE header — includes PEGTL and pulls in factory functions
// across all three expression domains.
//
// Design: a single value-stack of `parsed_expression` variants.
// Operand-level rules (number_literal, identifier, paren_expression)
// push exactly one value when they match. Operator-tail rules
// (mul_tail, add_tail) push the right operand THEN combine it with
// the previous top using the matched operator — so by the time the
// outer `mul_term` / `add_term` finishes, the top of the stack is
// the fully-folded result of that precedence level.
//
// Phase 2a: numbers, identifiers, + - * /. No mixed-domain coercion
// yet (phase 2d wires up the full type-coercion table); for now
// every value on the stack is scalar.

#include "function_registry.h"
#include "grammar.h"

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/parser/parse_error.h>
#include <numsim_cas/parser/parser.h>
#include <numsim_cas/parser/symbol_table.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>

#include <tao/pegtl.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace numsim::cas::parser::actions {

namespace pegtl = tao::pegtl;

/**
 * @struct parser_state
 * @brief Mutable state threaded through every PEGTL action.
 *
 * Holds:
 *   - the value stack (variants of holder types across the three domains),
 *   - a reference to the caller-supplied symbol_table,
 *   - the original source view (for error rendering).
 *
 * The operator-tail rules (`mul_tail_star`, `add_tail_plus`, …) are
 * split per-operator in the grammar so each action template
 * specialisation knows its op statically. No `pending_op` field is
 * needed — that approach was attempted and was fragile under nested
 * operator firings.
 */
struct parser_state {
  // Phase 2e: values stack is the wider `registry::parser_value`
  // variant (parsed_expression alternatives + index_list_value) so
  // bracket-list args can flow through the same stack as
  // expressions. At the end of `parse()`, the final stack entry
  // must be one of the three expression alternatives — an
  // index_list_value at the top is a syntax error (caught
  // explicitly in parser.cpp).
  std::vector<registry::parser_value> values;

  // Stack of value-stack depths at function-call '(' marks. Pushed by
  // `function_call_open`'s action, popped by `function_call`'s — the
  // difference between the current size and the popped mark is the
  // number of arguments pushed by the enclosed expression list.
  // Stack-of-marks supports nested calls like `max(sin(x), cos(x))`.
  std::vector<std::size_t> arg_marks;

  // Pending kv pairs for the current tensor declaration. Both must be
  // present by the time the closing brace fires the tensor_decl
  // action. Flat fields (not a stack) because tensor_decl bodies
  // can't nest — the RHS of `rank=` / `dim=` must be an integer
  // literal, not an expression.
  std::optional<std::size_t> pending_rank;
  std::optional<std::size_t> pending_dim;

  symbol_table &syms;
  std::string_view source;

  explicit parser_state(symbol_table &s, std::string_view src) noexcept
      : syms(s), source(src) {}
};

// Helper: extract a scalar holder from a value-stack entry. Used by
// scalar-only operators (+, -, ^, comparisons, unary -). The stack
// now holds `parser_value` which includes `index_list_value`; that
// alternative is rejected here (operators can't take index lists).
inline expression_holder<scalar_expression> &
require_scalar(registry::parser_value &v, std::size_t pos,
               std::string_view source) {
  if (auto *s = std::get_if<expression_holder<scalar_expression>>(&v))
    return *s;
  throw type_mismatch_error("operator expects scalar operands; got non-scalar",
                            pos, source);
}

// Same-domain combinator: lhs OP rhs only when both values are in
// the SAME alternative AND that alternative is an expression
// (not index_list_value). Used by +, -, and the comparison
// operators.
template <typename Op>
void combine_same_domain(parser_state &state, std::size_t pos, Op &&op,
                         char const *op_name) {
  if (state.values.size() < 2) {
    throw syntax_error("operator missing left or right operand", pos,
                       state.source);
  }
  auto rhs = std::move(state.values.back());
  state.values.pop_back();
  auto lhs = std::move(state.values.back());
  state.values.pop_back();
  // Reject index_list_value either side — operators don't take
  // bracket-lists.
  if (std::holds_alternative<registry::index_list_value>(lhs) ||
      std::holds_alternative<registry::index_list_value>(rhs)) {
    throw type_mismatch_error(std::string("operator '") + op_name +
                                  "' does not accept bracket-list arguments",
                              pos, state.source);
  }
  if (lhs.index() != rhs.index()) {
    throw type_mismatch_error(std::string("operator '") + op_name +
                                  "' requires both operands in the same domain "
                                  "(scalar+scalar, tensor+tensor, or t2s+t2s)",
                              pos, state.source);
  }
  state.values.emplace_back(std::visit(
      [&](auto &&l, auto &&r) -> registry::parser_value {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;
        if constexpr (std::is_same_v<L, registry::index_list_value> ||
                      std::is_same_v<R, registry::index_list_value>) {
          // Unreachable — guarded above. The throw exists so the
          // lambda compiles for those instantiations; if it ever
          // does fire (logic bug), the message + real position keep
          // the diagnostic useful instead of pointing at byte 0.
          throw type_mismatch_error("internal: index_list slipped past guard",
                                    pos, state.source);
        } else if constexpr (std::is_same_v<L, R>) {
          return op(std::move(l), std::move(r));
        } else {
          // Unreachable — guarded by lhs.index() == rhs.index() above.
          throw type_mismatch_error("internal: domain mismatch slipped past "
                                    "same-domain guard",
                                    0, std::string_view{});
        }
      },
      std::move(lhs), std::move(rhs)));
}

// Scalar-only combinator: both operands must be scalar. Used by ^
// (power) and the six comparison operators.
template <typename Op>
void combine_scalar_only(parser_state &state, std::size_t pos, Op &&op) {
  if (state.values.size() < 2) {
    throw syntax_error("operator missing left or right operand", pos,
                       state.source);
  }
  auto rhs = require_scalar(state.values.back(), pos, state.source);
  state.values.pop_back();
  auto lhs = require_scalar(state.values.back(), pos, state.source);
  state.values.pop_back();
  state.values.emplace_back(op(std::move(lhs), std::move(rhs)));
}

// Compile-time table: which (L, R) pairs the codebase's `operator*`
// supports as a user-facing call producing a valid result holder.
// We hand-list rather than relying on `requires { l * r; }` SFINAE
// because the codebase's `cas_binary_op` concept is permissive
// enough to admit some pairs whose body then fails — the failure
// is inside the operator, not at overload resolution, so SFINAE
// doesn't filter it. Hand-listing matches the documented
// type-coercion table in the plan.
template <typename L, typename R>
constexpr bool can_mul =
    (std::is_same_v<L, expression_holder<scalar_expression>> &&
     std::is_same_v<R, expression_holder<scalar_expression>>) ||
    (std::is_same_v<L, expression_holder<scalar_expression>> &&
     std::is_same_v<R, expression_holder<tensor_expression>>) ||
    (std::is_same_v<L, expression_holder<tensor_expression>> &&
     std::is_same_v<R, expression_holder<scalar_expression>>) ||
    (std::is_same_v<L, expression_holder<scalar_expression>> &&
     std::is_same_v<R, expression_holder<tensor_to_scalar_expression>>) ||
    (std::is_same_v<L, expression_holder<tensor_to_scalar_expression>> &&
     std::is_same_v<R, expression_holder<scalar_expression>>) ||
    (std::is_same_v<L, expression_holder<tensor_to_scalar_expression>> &&
     std::is_same_v<R, expression_holder<tensor_to_scalar_expression>>);

// `operator/` has a NARROWER set: scalar denominators only, plus
// the (t2s,t2s) same-domain case. (s/t) and (s/t2s) intentionally
// excluded — these aren't in the codebase's tag_invoke surface.
template <typename L, typename R>
constexpr bool can_div =
    (std::is_same_v<L, expression_holder<scalar_expression>> &&
     std::is_same_v<R, expression_holder<scalar_expression>>) ||
    (std::is_same_v<L, expression_holder<tensor_expression>> &&
     std::is_same_v<R, expression_holder<scalar_expression>>) ||
    (std::is_same_v<L, expression_holder<tensor_to_scalar_expression>> &&
     std::is_same_v<R, expression_holder<scalar_expression>>) ||
    (std::is_same_v<L, expression_holder<tensor_to_scalar_expression>> &&
     std::is_same_v<R, expression_holder<tensor_to_scalar_expression>>);

// Mixed-domain `*`: use `can_mul` table. The `can_mul` predicate is
// false for any pair involving `index_list_value`, so those fall
// into the else branch and throw — no extra guard needed.
template <typename Op>
void combine_mul(parser_state &state, std::size_t pos, Op &&op) {
  if (state.values.size() < 2) {
    throw syntax_error("operator missing left or right operand", pos,
                       state.source);
  }
  auto rhs = std::move(state.values.back());
  state.values.pop_back();
  auto lhs = std::move(state.values.back());
  state.values.pop_back();
  state.values.emplace_back(std::visit(
      [&](auto &&l, auto &&r) -> registry::parser_value {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;
        if constexpr (can_mul<L, R>) {
          return op(std::move(l), std::move(r));
        } else {
          throw type_mismatch_error(
              "operator '*' is not defined for this combination of "
              "operand types",
              pos, state.source);
        }
      },
      std::move(lhs), std::move(rhs)));
}

// Mixed-domain `/`: use `can_div` table.
template <typename Op>
void combine_div(parser_state &state, std::size_t pos, Op &&op) {
  if (state.values.size() < 2) {
    throw syntax_error("operator missing left or right operand", pos,
                       state.source);
  }
  auto rhs = std::move(state.values.back());
  state.values.pop_back();
  auto lhs = std::move(state.values.back());
  state.values.pop_back();
  state.values.emplace_back(std::visit(
      [&](auto &&l, auto &&r) -> registry::parser_value {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;
        if constexpr (can_div<L, R>) {
          return op(std::move(l), std::move(r));
        } else {
          throw type_mismatch_error(
              "operator '/' is not defined for this combination of "
              "operand types",
              pos, state.source);
        }
      },
      std::move(lhs), std::move(rhs)));
}

// Pop the top scalar operand and replace it with op(top). Used by
// unary minus (scalar-only on this branch).
template <typename Op>
void replace_top(parser_state &state, std::size_t pos, Op &&op) {
  if (state.values.empty()) {
    throw syntax_error("unary operator missing operand", pos, state.source);
  }
  auto v = require_scalar(state.values.back(), pos, state.source);
  state.values.pop_back();
  state.values.emplace_back(op(std::move(v)));
}

// ─── Default action: do nothing ───────────────────────────────────
template <typename Rule> struct action : pegtl::nothing<Rule> {};

// ─── Operand actions ──────────────────────────────────────────────

// number_literal: parse the matched range with std::from_chars (locale-
// independent) and push a scalar_constant.
template <> struct action<grammar::number_literal> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    auto sv = in.string_view();
    // Decimal point in the matched range tells us it's a double.
    if (sv.find('.') != std::string_view::npos) {
      double value = 0.0;
      auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
      if (ec != std::errc{} || ptr != sv.data() + sv.size()) {
        throw lexical_error("malformed decimal literal", in.position().byte,
                            state.source);
      }
      state.values.emplace_back(make_scalar_constant(value));
    } else {
      std::int64_t value = 0;
      auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
      if (ec != std::errc{} || ptr != sv.data() + sv.size()) {
        throw lexical_error("malformed integer literal", in.position().byte,
                            state.source);
      }
      state.values.emplace_back(make_scalar_constant(value));
    }
  }
};

// identifier: look up the symbol table first — if `name` already
// exists, push the existing entry (scalar or tensor). Only declare
// as a fresh scalar when the name is unknown. This lets the user
// do `A{rank=2, dim=3}` once and then refer to `A` bare in the
// rest of the expression.
template <> struct action<grammar::identifier> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    auto name = in.string_view();
    if (auto existing = state.syms.get(name)) {
      std::visit([&](auto const &expr) { state.values.emplace_back(expr); },
                 *existing);
      return;
    }
    try {
      auto expr = state.syms.get_or_declare_scalar(name);
      state.values.emplace_back(std::move(expr));
    } catch (parse_error const &e) {
      // symbol_table threw a position-less error; re-throw with our
      // input context attached. Shouldn't fire on the get-first path
      // but kept defensively.
      throw type_collision_error(e.what(), in.position().byte, state.source);
    }
  }
};

// ─── Per-op tail actions ──────────────────────────────────────────
// Each rule is one specific operator paired with its RHS operand, so
// the action knows which op fired statically.

template <> struct action<grammar::mul_tail_star> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    combine_mul(state, in.position().byte,
                [](auto &&a, auto &&b) { return a * b; });
  }
};
template <> struct action<grammar::mul_tail_slash> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    combine_div(state, in.position().byte,
                [](auto &&a, auto &&b) { return a / b; });
  }
};
template <> struct action<grammar::add_tail_plus> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    combine_same_domain(
        state, in.position().byte, [](auto &&a, auto &&b) { return a + b; },
        "+");
  }
};
template <> struct action<grammar::add_tail_minus> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    combine_same_domain(
        state, in.position().byte, [](auto &&a, auto &&b) { return a - b; },
        "-");
  }
};

// ─── Power (right-associative) ────────────────────────────────────
// `power_tail` matches `^ <power>` where the inner `power` recurses
// — by the time this action fires, the inner power has fully
// reduced and pushed its result. So the stack has [..., lhs, rhs].
template <> struct action<grammar::power_tail> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    combine_scalar_only(state, in.position().byte,
                        [](auto &&a, auto &&b) { return pow(a, b); });
  }
};

// ─── Unary minus ──────────────────────────────────────────────────
// `unary_minus` is `'-' unary` (right-recursive). The inner `unary`
// pushes the value first; we replace the top with its negation.
template <> struct action<grammar::unary_minus> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    replace_top(state, in.position().byte, [](auto &&v) { return -v; });
  }
};

// ─── Comparison ops ───────────────────────────────────────────────
// Each comparison combines two scalars into a scalar indicator
// (1.0 / 0.0) via the codebase's `lt`/`gt`/`le`/`ge`/`eq`/`ne` free
// functions (Option B predicates from #136).
template <> struct action<grammar::cmp_tail_lt> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    combine_scalar_only(state, in.position().byte,
                        [](auto &&a, auto &&b) { return lt(a, b); });
  }
};
template <> struct action<grammar::cmp_tail_le> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    combine_scalar_only(state, in.position().byte,
                        [](auto &&a, auto &&b) { return le(a, b); });
  }
};
template <> struct action<grammar::cmp_tail_gt> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    combine_scalar_only(state, in.position().byte,
                        [](auto &&a, auto &&b) { return gt(a, b); });
  }
};
template <> struct action<grammar::cmp_tail_ge> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    combine_scalar_only(state, in.position().byte,
                        [](auto &&a, auto &&b) { return ge(a, b); });
  }
};
template <> struct action<grammar::eq_tail_eq> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    combine_scalar_only(state, in.position().byte,
                        [](auto &&a, auto &&b) { return eq(a, b); });
  }
};
template <> struct action<grammar::eq_tail_ne> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    combine_scalar_only(state, in.position().byte,
                        [](auto &&a, auto &&b) { return ne(a, b); });
  }
};

// ─── Function calls ───────────────────────────────────────────────
// The opening '(' of a function call records the current value-stack
// depth. By the time the matching ')' fires the function_call action,
// values pushed since the mark are exactly the call's arguments.

template <> struct action<grammar::function_call_open> {
  static void apply0(parser_state &state) {
    state.arg_marks.push_back(state.values.size());
  }
};

template <> struct action<grammar::function_call> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    auto pos = in.position().byte;
    auto matched = in.string_view();

    // Function name = matched range up to first non-identifier char.
    std::size_t name_end = 0;
    while (name_end < matched.size() &&
           (std::isalnum(static_cast<unsigned char>(matched[name_end])) ||
            matched[name_end] == '_')) {
      ++name_end;
    }
    std::string name(matched.substr(0, name_end));

    // Pop the arg mark recorded by function_call_open's action.
    if (state.arg_marks.empty()) {
      throw syntax_error("internal: missing arg-mark for function call", pos,
                         state.source);
    }
    auto mark = state.arg_marks.back();
    state.arg_marks.pop_back();
    auto arg_count = state.values.size() - mark;

    // Resolve in the polymorphic registry.
    auto const &reg = registry::function_registry();
    auto it = reg.find(name);
    if (it == reg.end()) {
      throw unknown_function_error(std::move(name), pos, state.source);
    }
    auto const &entry = it->second;
    if (arg_count != entry.arg_kinds.size()) {
      throw arity_error(std::move(name), entry.arg_kinds.size(), arg_count, pos,
                        state.source);
    }

    // Collect args in source order. The stack has them in push order
    // (= source order); pop from back and reverse at the end.
    registry::arg_vec args;
    args.reserve(arg_count);
    for (std::size_t i = 0; i < arg_count; ++i) {
      args.push_back(std::move(state.values.back()));
      state.values.pop_back();
    }
    std::reverse(args.begin(), args.end());

    // Type-check each arg against the entry's declared kinds before
    // calling dispatch — dispatch uses unchecked `std::get` so a
    // mismatch here would throw `std::bad_variant_access` rather
    // than our nicer type_mismatch_error.
    for (std::size_t i = 0; i < arg_count; ++i) {
      bool ok = false;
      switch (entry.arg_kinds[i]) {
      case registry::arg_kind::scalar:
        ok = std::holds_alternative<expression_holder<scalar_expression>>(
            args[i]);
        break;
      case registry::arg_kind::tensor:
        ok = std::holds_alternative<expression_holder<tensor_expression>>(
            args[i]);
        break;
      case registry::arg_kind::index_list:
        ok = std::holds_alternative<registry::index_list_value>(args[i]);
        break;
      }
      if (!ok) {
        throw type_mismatch_error(
            "function '" + name + "': argument " + std::to_string(i + 1) +
                " has wrong type for the expected signature",
            pos, state.source);
      }
    }

    // Dispatch returns `parsed_expression` (3-variant); convert up to
    // the wider `parser_value` (4-variant) before pushing onto the
    // parser-internal stack.
    auto result = entry.dispatch(std::move(args));
    state.values.emplace_back(std::visit(
        [](auto &&v) -> registry::parser_value { return std::move(v); },
        std::move(result)));
  }
};

// ─── Bracket-list index literal: [i1, i2, ...] ────────────────────
// Parses the matched range, extracting digit runs as 1-based indices.
// Pushes an index_list_value onto the parser-internal value stack
// for the enclosing function_call action to consume.
//
// The grammar's `list<integer_literal, ...>` guarantees at least one
// integer matched before this action fires, so we don't re-check for
// emptiness here.
template <> struct action<grammar::index_list_literal> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    auto pos = in.position().byte;
    auto sv = in.string_view();
    registry::index_list_value v;
    std::size_t i = 0;
    while (i < sv.size()) {
      if (std::isdigit(static_cast<unsigned char>(sv[i]))) {
        std::size_t j = i;
        while (j < sv.size() &&
               std::isdigit(static_cast<unsigned char>(sv[j]))) {
          ++j;
        }
        std::size_t value = 0;
        auto [ptr, ec] = std::from_chars(sv.data() + i, sv.data() + j, value);
        if (ec == std::errc::result_out_of_range) {
          throw lexical_error(
              "index in bracket-list exceeds maximum representable value",
              pos + i, state.source);
        }
        if (ec != std::errc{} || value < 1) {
          throw lexical_error(
              "index in bracket-list must be a positive 1-based integer",
              pos + i, state.source);
        }
        v.indices.push_back(value);
        i = j;
      } else {
        ++i;
      }
    }
    state.values.emplace_back(std::move(v));
  }
};

// ─── Tensor declarations: A{rank=R, dim=D} ────────────────────────
// `rank_kv` / `dim_kv` actions parse the matched integer literal and
// store it in parser_state. `tensor_decl` consumes both, declares
// the tensor in the symbol_table, and pushes the holder.

namespace detail_kv {
inline std::size_t parse_kv_integer(std::string_view matched, std::size_t pos,
                                    std::string_view source) {
  // The matched range starts with the keyword ('rank' or 'dim'),
  // contains ws + '=' + ws, then the integer literal at the end.
  // Scan backwards for the start of the digit run.
  std::size_t end = matched.size();
  while (end > 0 &&
         std::isdigit(static_cast<unsigned char>(matched[end - 1]))) {
    --end;
  }
  std::size_t start = end;
  while (start < matched.size() &&
         std::isdigit(static_cast<unsigned char>(matched[start]))) {
    ++start;
  }
  if (start == end) {
    throw lexical_error("internal: kv value missing digit run", pos, source);
  }
  std::size_t value = 0;
  auto const *first = matched.data() + end;
  auto const *last = matched.data() + start;
  auto [ptr, ec] = std::from_chars(first, last, value);
  if (ec != std::errc{}) {
    throw lexical_error("malformed integer in tensor declaration", pos, source);
  }
  return value;
}
} // namespace detail_kv

template <> struct action<grammar::rank_kv> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    if (state.pending_rank.has_value()) {
      throw syntax_error("tensor declaration: duplicate 'rank=' keyword",
                         in.position().byte, state.source);
    }
    state.pending_rank = detail_kv::parse_kv_integer(
        in.string_view(), in.position().byte, state.source);
  }
};
template <> struct action<grammar::dim_kv> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    if (state.pending_dim.has_value()) {
      throw syntax_error("tensor declaration: duplicate 'dim=' keyword",
                         in.position().byte, state.source);
    }
    state.pending_dim = detail_kv::parse_kv_integer(
        in.string_view(), in.position().byte, state.source);
  }
};

template <> struct action<grammar::tensor_decl> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    auto pos = in.position().byte;
    auto matched = in.string_view();
    // Tensor name = matched range up to first non-identifier char.
    std::size_t name_end = 0;
    while (name_end < matched.size() &&
           (std::isalnum(static_cast<unsigned char>(matched[name_end])) ||
            matched[name_end] == '_')) {
      ++name_end;
    }
    std::string name(matched.substr(0, name_end));

    if (!state.pending_rank.has_value() || !state.pending_dim.has_value()) {
      // Defensive: even though if_must should have caught the missing
      // brace path, reset pending fields before throwing in case some
      // partial state lingers from a prior failed kv match.
      state.pending_rank.reset();
      state.pending_dim.reset();
      throw syntax_error("tensor declaration '" + name +
                             "{…}' must specify both rank= and dim=",
                         pos, state.source);
    }
    auto rank = *state.pending_rank;
    auto dim = *state.pending_dim;
    state.pending_rank.reset();
    state.pending_dim.reset();

    // Validate rank and dim before handing to the tensor constructor.
    // The constructor takes both verbatim; without this check, rank=0
    // or dim=0 silently builds a degenerate tensor that explodes
    // downstream at a confusing site.
    if (rank == 0) {
      throw syntax_error("tensor declaration '" + name +
                             "': rank must be >= 1, got 0",
                         pos, state.source);
    }
    if (dim == 0) {
      throw syntax_error("tensor declaration '" + name +
                             "': dim must be >= 1, got 0",
                         pos, state.source);
    }

    try {
      auto expr = state.syms.get_or_declare_tensor(name, rank, dim);
      state.values.emplace_back(std::move(expr));
    } catch (parse_error const &e) {
      // symbol_table threw position-less; re-throw with our pos.
      // dynamic_cast preserves the redeclaration_error subtype so a
      // caller can pattern-match on the error kind.
      if (dynamic_cast<redeclaration_error const *>(&e)) {
        throw redeclaration_error(e.what(), pos, state.source);
      }
      throw type_collision_error(e.what(), pos, state.source);
    }
  }
};

} // namespace numsim::cas::parser::actions

#endif // NUMSIM_CAS_PARSER_ACTIONS_H
