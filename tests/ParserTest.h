#ifndef PARSERTEST_H
#define PARSERTEST_H

// Parser tests for issue #214 — the PEGTL string parser.
//
// File is always included in tests/main.cpp; body is gated on
// NUMSIM_CAS_PARSER_ENABLED, defined only when the CMake option
// NUMSIM_CAS_BUILD_PARSER=ON wires up the NumSim_CAS::Parser library
// target and links it into the test binary. When the option is OFF
// this file is a no-op.

#ifdef NUMSIM_CAS_PARSER_ENABLED

#include <gtest/gtest.h>

#include <numsim_cas/parser/parse_error.h>
#include <numsim_cas/parser/parser.h>
#include <numsim_cas/parser/symbol_table.h>
#include <numsim_cas/scalar/visitors/scalar_evaluator.h>

#include <string>
#include <variant>

namespace numsim::cas::parser_test {

// Bring the parser's public types into scope unqualified to keep test
// bodies readable. Restricted to this anonymous-ish translation-unit
// namespace; doesn't leak to other test files.
using numsim::cas::parser::arity_error;
using numsim::cas::parser::parse;
using numsim::cas::parser::parse_error;
using numsim::cas::parser::parse_scalar;
using numsim::cas::parser::parse_t2s;
using numsim::cas::parser::parse_tensor;
using numsim::cas::parser::parsed_expression;
using numsim::cas::parser::redeclaration_error;
using numsim::cas::parser::symbol_table;
using numsim::cas::parser::syntax_error;
using numsim::cas::parser::type_collision_error;
using numsim::cas::parser::type_mismatch_error;
using numsim::cas::parser::unknown_symbol_error;

// ─── symbol_table: scalar declarations ─────────────────────────────

TEST(SymbolTable, ImplicitScalarOnFirstUse) {
  symbol_table st;
  auto x = st.get_or_declare_scalar("x");
  EXPECT_TRUE(x.is_valid());
  EXPECT_TRUE(st.has("x"));
}

TEST(SymbolTable, RepeatedScalarLookupReturnsSameHolder) {
  symbol_table st;
  auto a = st.get_or_declare_scalar("alpha");
  auto b = st.get_or_declare_scalar("alpha");
  EXPECT_EQ(a, b)
      << "Repeated lookups of an implicitly-declared scalar must return "
         "the same expression_holder so structural equality across the "
         "parse holds.";
}

TEST(SymbolTable, DifferentScalarsHaveDistinctHolders) {
  symbol_table st;
  auto x = st.get_or_declare_scalar("x");
  auto y = st.get_or_declare_scalar("y");
  EXPECT_NE(x, y);
}

// ─── symbol_table: tensor declarations ─────────────────────────────

TEST(SymbolTable, TensorDeclaresWithRankAndDim) {
  symbol_table st;
  auto a = st.get_or_declare_tensor("A", /*rank=*/2, /*dim=*/3);
  EXPECT_TRUE(a.is_valid());
  EXPECT_EQ(a.get().rank(), 2u);
  EXPECT_EQ(a.get().dim(), 3u);
}

TEST(SymbolTable, TensorIdenticalRedeclarationReturnsSameHolder) {
  symbol_table st;
  auto a1 = st.get_or_declare_tensor("A", 2, 3);
  auto a2 = st.get_or_declare_tensor("A", 2, 3);
  EXPECT_EQ(a1, a2);
}

TEST(SymbolTable, TensorRedeclarationWithDifferentRankThrows) {
  symbol_table st;
  st.get_or_declare_tensor("A", 2, 3);
  EXPECT_THROW(
      { [[maybe_unused]] auto e = st.get_or_declare_tensor("A", 4, 3); },
      redeclaration_error);
}

TEST(SymbolTable, TensorRedeclarationWithDifferentDimThrows) {
  symbol_table st;
  st.get_or_declare_tensor("A", 2, 3);
  EXPECT_THROW(
      { [[maybe_unused]] auto e = st.get_or_declare_tensor("A", 2, 2); },
      redeclaration_error);
}

// ─── symbol_table: cross-type collision ────────────────────────────

TEST(SymbolTable, ScalarThenTensorThrowsCollision) {
  symbol_table st;
  st.get_or_declare_scalar("X");
  EXPECT_THROW(
      { [[maybe_unused]] auto e = st.get_or_declare_tensor("X", 2, 3); },
      type_collision_error);
}

TEST(SymbolTable, TensorThenScalarThrowsCollision) {
  symbol_table st;
  st.get_or_declare_tensor("X", 2, 3);
  EXPECT_THROW(
      { [[maybe_unused]] auto e = st.get_or_declare_scalar("X"); },
      type_collision_error);
}

// ─── symbol_table: read-only inspection ────────────────────────────

TEST(SymbolTable, TensorShapeReturnsRankAndDim) {
  symbol_table st;
  st.get_or_declare_tensor("A", 4, 3);
  auto shape = st.tensor_shape("A");
  ASSERT_TRUE(shape.has_value());
  EXPECT_EQ(shape->first, 4u);
  EXPECT_EQ(shape->second, 3u);
}

TEST(SymbolTable, TensorShapeOfScalarReturnsNullopt) {
  symbol_table st;
  st.get_or_declare_scalar("x");
  EXPECT_FALSE(st.tensor_shape("x").has_value());
}

TEST(SymbolTable, TensorShapeOfUnknownReturnsNullopt) {
  symbol_table st;
  EXPECT_FALSE(st.tensor_shape("never_declared").has_value());
}

TEST(SymbolTable, HasReportsBothScalarAndTensor) {
  symbol_table st;
  st.get_or_declare_scalar("x");
  st.get_or_declare_tensor("A", 2, 3);
  EXPECT_TRUE(st.has("x"));
  EXPECT_TRUE(st.has("A"));
  EXPECT_FALSE(st.has("y"));
}

// ─── parse_error: snippet rendering ────────────────────────────────

TEST(ParseError, NoSourceGivesBareMessage) {
  parse_error e("something went wrong", 0, std::string_view{});
  std::string what = e.what();
  EXPECT_EQ(what, "parse error: something went wrong");
  EXPECT_FALSE(e.has_position());
}

TEST(ParseError, SingleLineSourceRendersSnippetWithCaret) {
  std::string src = "sin(x + 2";
  parse_error e("expected ')'", /*pos=*/src.size(), src);
  std::string what = e.what();
  // Header reports line=1, col=10 (one past the last char).
  EXPECT_NE(what.find("at 1:10:"), std::string::npos)
      << "Header missing line:col. what() was:\n"
      << what;
  // Snippet must include the offending line and a caret at col 10.
  EXPECT_NE(what.find("sin(x + 2"), std::string::npos);
  EXPECT_NE(what.find("\n           ^"), std::string::npos)
      << "Caret should be at column 10 (9 spaces + ^). what() was:\n"
      << what;
  EXPECT_TRUE(e.has_position());
  EXPECT_EQ(e.line(), 1u);
  EXPECT_EQ(e.column(), 10u);
}

TEST(ParseError, MultiLineSourceCaretAlignsWithCorrectLine) {
  std::string src = "line one\nline two has the error here\nline three";
  // Point at 'e' in "error" — column 18 of line 2.
  // ("line two has the " is 17 chars; 'e' is at column 18, 1-based.)
  auto pos = src.find("error");
  ASSERT_NE(pos, std::string::npos);
  parse_error e("unexpected 'error'", pos, src);
  EXPECT_EQ(e.line(), 2u);
  EXPECT_EQ(e.column(), 18u);
  std::string what = e.what();
  // Snippet must be the *second* line, not the first.
  EXPECT_NE(what.find("line two has the error here"), std::string::npos);
  EXPECT_EQ(what.find("line one"), std::string::npos)
      << "First line should not appear in the snippet. what() was:\n"
      << what;
  // Caret should be at column 18 (17 spaces + ^).
  EXPECT_NE(what.find("\n  " + std::string(17, ' ') + '^'), std::string::npos)
      << "Caret column mismatch. what() was:\n"
      << what;
}

// ─── parse_error: subclass extra fields ────────────────────────────

TEST(ParseError, UnknownSymbolErrorCarriesName) {
  unknown_symbol_error e("foo", 0, std::string_view{});
  EXPECT_EQ(e.name(), "foo");
  EXPECT_NE(std::string(e.what()).find("foo"), std::string::npos);
}

TEST(ParseError, ArityErrorCarriesCounts) {
  arity_error e("sin", /*expected=*/1, /*actual=*/2, 0, std::string_view{});
  EXPECT_EQ(e.function(), "sin");
  EXPECT_EQ(e.expected_arity(), 1u);
  EXPECT_EQ(e.actual_arity(), 2u);
  EXPECT_NE(std::string(e.what()).find("sin"), std::string::npos);
  EXPECT_NE(std::string(e.what()).find("1"), std::string::npos);
  EXPECT_NE(std::string(e.what()).find("2"), std::string::npos);
}

// ─── Grammar (phase 2a): numbers, identifiers, + - * / and parens ──

// Helper: evaluate a scalar expression with all named scalar symbols
// in `bindings` bound to the given doubles. Returns the result.
inline double eval_scalar(
    expression_holder<scalar_expression> const &expr, symbol_table &syms,
    std::initializer_list<std::pair<std::string, double>> bindings = {}) {
  numsim::cas::scalar_evaluator<double> ev;
  for (auto const &[name, value] : bindings) {
    ev.set(syms.get_or_declare_scalar(name), value);
  }
  return ev.apply(expr);
}

TEST(ParserGrammar, IntegerLiteralEvaluatesToValue) {
  symbol_table syms;
  EXPECT_DOUBLE_EQ(eval_scalar(parse_scalar("42", syms), syms), 42.0);
}

TEST(ParserGrammar, DecimalLiteralEvaluatesToValue) {
  symbol_table syms;
  EXPECT_DOUBLE_EQ(eval_scalar(parse_scalar("3.14", syms), syms), 3.14);
}

TEST(ParserGrammar, IdentifierResolvesToScalarVariable) {
  symbol_table syms;
  auto e = parse_scalar("x", syms);
  EXPECT_TRUE(syms.has("x"));
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 7.0}}), 7.0);
}

