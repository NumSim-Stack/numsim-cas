#ifndef NUMSIM_CAS_PARSER_GRAMMAR_H
#define NUMSIM_CAS_PARSER_GRAMMAR_H

// PEGTL grammar for the numsim-cas expression parser (issue #214).
//
// This is a SRC-PRIVATE header — it includes PEGTL and pulls in
// substantial template code. Public consumers go through
// `<numsim_cas/parser/parser.h>` which does NOT include this file.
//
// Phase 2a coverage: numbers (int + decimal), bare identifiers,
// parentheses, `+ - * /`. Subsequent commits add power, unary minus,
// comparisons, function calls, and the tensor-declaration brace
// syntax.

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
struct plus_op : pegtl::one<'+'> {};
struct minus_op : pegtl::one<'-'> {};
struct star_op : pegtl::one<'*'> {};
struct slash_op : pegtl::one<'/'> {};

// ─── Forward decls ────────────────────────────────────────────────
struct expression;

// ─── Primary: literal | identifier | '(' expression ')' ───────────
// Order: number first (a leading digit can never start an identifier
// or a parenthesised expression, so trying number_literal first is
// safe and cheap), then identifier, then parens.
struct paren_expression
    : pegtl::seq<pegtl::one<'('>, ws, expression, ws, pegtl::one<')'>> {};

struct primary : pegtl::sor<number_literal, identifier, paren_expression> {};

// ─── Multiplicative level: primary (('*'|'/') primary)* ───────────
// Per-op tail rules so the action template specialization for each
// op can dispatch statically. (A single `mul_tail` with a `sor` over
// star/slash would force the action to inspect the matched range
// at runtime to figure out which op fired — error-prone and forced
// us into a fragile `pending_op` variable in an earlier attempt.)
struct mul_tail_star : pegtl::seq<ws, star_op, ws, primary> {};
struct mul_tail_slash : pegtl::seq<ws, slash_op, ws, primary> {};
struct mul_tail : pegtl::sor<mul_tail_star, mul_tail_slash> {};
struct mul_term : pegtl::seq<primary, pegtl::star<mul_tail>> {};

// ─── Additive level: mul_term (('+'|'-') mul_term)* ───────────────
struct add_tail_plus : pegtl::seq<ws, plus_op, ws, mul_term> {};
struct add_tail_minus : pegtl::seq<ws, minus_op, ws, mul_term> {};
struct add_tail : pegtl::sor<add_tail_plus, add_tail_minus> {};
struct add_term : pegtl::seq<mul_term, pegtl::star<add_tail>> {};

// Top-level expression production.
struct expression : add_term {};

// Grammar root: optional leading/trailing whitespace, must consume
// to end of input.
struct grammar : pegtl::must<ws, expression, ws, pegtl::eof> {};

} // namespace numsim::cas::parser::grammar

#endif // NUMSIM_CAS_PARSER_GRAMMAR_H
