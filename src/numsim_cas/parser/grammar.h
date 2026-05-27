#ifndef NUMSIM_CAS_PARSER_GRAMMAR_H
#define NUMSIM_CAS_PARSER_GRAMMAR_H

// PEGTL grammar for the numsim-cas expression parser (issue #214).
//
// This is a SRC-PRIVATE header — it includes PEGTL and pulls in
// substantial template code. Public consumers go through
// `<numsim_cas/parser/parser.h>` which does NOT include this file.
//
// Phase 2a coverage: numbers (int + decimal), bare identifiers,
// parentheses, `+ - * /`.
// Phase 2b additions: right-associative `^`, unary `-`, comparison
// operators (`<`, `<=`, `>`, `>=`, `==`, `!=`).
// Phase 2c additions: function calls — scalar→scalar functions
// (trig + hyperbolic + exp/log/sqrt/abs/sign + pow + comparisons)
// are dispatched through a static registry in function_registry.h.
// The grammar uses a distinct `function_name` rule (no action) so
// the bare-identifier action doesn't fire when matching the prefix
// of a call; if_must<(> commits once the open paren is consumed so
// backtracking past it can't smear state.
// Phase 2d additions: tensor declarations (`A{rank=R, dim=D}`),
// tensor→tensor functions (trans, inv) and tensor→t2s functions
// (trace, det, norm, dot). Bare identifiers now look up their
// existing declaration in the symbol_table and push the existing
// holder (scalar or tensor) — only fall back to implicit scalar
// declaration when the name is unknown.
// Phase 2e additions: bracket-list index notation `[i, j]` for
// `inner_product(A, [3,4], B, [1,2])` and
// `dot_product(A, [1,2], B, [1,2])`. The index list is a new
// grammar primary inside `arg_item` — it's only accepted in
// function-call arg lists, not as a free-standing primary.
// (`outer_product` has no top-level free function on this branch,
// so it isn't in the registry.)
//
// What's still missing (deferred to a future phase or to the
// upstream library): max/min/if_then_else/Macauley family (need
// #207/#208/#209 to land on main); dev/sym/vol/skew projectors
// (need projection_tensor.h, also from the unmerged stack);
// upper-bound validation on contraction indices (delegated to the
// underlying library at evaluation time).

#include <tao/pegtl.hpp>