TEST(ParserGrammar, AdditionLeftAssociative) {
  symbol_table syms;
  // a + b + c parses as ((a + b) + c). Either is fine for + since it's
  // associative, but the lock-in test pins evaluation regardless.
  auto e = parse_scalar("a + b + c", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"a", 1}, {"b", 2}, {"c", 3}}), 6.0);
}

TEST(ParserGrammar, SubtractionLeftAssociative) {
  symbol_table syms;
  // a - b - c must parse as ((a - b) - c) = a - b - c, NOT a - (b - c).
  auto e = parse_scalar("10 - 3 - 2", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms), 5.0)
      << "Subtraction must be left-associative.";
}

TEST(ParserGrammar, MultiplicationBindsTighterThanAddition) {
  symbol_table syms;
  // 2 + 3 * 4 should be 14, not 20.
  auto e = parse_scalar("2 + 3 * 4", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms), 14.0);
}

TEST(ParserGrammar, DivisionLeftAssociative) {
  symbol_table syms;
  // 100 / 10 / 2 must be 5, not 20.
  auto e = parse_scalar("100 / 10 / 2", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms), 5.0)
      << "Division must be left-associative.";
}

TEST(ParserGrammar, ParensOverridePrecedence) {
  symbol_table syms;
  auto e = parse_scalar("(2 + 3) * 4", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms), 20.0);
}

