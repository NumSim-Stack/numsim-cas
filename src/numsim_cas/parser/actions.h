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

#include "grammar.h"

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/parser/parse_error.h>
#include <numsim_cas/parser/parser.h>
#include <numsim_cas/parser/symbol_table.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>

#include <tao/pegtl.hpp>

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
  std::vector<parsed_expression> values;
  symbol_table &syms;
  std::string_view source;

  explicit parser_state(symbol_table &s, std::string_view src) noexcept
      : syms(s), source(src) {}
};

// Helper: extract a scalar holder from a value-stack entry. Phase 2a
// expects every value to be scalar; mixed-domain combinations are a
// later-phase concern.
inline expression_holder<scalar_expression> &
require_scalar(parsed_expression &v, std::size_t pos, std::string_view source) {
  if (auto *s = std::get_if<expression_holder<scalar_expression>>(&v))
    return *s;
  throw type_mismatch_error("operator expects scalar operands; got non-scalar",
                            pos, source);
}

// Pop the top two scalar operands. `pos` is the byte offset of the
// operator for error reporting.
template <typename Op>
void combine_top_two(parser_state &state, std::size_t pos, Op &&op) {
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

// Pop the top scalar operand and replace it with op(top). Used by
// unary minus.
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

// identifier: look up (or implicitly declare) as a scalar variable
// in the symbol table. Phase 2d will branch on tensor-vs-scalar.
template <> struct action<grammar::identifier> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    try {
      auto expr = state.syms.get_or_declare_scalar(in.string_view());
      state.values.emplace_back(std::move(expr));
    } catch (parse_error const &e) {
      // symbol_table threw a position-less error; re-throw with our
      // input context attached.
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
    combine_top_two(state, in.position().byte,
                    [](auto &&a, auto &&b) { return a * b; });
  }
};
template <> struct action<grammar::mul_tail_slash> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    combine_top_two(state, in.position().byte,
                    [](auto &&a, auto &&b) { return a / b; });
  }
};
template <> struct action<grammar::add_tail_plus> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    combine_top_two(state, in.position().byte,
                    [](auto &&a, auto &&b) { return a + b; });
  }
};
template <> struct action<grammar::add_tail_minus> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    combine_top_two(state, in.position().byte,
                    [](auto &&a, auto &&b) { return a - b; });
  }
};

// ─── Power (right-associative) ────────────────────────────────────
// `power_tail` matches `^ <power>` where the inner `power` recurses
// — by the time this action fires, the inner power has fully
// reduced and pushed its result. So the stack has [..., lhs, rhs].
template <> struct action<grammar::power_tail> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    combine_top_two(state, in.position().byte,
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
    combine_top_two(state, in.position().byte,
                    [](auto &&a, auto &&b) { return lt(a, b); });
  }
};
template <> struct action<grammar::cmp_tail_le> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    combine_top_two(state, in.position().byte,
                    [](auto &&a, auto &&b) { return le(a, b); });
  }
};
template <> struct action<grammar::cmp_tail_gt> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    combine_top_two(state, in.position().byte,
                    [](auto &&a, auto &&b) { return gt(a, b); });
  }
};
template <> struct action<grammar::cmp_tail_ge> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    combine_top_two(state, in.position().byte,
                    [](auto &&a, auto &&b) { return ge(a, b); });
  }
};
template <> struct action<grammar::eq_tail_eq> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    combine_top_two(state, in.position().byte,
                    [](auto &&a, auto &&b) { return eq(a, b); });
  }
};
template <> struct action<grammar::eq_tail_ne> {
  template <typename Input>
  static void apply(Input const &in, parser_state &state) {
    combine_top_two(state, in.position().byte,
                    [](auto &&a, auto &&b) { return ne(a, b); });
  }
};

} // namespace numsim::cas::parser::actions

#endif // NUMSIM_CAS_PARSER_ACTIONS_H
