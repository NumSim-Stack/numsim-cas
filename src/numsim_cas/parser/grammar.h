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
// Subsequent commits add function calls (2c) and tensor declarations
// + contraction syntax (2d).

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

// ─── Primary: literal | identifier | '(' expression ')' ───────────
struct paren_expression
    : pegtl::seq<pegtl::one<'('>, ws, expression, ws, pegtl::one<')'>> {};

struct primary : pegtl::sor<number_literal, identifier, paren_expression> {};

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