TEST(ParserGrammar, NestedParensEvaluateCorrectly) {
  symbol_table syms;
  auto e = parse_scalar("((1 + 2) * (3 + 4))", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms), 21.0);
}

TEST(ParserGrammar, WhitespaceIsSkippedFreely) {
  symbol_table syms;
  auto e = parse_scalar("  1  +  2  *  3  ", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms), 7.0);
}

TEST(ParserGrammar, IdentifierAndLiteralCombine) {
  symbol_table syms;
  auto e = parse_scalar("2 * x + 3", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 5}}), 13.0);
}

TEST(ParserGrammar, SameIdentifierTwiceUsesSameVariable) {
  // Critical: `x + x` should evaluate to `2x`, NOT to `x + freshly-
  // declared-x`. Verifies the symbol_table lookup-or-declare path
  // returns the same holder on both lookups.
  symbol_table syms;
  auto e = parse_scalar("x + x", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 4}}), 8.0);
}

TEST(ParserGrammar, ParserReturnsScalarVariant) {
  symbol_table syms;
  auto result = parse("1 + 2", syms);
  EXPECT_TRUE(
      std::holds_alternative<expression_holder<scalar_expression>>(result))
      << "Top-level scalar expression should land in the scalar variant "
         "alternative.";
}

TEST(ParserGrammar, ParseTensorOnScalarExpressionThrows) {
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse_tensor("1 + 2", syms); },
      type_mismatch_error);
}

TEST(ParserGrammar, ParseT2sOnScalarExpressionThrows) {
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse_t2s("1 + 2", syms); },
      type_mismatch_error);
}

TEST(ParserGrammar, MalformedInputThrowsSyntaxError) {
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse_scalar("1 +", syms); }, parse_error);
}

TEST(ParserGrammar, TrailingGarbageThrows) {
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse_scalar("1 + 2 banana", syms); },
      parse_error);
}

} // namespace numsim::cas::parser_test

#endif // NUMSIM_CAS_PARSER_ENABLED

#endif // PARSERTEST_H