namespace numsim::cas::parser::grammar {

namespace pegtl = tao::pegtl;

// ─── Whitespace ────────────────────────────────────────────────────
// Skipped freely between tokens, never inside identifiers or numbers.
struct ws : pegtl::star<pegtl::space> {};

// ─── Numeric literals ─────────────────────────────────────────────
// integer:  one or more digits
// decimal:  digits '.' digits   OR   digits '.'   (e.g. "2.")
// number:   decimal | integer  (decimal first so the parser doesn't
//           greedily consume the leading digits as an integer)
struct integer_literal : pegtl::plus<pegtl::digit> {};
struct decimal_literal : pegtl::seq<pegtl::plus<pegtl::digit>, pegtl::one<'.'>,
                                    pegtl::star<pegtl::digit>> {};
struct number_literal : pegtl::sor<decimal_literal, integer_literal> {};

// ─── Identifiers ──────────────────────────────────────────────────
// `[a-zA-Z_][a-zA-Z0-9_]*`. The parser treats every bare identifier
// as a scalar variable (declared implicitly on first use).
struct identifier_first : pegtl::sor<pegtl::alpha, pegtl::one<'_'>> {};
struct identifier_rest : pegtl::sor<pegtl::alnum, pegtl::one<'_'>> {};
struct identifier : pegtl::seq<identifier_first, pegtl::star<identifier_rest>> {
};

// ─── Operator tokens ──────────────────────────────────────────────
// Single-character tokens. Multi-character tokens are listed below
// in priority order (try multi-char first when alternatives share a
// prefix, e.g. `<=` before `<`).
struct plus_op : pegtl::one<'+'> {};
struct minus_op : pegtl::one<'-'> {};
struct star_op : pegtl::one<'*'> {};
struct slash_op : pegtl::one<'/'> {};
struct caret_op : pegtl::one<'^'> {};

// Comparison tokens. Two-char variants must come before their
// single-char siblings in the `sor<>` that wraps them — otherwise
// `<=` parses as `<` followed by `=` which doesn't match anything.
struct eq_eq_op : pegtl::string<'=', '='> {};
struct ne_op : pegtl::string<'!', '='> {};
struct le_op : pegtl::string<'<', '='> {};
struct ge_op : pegtl::string<'>', '='> {};
struct lt_op : pegtl::one<'<'> {};
struct gt_op : pegtl::one<'>'> {};

// ─── Forward decls ────────────────────────────────────────────────
struct expression;
struct unary;
struct power;

// ─── Function calls ───────────────────────────────────────────────
// `function_name` has the same shape as `identifier` but is a
// distinct PEGTL rule so we don't share its action template
// specialisation. That keeps the bare-identifier scalar-variable
// push out of the function-call match path.
//
// `function_call_open` / `function_call_close` are distinct from
// the generic `(` / `)` used in `paren_expression` for the same
// reason — their action records the value-stack depth so we know
// how many args were pushed by the time the call finishes.
//
// Once `(` is matched, the rest is wrapped in `if_must` so a
// missing close-paren (or malformed argument) raises a hard error
// rather than backtracking to try `identifier` and leaving state
// in an inconsistent shape.
struct function_name
    : pegtl::seq<identifier_first, pegtl::star<identifier_rest>> {};
struct function_call_open : pegtl::one<'('> {};
struct function_call_close : pegtl::one<')'> {};

// ─── Bracket-list index literal (phase 2e) ────────────────────────
// `[i1, i2, …]` — only accepted as an `arg_item`, not as a primary.
// Inside contraction-function arg lists (`inner_product`,
// `dot_product`). Indices are 1-based positive integers; the action
// converts to 0-based when constructing the `sequence`.
struct index_list_open : pegtl::one<'['> {};
struct index_list_close : pegtl::one<']'> {};
struct index_list_literal
    : pegtl::seq<
          index_list_open, ws,
          pegtl::if_must<pegtl::list<integer_literal,
                                     pegtl::pad<pegtl::one<','>, pegtl::space>>,
                         ws, index_list_close>> {};

// An arg in a function call is either an index_list literal or a
// full expression. Order matters: index_list must come first so the
// `[` token isn't (mis)parsed as something else.
struct arg_item : pegtl::sor<index_list_literal, expression> {};
struct arg_list
    : pegtl::list<arg_item, pegtl::pad<pegtl::one<','>, pegtl::space>> {};
struct function_call
    : pegtl::seq<function_name, ws,
                 pegtl::if_must<function_call_open, ws, pegtl::opt<arg_list>,
                                ws, function_call_close>> {};

// ─── Tensor declarations: Name{rank=R, dim=D} ─────────────────────
// Same no-action-on-name + if_must<brace> pattern as function_call.
// The kv pairs may appear in either order. Re-declaration with a
// different (rank, dim) raises redeclaration_error from the
// symbol_table; identical re-declaration is a no-op.
struct rank_keyword : pegtl::string<'r', 'a', 'n', 'k'> {};
struct dim_keyword : pegtl::string<'d', 'i', 'm'> {};
struct kv_eq : pegtl::one<'='> {};
struct rank_kv : pegtl::seq<rank_keyword, ws, kv_eq, ws, integer_literal> {};
struct dim_kv : pegtl::seq<dim_keyword, ws, kv_eq, ws, integer_literal> {};
struct tensor_kv : pegtl::sor<rank_kv, dim_kv> {};
struct tensor_kv_list
    : pegtl::list<tensor_kv, pegtl::pad<pegtl::one<','>, pegtl::space>> {};

struct tensor_name
    : pegtl::seq<identifier_first, pegtl::star<identifier_rest>> {};
struct tensor_decl_open : pegtl::one<'{'> {};
struct tensor_decl_close : pegtl::one<'}'> {};
struct tensor_decl
    : pegtl::seq<tensor_name, ws,
                 pegtl::if_must<tensor_decl_open, ws, tensor_kv_list, ws,
                                tensor_decl_close>> {};

// ─── Primary: literal | function_call | tensor_decl | identifier | parens
// Order matters: function_call before tensor_decl before identifier
// so `name(` / `name{` prefixes aren't consumed as bare scalar
// variables. Both compound alternatives backtrack cleanly when their
// distinguishing token (`(` or `{`) doesn't follow the name.
struct paren_expression
    : pegtl::seq<pegtl::one<'('>, ws, expression, ws, pegtl::one<')'>> {};

struct primary : pegtl::sor<number_literal, function_call, tensor_decl,
                            identifier, paren_expression> {};

// ─── Power level (highest precedence): primary ('^' power)? ──────
// Right-recursive: the `^`'s right operand is `power` itself, NOT
// `primary`. That makes `2 ^ 3 ^ 2` parse as `2 ^ (3 ^ 2) = 512`.
// (PEGTL's `list<X, Y>` is left-associative; the right-recursion
// here is the canonical workaround.)
struct power_tail : pegtl::seq<ws, caret_op, ws, power> {};
struct power : pegtl::seq<primary, pegtl::opt<power_tail>> {};

// ─── Unary minus: '-' unary | power ──────────────────────────────
// Unary `-` binds tighter than `*` (so `-2 * x = (-2) * x`) but
// looser than `^` (so `-x^2 = -(x^2)`). Right-recursive too so
// `- - x` parses as `-(-x)` (which the construction-time simplifier
// then folds to x).
struct unary_minus : pegtl::seq<minus_op, ws, unary> {};
struct unary : pegtl::sor<unary_minus, power> {};

// ─── Multiplicative level: unary (('*'|'/') unary)* ──────────────
// Per-op tail rules so each action specialisation dispatches
// statically. See the doc comment at the head of this file for why
// the single-tail-with-sor-over-ops approach was rejected.
struct mul_tail_star : pegtl::seq<ws, star_op, ws, unary> {};
struct mul_tail_slash : pegtl::seq<ws, slash_op, ws, unary> {};
struct mul_tail : pegtl::sor<mul_tail_star, mul_tail_slash> {};
struct mul_term : pegtl::seq<unary, pegtl::star<mul_tail>> {};

// ─── Additive level: mul_term (('+'|'-') mul_term)* ──────────────
struct add_tail_plus : pegtl::seq<ws, plus_op, ws, mul_term> {};
struct add_tail_minus : pegtl::seq<ws, minus_op, ws, mul_term> {};
struct add_tail : pegtl::sor<add_tail_plus, add_tail_minus> {};
struct add_term : pegtl::seq<mul_term, pegtl::star<add_tail>> {};

// ─── Comparison level: add_term ((< | <= | > | >=) add_term)* ────
// Comparisons evaluate to scalar indicators (1.0 / 0.0) following
// Option B from #136. The two-char variants come first in `sor`.
struct cmp_tail_le : pegtl::seq<ws, le_op, ws, add_term> {};
struct cmp_tail_ge : pegtl::seq<ws, ge_op, ws, add_term> {};
struct cmp_tail_lt : pegtl::seq<ws, lt_op, ws, add_term> {};
struct cmp_tail_gt : pegtl::seq<ws, gt_op, ws, add_term> {};
struct cmp_tail
    : pegtl::sor<cmp_tail_le, cmp_tail_ge, cmp_tail_lt, cmp_tail_gt> {};
struct cmp_term : pegtl::seq<add_term, pegtl::star<cmp_tail>> {};

// ─── Equality level (lowest precedence): cmp_term ((== | !=) cmp_term)* ──
struct eq_tail_eq : pegtl::seq<ws, eq_eq_op, ws, cmp_term> {};
struct eq_tail_ne : pegtl::seq<ws, ne_op, ws, cmp_term> {};
struct eq_tail : pegtl::sor<eq_tail_eq, eq_tail_ne> {};
struct eq_term : pegtl::seq<cmp_term, pegtl::star<eq_tail>> {};

// Top-level expression production.
struct expression : eq_term {};

// Grammar root: optional leading/trailing whitespace, must consume
// to end of input.
struct grammar : pegtl::must<ws, expression, ws, pegtl::eof> {};

} // namespace numsim::cas::parser::grammar

#endif // NUMSIM_CAS_PARSER_GRAMMAR_H
