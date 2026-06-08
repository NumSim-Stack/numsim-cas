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

#include "cas_test_helpers.h"

#include <numsim_cas/parser/parse_error.h>
#include <numsim_cas/parser/parser.h>
#include <numsim_cas/parser/symbol_table.h>
#include <numsim_cas/scalar/scalar_diff.h>
#include <numsim_cas/scalar/visitors/scalar_evaluator.h>
#include <numsim_cas/tensor/data/tensor_data.h>
#include <numsim_cas/tensor/identity_tensor.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_diff.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_evaluator.h>

#include <cmath>
#include <locale>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>

namespace numsim::cas::parser_test {

// Bring the parser's public types into scope unqualified to keep test
// bodies readable. Restricted to this anonymous-ish translation-unit
// namespace; doesn't leak to other test files.
using numsim::cas::parser::arity_error;
using numsim::cas::parser::lexical_error;
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
using numsim::cas::parser::unknown_function_error;
using numsim::cas::parser::unknown_symbol_error;

// numsim_cas core API used by integration tests.
using numsim::cas::diff;
using numsim::cas::identity_tensor;
using numsim::cas::make_expression;

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

// ─── Phase 2b: power, unary minus, comparisons ─────────────────────

TEST(ParserGrammar, PowerBasic) {
  symbol_table syms;
  EXPECT_DOUBLE_EQ(eval_scalar(parse_scalar("2 ^ 3", syms), syms), 8.0);
}

TEST(ParserGrammar, PowerRightAssociative) {
  // 2 ^ 3 ^ 2 must be 2^(3^2) = 2^9 = 512, NOT (2^3)^2 = 64.
  // Lock-in for the right-recursive grammar (the known PEGTL gotcha
  // documented in grammar.h).
  symbol_table syms;
  EXPECT_DOUBLE_EQ(eval_scalar(parse_scalar("2 ^ 3 ^ 2", syms), syms), 512.0);
}

TEST(ParserGrammar, PowerBindsTighterThanMul) {
  // 2 * 3 ^ 2 = 2 * 9 = 18, NOT (2*3)^2 = 36.
  symbol_table syms;
  EXPECT_DOUBLE_EQ(eval_scalar(parse_scalar("2 * 3 ^ 2", syms), syms), 18.0);
}

TEST(ParserGrammar, UnaryMinusOnLiteral) {
  symbol_table syms;
  EXPECT_DOUBLE_EQ(eval_scalar(parse_scalar("-5", syms), syms), -5.0);
}

TEST(ParserGrammar, UnaryMinusOnIdentifier) {
  symbol_table syms;
  EXPECT_DOUBLE_EQ(eval_scalar(parse_scalar("-x", syms), syms, {{"x", 3.0}}),
                   -3.0);
}

TEST(ParserGrammar, UnaryMinusBindsTighterThanMul) {
  // -2 * x = (-2) * x, evaluated at x=4 → -8.
  symbol_table syms;
  EXPECT_DOUBLE_EQ(
      eval_scalar(parse_scalar("-2 * x", syms), syms, {{"x", 4.0}}), -8.0);
}

TEST(ParserGrammar, PowerBindsTighterThanUnaryMinus) {
  // -x^2 = -(x^2), evaluated at x=3 → -9.
  symbol_table syms;
  EXPECT_DOUBLE_EQ(
      eval_scalar(parse_scalar("-x ^ 2", syms), syms, {{"x", 3.0}}), -9.0);
}

TEST(ParserGrammar, DoubleUnaryMinusCancels) {
  // - -x = -(-x) = x. The construction-time simplifier folds the
  // double-negation away; either evaluation order gives the same
  // numeric result.
  symbol_table syms;
  EXPECT_DOUBLE_EQ(eval_scalar(parse_scalar("- -x", syms), syms, {{"x", 5.0}}),
                   5.0);
}

TEST(ParserGrammar, BinaryMinusVsUnaryMinusAfterBinary) {
  // 5 - -3 should be 5 + 3 = 8 (binary minus then unary minus on 3).
  symbol_table syms;
  EXPECT_DOUBLE_EQ(eval_scalar(parse_scalar("5 - -3", syms), syms), 8.0);
}

TEST(ParserGrammar, ComparisonLtReturnsIndicator) {
  // Comparisons produce Option B scalar indicators: 1.0 if true, else 0.0.
  symbol_table syms;
  auto e = parse_scalar("x < y", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 1}, {"y", 2}}), 1.0);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 5}, {"y", 2}}), 0.0);
  // Equal evaluates as not-less-than.
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 2}, {"y", 2}}), 0.0);
}

TEST(ParserGrammar, ComparisonLeIncludesEquality) {
  symbol_table syms;
  auto e = parse_scalar("x <= y", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 2}, {"y", 2}}), 1.0);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 3}, {"y", 2}}), 0.0);
}

TEST(ParserGrammar, ComparisonGtAndGe) {
  symbol_table syms;
  auto egt = parse_scalar("a > b", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(egt, syms, {{"a", 5}, {"b", 2}}), 1.0);
  EXPECT_DOUBLE_EQ(eval_scalar(egt, syms, {{"a", 2}, {"b", 5}}), 0.0);
  auto ege = parse_scalar("a >= b", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(ege, syms, {{"a", 2}, {"b", 2}}), 1.0);
}

TEST(ParserGrammar, ComparisonEqAndNe) {
  symbol_table syms;
  auto eq_e = parse_scalar("a == b", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(eq_e, syms, {{"a", 3}, {"b", 3}}), 1.0);
  EXPECT_DOUBLE_EQ(eval_scalar(eq_e, syms, {{"a", 3}, {"b", 4}}), 0.0);
  auto ne_e = parse_scalar("a != b", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(ne_e, syms, {{"a", 3}, {"b", 3}}), 0.0);
  EXPECT_DOUBLE_EQ(eval_scalar(ne_e, syms, {{"a", 3}, {"b", 4}}), 1.0);
}

TEST(ParserGrammar, ComparisonBindsLooserThanArithmetic) {
  // x + 1 < y * 2 must parse as (x+1) < (y*2), not as x + (1 < y) * 2.
  symbol_table syms;
  auto e = parse_scalar("x + 1 < y * 2", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 2}, {"y", 3}}), 1.0); // 3 < 6
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 9}, {"y", 3}}), 0.0); // 10 < 6
}

TEST(ParserGrammar, EqualityBindsLooserThanComparison) {
  // a < b == c parses as (a < b) == c, not a < (b == c).
  // With a=1, b=2, c=1: (1 < 2)=1, 1 == 1 → 1.
  // With a=3, b=2, c=1: (3 < 2)=0, 0 == 1 → 0.
  symbol_table syms;
  auto e = parse_scalar("a < b == c", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"a", 1}, {"b", 2}, {"c", 1}}), 1.0);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"a", 3}, {"b", 2}, {"c", 1}}), 0.0);
}

// ─── Phase 2c: function calls ──────────────────────────────────────

TEST(ParserGrammar, UnaryFunctionSin) {
  symbol_table syms;
  EXPECT_DOUBLE_EQ(eval_scalar(parse_scalar("sin(0)", syms), syms), 0.0);
}

TEST(ParserGrammar, UnaryFunctionCos) {
  symbol_table syms;
  EXPECT_DOUBLE_EQ(eval_scalar(parse_scalar("cos(0)", syms), syms), 1.0);
}

TEST(ParserGrammar, UnaryFunctionExp) {
  symbol_table syms;
  EXPECT_NEAR(eval_scalar(parse_scalar("exp(1)", syms), syms), std::exp(1.0),
              1e-12);
}

TEST(ParserGrammar, UnaryFunctionLog) {
  symbol_table syms;
  EXPECT_NEAR(eval_scalar(parse_scalar("log(exp(2))", syms), syms), 2.0, 1e-12);
}

TEST(ParserGrammar, UnaryFunctionSqrt) {
  symbol_table syms;
  EXPECT_DOUBLE_EQ(eval_scalar(parse_scalar("sqrt(9)", syms), syms), 3.0);
}

TEST(ParserGrammar, UnaryFunctionAbs) {
  symbol_table syms;
  EXPECT_DOUBLE_EQ(
      eval_scalar(parse_scalar("abs(x)", syms), syms, {{"x", -5.0}}), 5.0);
}

TEST(ParserGrammar, UnaryFunctionSinOnIdentifier) {
  symbol_table syms;
  auto e = parse_scalar("sin(x)", syms);
  EXPECT_NEAR(eval_scalar(e, syms, {{"x", 1.5}}), std::sin(1.5), 1e-12);
}

TEST(ParserGrammar, HyperbolicTanh) {
  symbol_table syms;
  EXPECT_DOUBLE_EQ(eval_scalar(parse_scalar("tanh(0)", syms), syms), 0.0);
}

TEST(ParserGrammar, BinaryFunctionPow) {
  symbol_table syms;
  EXPECT_DOUBLE_EQ(eval_scalar(parse_scalar("pow(2, 8)", syms), syms), 256.0);
}

TEST(ParserGrammar, BinaryFunctionLt) {
  symbol_table syms;
  auto e = parse_scalar("lt(x, y)", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 1}, {"y", 2}}), 1.0);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 5}, {"y", 2}}), 0.0);
}

// ─── β-2a additions: piecewise + constitutive primitives ──────────

TEST(ParserGrammar, BinaryFunctionMax) {
  symbol_table syms;
  auto e = parse_scalar("max(x, y)", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 2}, {"y", 5}}), 5.0);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 7}, {"y", 5}}), 7.0);
}

TEST(ParserGrammar, BinaryFunctionMin) {
  symbol_table syms;
  auto e = parse_scalar("min(x, y)", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 2}, {"y", 5}}), 2.0);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 7}, {"y", 5}}), 5.0);
}

TEST(ParserGrammar, TernaryFunctionIfThenElse) {
  // Condition non-zero → 'then' branch. Uses symbolic args so the
  // construction-time folds in scalar_std.h (cond==scalar_zero,
  // cond==scalar_one, numeric-constant cond) cannot fire; the
  // dispatch genuinely flows through the registered entry and the
  // make_expression<scalar_if_then_else> call.
  symbol_table syms;
  auto e = parse_scalar("if_then_else(c, a, b)", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"c", 1}, {"a", 10}, {"b", 20}}),
                   10.0);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"c", 0}, {"a", 10}, {"b", 20}}),
                   20.0);
  // Structural lock-in (qa review H1): parse must produce the same
  // node as a hand-built if_then_else with identical sub-expressions.
  // If the registration were removed, the parser would throw
  // unknown_function_error; if it were rewired to a different factory,
  // the hash would diverge.
  auto c = syms.get_or_declare_scalar("c");
  auto a = syms.get_or_declare_scalar("a");
  auto b = syms.get_or_declare_scalar("b");
  auto expected = if_then_else(c, a, b);
  EXPECT_EQ(e.get().hash_value(), expected.get().hash_value());
}

TEST(ParserGrammar, TernaryFunctionIfThenElseArityErrors) {
  // qa review H3: the first 3-arg registry entry — confirm the
  // dispatch enforces arity. Out-of-bounds access in scalar_ternary
  // (reading a[2] from a length-2 arg_vec) would be UB and only an
  // explicit arity check catches it cleanly.
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse_scalar("if_then_else(x, y)", syms); },
      arity_error);
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse_scalar("if_then_else(x)", syms); },
      arity_error);
  EXPECT_THROW(
      {
        [[maybe_unused]] auto e =
            parse_scalar("if_then_else(x, y, z, w)", syms);
      },
      arity_error);
}

TEST(ParserGrammar, IfThenElseTensorBranchesRaiseTypeMismatch) {
  // Code-reviewer MED-2 (pass-1): documents the partial-coverage
  // decision in function_registry.h — the (scalar cond, tensor then,
  // tensor else) overload from tensor_std.h is unreachable from the
  // parser today, and any future change that "fixes" this without
  // updating the comment should fail here.
  //
  // TRIPWIRE (pass-8 M-1): this is a deferred-resolution lock-in,
  // not a permanent contract. When the remaining 2 of 7 #229
  // checkboxes (t2s / tensor `if_then_else` registry bindings) land,
  // DELETE OR FLIP THIS TEST — at that point parsing tensor branches
  // should succeed, not throw.
  symbol_table syms;
  EXPECT_THROW(
      {
        [[maybe_unused]] auto e =
            parse("if_then_else(x, A{rank=2, dim=3}, B{rank=2, dim=3})", syms);
      },
      type_mismatch_error);
}

TEST(ParserGrammar, UnaryFunctionMacauleyPlus) {
  // <e>+ = max(e, 0): clips negatives to 0.
  symbol_table syms;
  auto e = parse_scalar("macauley_plus(x)", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 3.5}}), 3.5);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", -2.0}}), 0.0);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 0.0}}), 0.0);
}

TEST(ParserGrammar, UnaryFunctionMacauleyMinus) {
  // <e>- = -min(e, 0): magnitude of the negative part, 0 on
  // non-negatives. qa review M2: include the x=0 boundary (parallel
  // to macauley_plus above).
  symbol_table syms;
  auto e = parse_scalar("macauley_minus(x)", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 3.5}}), 0.0);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", -2.0}}), 2.0);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 0.0}}), 0.0);
}

TEST(ParserGrammar, UnaryFunctionHeaviside) {
  // H(e) = ge(e, 0): right-continuous step, H(0) = 1. The argument is
  // a symbol (not a literal) so the runtime evaluator path is tested;
  // a literal 0 would constant-fold in ge() before the evaluator runs.
  symbol_table syms;
  auto e = parse_scalar("heaviside(x)", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 1.0}}), 1.0);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 0.0}}), 1.0);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", -1.0}}), 0.0);
}

TEST(ParserGrammar, BinaryFunctionSmoothedMacauley) {
  // (e + sqrt(e² + ε²)) / 2 — for ε > 0, smooths max(e, 0).
  symbol_table syms;
  auto e = parse_scalar("smoothed_macauley(x, eps)", syms);
  // At ε = 0.1, x = 1: (1 + sqrt(1 + 0.01)) / 2.
  const double expected = (1.0 + std::sqrt(1.0 + 0.01)) / 2.0;
  EXPECT_NEAR(eval_scalar(e, syms, {{"x", 1.0}, {"eps", 0.1}}), expected,
              1e-12);
}

TEST(ParserGrammar, MacauleyPlusLowersToMax) {
  // Pass-3 review (architect F4 + qa F1): macauley_plus has no
  // dedicated AST node — it constructs `max(x, 0)` at parse time.
  // Lock the lowering with a hash check so β-2d's round-trip story
  // has a documented baseline: SEMANTIC round-trip (hash) holds;
  // SYNTACTIC round-trip (string) does not (printer emits
  // `max(x, 0)`, not `macauley_plus(x)`).
  // Pass-4 hardening (qa): also assert the node type so a
  // hypothetical hash-coincidental rewrite to a different node would
  // be caught (paranoid but cheap — the assertion documents intent).
  symbol_table syms_named;
  auto from_name = parse_scalar("macauley_plus(x)", syms_named);
  symbol_table syms_lowered;
  auto from_lowered = parse_scalar("max(x, 0)", syms_lowered);
  EXPECT_EQ(from_name.get().hash_value(), from_lowered.get().hash_value());
  EXPECT_TRUE(is_same<scalar_max>(from_name));
}

TEST(ParserGrammar, MacauleyMinusLowersToNegMin) {
  // macauley_minus(x) constructs `-min(x, 0)`.
  symbol_table syms_named;
  auto from_name = parse_scalar("macauley_minus(x)", syms_named);
  symbol_table syms_lowered;
  auto from_lowered = parse_scalar("-min(x, 0)", syms_lowered);
  EXPECT_EQ(from_name.get().hash_value(), from_lowered.get().hash_value());
  EXPECT_TRUE(is_same<scalar_negative>(from_name));
}

TEST(ParserGrammar, HeavisideLowersToGe) {
  // heaviside(x) constructs `ge(x, 0)` (right-continuous step).
  symbol_table syms_named;
  auto from_name = parse_scalar("heaviside(x)", syms_named);
  symbol_table syms_lowered;
  auto from_lowered = parse_scalar("ge(x, 0)", syms_lowered);
  EXPECT_EQ(from_name.get().hash_value(), from_lowered.get().hash_value());
  EXPECT_TRUE(is_same<scalar_ge>(from_name));
}

TEST(ParserGrammar, SmoothedMacauleyLowersToArithmetic) {
  // smoothed_macauley(x, eps) constructs `(x + sqrt(x² + eps²)) / 2`.
  // Division composes as `a/b → a * pow(b, -1)` per the project's
  // div-via-mul-pow contract, so the top-level node is scalar_mul.
  // Pass-5 hardening: symmetric with the other *LowersTo* tests
  // (pin the expected top-level type alongside the hash equality).
  symbol_table syms_named;
  auto from_name = parse_scalar("smoothed_macauley(x, eps)", syms_named);
  symbol_table syms_lowered;
  auto from_lowered =
      parse_scalar("(x + sqrt(pow(x, 2) + pow(eps, 2))) / 2", syms_lowered);
  EXPECT_EQ(from_name.get().hash_value(), from_lowered.get().hash_value());
  EXPECT_TRUE(is_same<scalar_mul>(from_name));
}

TEST(ParserGrammar, BinaryFunctionSmoothedMacauleyZeroEpsCollapses) {
  // qa review M1: ε → 0 limit recovers the non-smooth Macauley
  // bracket (scalar_std.h:756). Pass-6 verified the literal `0`
  // routes through `make_scalar_constant(int64_t{0})` and returns
  // the `scalar_zero` singleton — so the construction-time fold
  // `if (is_same<scalar_zero>(eps)) return macauley_plus(e)` always
  // fires here and the expression IS `macauley_plus(x)` = `max(x, 0)`.
  // Pass-7 hardening: also assert the node type. If the fold
  // silently regresses to the numerical (e + sqrt(e²+0))/2 path,
  // evaluation would still produce the same numbers but the test
  // would no longer pin the fold. Symmetric with the *LowersTo*
  // tests' node-type pins.
  symbol_table syms;
  auto e = parse_scalar("smoothed_macauley(x, 0)", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 3.0}}), 3.0);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", -3.0}}), 0.0);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 0.0}}), 0.0);
  EXPECT_TRUE(is_same<scalar_max>(e));
}

TEST(ParserGrammar, FunctionCallNestedInArithmetic) {
  // sin(x)^2 + cos(x)^2 == 1 for any x — the classic identity.
  symbol_table syms;
  auto e = parse_scalar("sin(x) ^ 2 + cos(x) ^ 2", syms);
  EXPECT_NEAR(eval_scalar(e, syms, {{"x", 0.7}}), 1.0, 1e-12);
  EXPECT_NEAR(eval_scalar(e, syms, {{"x", -2.3}}), 1.0, 1e-12);
}

TEST(ParserGrammar, FunctionCallNestedInsideFunctionCall) {
  // exp(sin(0)) == exp(0) == 1.
  symbol_table syms;
  auto e = parse_scalar("exp(sin(x))", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 0.0}}), 1.0);
}

TEST(ParserGrammar, FunctionCallWithComplexArguments) {
  // pow(x + 1, 2 * y) — args are full expressions.
  symbol_table syms;
  auto e = parse_scalar("pow(x + 1, 2 * y)", syms);
  // (3 + 1) ^ (2 * 1.5) = 4 ^ 3 = 64.
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"x", 3}, {"y", 1.5}}), 64.0);
}

TEST(ParserGrammar, UnknownFunctionThrows) {
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse_scalar("nope(x)", syms); },
      unknown_function_error);
}

TEST(ParserGrammar, ArityMismatchThrows) {
  symbol_table syms;
  // sin is unary. Two args is an arity error.
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse_scalar("sin(x, y)", syms); },
      arity_error);
  // pow is binary. One arg is an arity error.
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse_scalar("pow(2)", syms); }, arity_error);
  // pow with zero args.
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse_scalar("pow()", syms); }, arity_error);
}

TEST(ParserGrammar, IdentifierWithoutParensIsScalarNotFunction) {
  // `sin` without parens is a bare identifier (scalar variable).
  // The grammar must NOT consume "sin" as a function-call prefix
  // and leave the parse in a bad state.
  symbol_table syms;
  auto e = parse_scalar("sin + 3", syms);
  EXPECT_TRUE(syms.has("sin"))
      << "'sin' without '(' should be declared as a scalar variable.";
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"sin", 4.0}}), 7.0);
}

TEST(ParserGrammar, FunctionCallMissingCloseParenThrows) {
  // Once `(` is consumed, the if_must<> in the grammar makes a
  // missing `)` a hard error rather than silent backtracking.
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse_scalar("sin(x", syms); }, parse_error);
}

// ─── Phase 2d: tensor declarations, tensor & t2s functions ────────

TEST(ParserGrammar, TensorDeclarationRegistersTensor) {
  symbol_table syms;
  auto e = parse_tensor("A{rank=2, dim=3}", syms);
  EXPECT_TRUE(e.is_valid());
  EXPECT_EQ(e.get().rank(), 2u);
  EXPECT_EQ(e.get().dim(), 3u);
  auto shape = syms.tensor_shape("A");
  ASSERT_TRUE(shape.has_value());
  EXPECT_EQ(shape->first, 2u);
  EXPECT_EQ(shape->second, 3u);
}

TEST(ParserGrammar, TensorDeclarationKvOrderIndependent) {
  symbol_table syms;
  auto e1 = parse_tensor("A{rank=4, dim=3}", syms);
  symbol_table syms2;
  auto e2 = parse_tensor("A{dim=3, rank=4}", syms2);
  EXPECT_EQ(e1.get().rank(), 4u);
  EXPECT_EQ(e1.get().dim(), 3u);
  EXPECT_EQ(e2.get().rank(), 4u);
  EXPECT_EQ(e2.get().dim(), 3u);
}

TEST(ParserGrammar, BareIdentAfterTensorDeclResolvesToTensor) {
  symbol_table syms;
  [[maybe_unused]] auto decl = parse_tensor("A{rank=2, dim=3}", syms);
  auto result = parse("A", syms);
  EXPECT_TRUE(
      std::holds_alternative<expression_holder<tensor_expression>>(result))
      << "Bare ident after tensor decl should resolve to the existing tensor.";
}

TEST(ParserGrammar, TensorDeclWithoutBothKvsThrows) {
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse_tensor("A{rank=2}", syms); },
      parse_error);
}

TEST(ParserGrammar, TensorRedeclarationMismatchThrows) {
  symbol_table syms;
  [[maybe_unused]] auto decl = parse_tensor("A{rank=2, dim=3}", syms);
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse_tensor("A{rank=4, dim=3}", syms); },
      redeclaration_error);
}

TEST(ParserGrammar, TraceReturnsT2s) {
  symbol_table syms;
  auto result = parse("trace(A{rank=2, dim=3})", syms);
  EXPECT_TRUE(
      std::holds_alternative<expression_holder<tensor_to_scalar_expression>>(
          result))
      << "trace(tensor) should land in the tensor_to_scalar variant.";
}

TEST(ParserGrammar, TransReturnsTensor) {
  symbol_table syms;
  auto result = parse("trans(A{rank=2, dim=3})", syms);
  EXPECT_TRUE(
      std::holds_alternative<expression_holder<tensor_expression>>(result))
      << "trans(tensor) should land in the tensor variant.";
}

TEST(ParserGrammar, InvOfTensorReturnsTensor) {
  symbol_table syms;
  auto result = parse_tensor("inv(A{rank=2, dim=3})", syms);
  EXPECT_TRUE(result.is_valid());
}

// ─── β-2a additions: rank-2 projectors + outer product ────────────

TEST(ParserGrammar, SymReturnsTensor) {
  symbol_table syms;
  auto result = parse_tensor("sym(A{rank=2, dim=3})", syms);
  EXPECT_TRUE(result.is_valid());
}

TEST(ParserGrammar, SkewReturnsTensor) {
  symbol_table syms;
  auto result = parse_tensor("skew(A{rank=2, dim=3})", syms);
  EXPECT_TRUE(result.is_valid());
}

TEST(ParserGrammar, DevReturnsTensor) {
  symbol_table syms;
  auto result = parse_tensor("dev(A{rank=2, dim=3})", syms);
  EXPECT_TRUE(result.is_valid());
}

TEST(ParserGrammar, VolReturnsTensor) {
  symbol_table syms;
  auto result = parse_tensor("vol(A{rank=2, dim=3})", syms);
  EXPECT_TRUE(result.is_valid());
}

TEST(ParserGrammar, SymOfScalarThrowsTypeMismatch) {
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse("sym(x)", syms); },
      type_mismatch_error);
}

TEST(ParserGrammar, DevOfScalarThrowsTypeMismatch) {
  // qa review M4: arg-kind checks are per-entry; mistyping one of the
  // projection helpers would only be caught by its own test.
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse("dev(x)", syms); },
      type_mismatch_error);
}

TEST(ParserGrammar, VolOfScalarThrowsTypeMismatch) {
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse("vol(x)", syms); },
      type_mismatch_error);
}

TEST(ParserGrammar, SkewOfScalarThrowsTypeMismatch) {
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse("skew(x)", syms); },
      type_mismatch_error);
}

TEST(ParserGrammar, OuterProductReturnsTensor) {
  // otimes(A_ij, B_kl) is a rank-4 tensor; parser names it `otimes`
  // or `outer_product` (alias). Both bind to the 2-arg form.
  symbol_table syms;
  auto e1 = parse_tensor("otimes(A{rank=2, dim=3}, B{rank=2, dim=3})", syms);
  EXPECT_TRUE(e1.is_valid());
  EXPECT_EQ(e1.get().rank(), 4u);
  symbol_table syms2;
  auto e2 =
      parse_tensor("outer_product(A{rank=2, dim=3}, B{rank=2, dim=3})", syms2);
  EXPECT_TRUE(e2.is_valid());
  EXPECT_EQ(e2.get().rank(), 4u);
}

TEST(ParserGrammar, OuterProductAliasProducesIdenticalExpression) {
  // qa review H2: `otimes` and `outer_product` are two registry
  // entries each holding their own dispatch lambda. Lock the alias
  // contract: identical source expression → identical hash. A future
  // editor that "optimizes" one path without touching the other
  // would fail here.
  symbol_table syms1;
  auto e1 = parse_tensor("otimes(A{rank=2, dim=3}, B{rank=2, dim=3})", syms1);
  symbol_table syms2;
  auto e2 =
      parse_tensor("outer_product(A{rank=2, dim=3}, B{rank=2, dim=3})", syms2);
  EXPECT_EQ(e1.get().hash_value(), e2.get().hash_value());
}

TEST(ParserGrammar, OuterProductOfScalarThrowsTypeMismatch) {
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse("otimes(x, y)", syms); },
      type_mismatch_error);
}

// ─── β-2c: tensor constants (function-form) + permute_indices ─────

TEST(ParserGrammar, ZeroTensorFactoryBuildsTensorZero) {
  // zero_tensor(dim=3, rank=2) → tensor_zero(dim=3, rank=2).
  // Locks the dispatch with hash equality against a hand-built
  // tensor_zero, mirroring the *LowersTo* pattern. Hash includes
  // dim+rank in tensor_zero, so a wrong arg order would fail.
  symbol_table syms;
  auto from_name = parse_tensor("zero_tensor(3, 2)", syms);
  auto expected = expression_holder<tensor_expression>{
      std::make_shared<tensor_zero>(std::size_t{3}, std::size_t{2})};
  EXPECT_EQ(from_name.get().hash_value(), expected.get().hash_value());
  EXPECT_EQ(from_name.get().rank(), 2u);
  EXPECT_EQ(from_name.get().dim(), 3u);
}

TEST(ParserGrammar, IdentityTensorFactoryBuildsIdentity) {
  symbol_table syms;
  auto from_name = parse_tensor("identity_tensor(3, 2)", syms);
  auto expected = expression_holder<tensor_expression>{
      std::make_shared<identity_tensor>(std::size_t{3}, std::size_t{2})};
  EXPECT_EQ(from_name.get().hash_value(), expected.get().hash_value());
  EXPECT_EQ(from_name.get().rank(), 2u);
  EXPECT_EQ(from_name.get().dim(), 3u);
}

TEST(ParserGrammar, EpsFactoryBuildsLeviCivita) {
  // eps(3) → levi_civita_tensor(dim=3). Rank == dim for LC.
  symbol_table syms;
  auto from_name = parse_tensor("eps(3)", syms);
  auto expected = numsim::cas::levi_civita(std::size_t{3});
  EXPECT_EQ(from_name.get().hash_value(), expected.get().hash_value());
  EXPECT_EQ(from_name.get().rank(), 3u);
  EXPECT_EQ(from_name.get().dim(), 3u);
}

TEST(ParserGrammar, ZeroTensorRejectsNonPositiveDim) {
  // The to_positive_size_t helper rejects 0 and negatives; the parser
  // surfaces this as type_mismatch_error (the same error class used
  // for arg-kind mismatches).
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse("zero_tensor(0, 2)", syms); },
      type_mismatch_error);
}

TEST(ParserGrammar, ZeroTensorRejectsNonIntegerLiteral) {
  // Decimal literal — scalar_number variant alternative is double,
  // not int64_t; the helper rejects it.
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse("zero_tensor(3.5, 2)", syms); },
      type_mismatch_error);
}

TEST(ParserGrammar, ZeroTensorRejectsNonConstantExpression) {
  // Symbol — not a scalar_constant at all. Worth pinning because
  // a future user might write `zero_tensor(n, m)` and the helper
  // must reject it cleanly rather than crash.
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse("zero_tensor(n, 2)", syms); },
      type_mismatch_error);
}

TEST(ParserGrammar, EpsRejectsNonPositiveDim) {
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse("eps(0)", syms); },
      type_mismatch_error);
}

TEST(ParserGrammar, PermuteIndicesFactoryBuildsPermutation) {
  // permute_indices(A{rank=2, dim=3}, [2, 1]) is the trans form.
  // Validates the dispatch fires and the index_list grammar (added
  // in β-2a's contraction work) accepts a 2-arg shape just as it
  // already accepted the 4-arg inner_product / dot_product shapes.
  symbol_table syms;
  auto result = parse_tensor("permute_indices(A{rank=2, dim=3}, [2, 1])", syms);
  EXPECT_TRUE(result.is_valid());
  EXPECT_EQ(result.get().rank(), 2u);
  EXPECT_EQ(result.get().dim(), 3u);
}

TEST(ParserGrammar, PermuteIndicesRankMismatchThrows) {
  // 3-element index list against a rank-2 tensor — caught by
  // permute_indices() itself (tensor_functions.h:309-312) as
  // invalid_expression_error.
  symbol_table syms;
  EXPECT_THROW(
      {
        [[maybe_unused]] auto e =
            parse("permute_indices(A{rank=2, dim=3}, [2, 1, 3])", syms);
      },
      invalid_expression_error);
}

TEST(ParserGrammar, PermuteIndicesOfScalarThrowsTypeMismatch) {
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse("permute_indices(x, [1])", syms); },
      type_mismatch_error);
}

TEST(ParserGrammar, TraceOfScalarThrowsTypeMismatch) {
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse("trace(x)", syms); },
      type_mismatch_error);
}

TEST(ParserGrammar, SinOfTensorThrowsTypeMismatch) {
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse("sin(A{rank=2, dim=3})", syms); },
      type_mismatch_error);
}

TEST(ParserGrammar, ScalarTimesTensorReturnsTensor) {
  symbol_table syms;
  auto result = parse("2 * A{rank=2, dim=3}", syms);
  EXPECT_TRUE(
      std::holds_alternative<expression_holder<tensor_expression>>(result));
}

TEST(ParserGrammar, TensorTimesScalarReturnsTensor) {
  symbol_table syms;
  auto result = parse("A{rank=2, dim=3} * 2", syms);
  EXPECT_TRUE(
      std::holds_alternative<expression_holder<tensor_expression>>(result));
}

TEST(ParserGrammar, TensorPlusScalarThrowsTypeMismatch) {
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse("A{rank=2, dim=3} + 2", syms); },
      type_mismatch_error);
}

TEST(ParserGrammar, ScalarDivTensorThrowsTypeMismatch) {
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse("2 / A{rank=2, dim=3}", syms); },
      type_mismatch_error);
}

TEST(ParserGrammar, TraceCompositionAcrossTwoTensors) {
  symbol_table syms;
  auto result =
      parse("trace(A{rank=2, dim=3}) + trace(B{rank=2, dim=3})", syms);
  EXPECT_TRUE(
      std::holds_alternative<expression_holder<tensor_to_scalar_expression>>(
          result));
}

TEST(ParserGrammar, ScalarTimesTraceReturnsT2s) {
  symbol_table syms;
  auto result = parse("2 * trace(A{rank=2, dim=3})", syms);
  EXPECT_TRUE(
      std::holds_alternative<expression_holder<tensor_to_scalar_expression>>(
          result));
}

// ─── Phase 2d review-pass additions ────────────────────────────────

TEST(ParserGrammar, TensorDeclDuplicateRankThrows) {
  // Review finding (bug 1): the kv_list grammar accepts repeated
  // kvs, and without an explicit guard the second one silently
  // overwrites the first. Pin the explicit-throw contract.
  symbol_table syms;
  EXPECT_THROW(
      {
        [[maybe_unused]] auto e =
            parse_tensor("A{rank=2, rank=4, dim=3}", syms);
      },
      syntax_error);
}

TEST(ParserGrammar, TensorDeclDuplicateDimThrows) {
  symbol_table syms;
  EXPECT_THROW(
      {
        [[maybe_unused]] auto e = parse_tensor("A{rank=2, dim=3, dim=4}", syms);
      },
      syntax_error);
}

TEST(ParserGrammar, TensorDeclZeroRankThrows) {
  // Review finding (bug 2): the tensor constructor takes rank/dim
  // verbatim; degenerate values explode at evaluation. Validate at
  // the parser level instead.
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse_tensor("A{rank=0, dim=3}", syms); },
      syntax_error);
}

TEST(ParserGrammar, TensorDeclZeroDimThrows) {
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse_tensor("A{rank=2, dim=0}", syms); },
      syntax_error);
}

TEST(ParserGrammar, ChainedComparisonIsCStyleNotMathAnd) {
  // Review finding (coverage gap 3): comparison ops left-associate
  // at the comparison precedence level. `a < b < c` parses as
  // `(a < b) < c`, NOT `(a < b) AND (b < c)`. The result of `a < b`
  // is a 0/1 indicator; that's then compared against c.
  //
  // With a=0, b=1, c=2: (0 < 1) = 1, 1 < 2 = 1.
  // With a=0, b=1, c=1: (0 < 1) = 1, 1 < 1 = 0.
  // With a=1, b=0, c=2: (1 < 0) = 0, 0 < 2 = 1  (NOT math-AND, which
  //                                              would be 1<0=F → 0).
  // The third case is the clearest behavioural divergence from a
  // math-user's chain interpretation; lock it in.
  symbol_table syms;
  auto e = parse_scalar("a < b < c", syms);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"a", 0}, {"b", 1}, {"c", 2}}), 1.0);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"a", 0}, {"b", 1}, {"c", 1}}), 0.0);
  EXPECT_DOUBLE_EQ(eval_scalar(e, syms, {{"a", 1}, {"b", 0}, {"c", 2}}), 1.0)
      << "Chained-comparison should be C-style (value-of-comparison "
         "then compare), NOT math-style AND.";
}

TEST(ParserGrammar, T2sPlusScalarThrowsTypeMismatch) {
  // Review finding (coverage gap 4): parallel to TensorPlusScalar.
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse("trace(A{rank=2, dim=3}) + 2", syms); },
      type_mismatch_error);
}

TEST(ParserGrammar, EmptyInputThrows) {
  // Review finding (coverage gap 6): empty source must throw, not
  // return some null variant.
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse_scalar("", syms); }, parse_error);
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse_scalar("   ", syms); }, parse_error);
}

TEST(ParserGrammar, BareOpenParenThrows) {
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse_scalar("(", syms); }, parse_error);
}

TEST(ParserGrammar, TensorFunctionOnScalarMentionsExpectedType) {
  // Review finding (coverage gap 5): error message should be useful.
  // The error needs to convey what failed; assert the call name
  // and arg-position appear in what().
  symbol_table syms;
  try {
    [[maybe_unused]] auto e = parse("trans(x)", syms);
    FAIL() << "Expected type_mismatch_error to be thrown.";
  } catch (type_mismatch_error const &e) {
    std::string what = e.what();
    EXPECT_NE(what.find("trans"), std::string::npos)
        << "Error message should mention the function name. what() was:\n"
        << what;
    EXPECT_NE(what.find("argument"), std::string::npos)
        << "Error message should mention 'argument'. what() was:\n"
        << what;
  }
}

TEST(ParserGrammar, InvOfScalarThrowsTypeMismatch) {
  // Review finding (coverage gap 7): inv expects tensor; parallel to
  // the trace test.
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse("inv(x)", syms); },
      type_mismatch_error);
}

TEST(ParserGrammar, TransOfScalarThrowsTypeMismatch) {
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse("trans(x)", syms); },
      type_mismatch_error);
}

TEST(ParserGrammar, LeadingAndTrailingWhitespaceTensorDecl) {
  // Whitespace handling around a sole tensor_decl.
  symbol_table syms;
  auto e = parse_tensor("   A{rank=2, dim=3}   ", syms);
  EXPECT_EQ(e.get().rank(), 2u);
  EXPECT_EQ(e.get().dim(), 3u);
}

// ─── Phase 2e: bracket-list indices + contraction functions ───────

TEST(ParserGrammar, InnerProductBasicReturnsTensor) {
  // inner_product(A, [3,4], B, [1,2]) — full contraction of the last
  // two indices of A with the first two of B. With A rank-4 and B
  // rank-2, the result is rank-(4+2-2-2)=2 tensor.
  symbol_table syms;
  auto result =
      parse("inner_product(A{rank=4, dim=3}, [3, 4], B{rank=2, dim=3}, [1, 2])",
            syms);
  EXPECT_TRUE(
      std::holds_alternative<expression_holder<tensor_expression>>(result));
}

TEST(ParserGrammar, DotProductReturnsT2s) {
  // dot_product over matching indices yields a scalar (t2s).
  symbol_table syms;
  auto result = parse(
      "dot_product(A{rank=2, dim=3}, [1, 2], B{rank=2, dim=3}, [1, 2])", syms);
  EXPECT_TRUE(
      std::holds_alternative<expression_holder<tensor_to_scalar_expression>>(
          result));
}

TEST(ParserGrammar, BracketListAcceptsSingleIndex) {
  // [1] — a single-index bracket-list. Useful for first/last-axis
  // contractions.
  symbol_table syms;
  auto result =
      parse("dot_product(A{rank=1, dim=3}, [1], B{rank=1, dim=3}, [1])", syms);
  EXPECT_TRUE(
      std::holds_alternative<expression_holder<tensor_to_scalar_expression>>(
          result));
}

TEST(ParserGrammar, BracketListEmptyThrows) {
  // [] not allowed — contraction requires at least one index per side.
  symbol_table syms;
  EXPECT_THROW(
      {
        [[maybe_unused]] auto e = parse(
            "inner_product(A{rank=2, dim=3}, [], B{rank=2, dim=3}, [1, 2])",
            syms);
      },
      parse_error);
}

TEST(ParserGrammar, BracketListZeroIndexThrows) {
  // Indices are 1-based; 0 is invalid.
  symbol_table syms;
  EXPECT_THROW(
      {
        [[maybe_unused]] auto e =
            parse("inner_product(A{rank=2, dim=3}, [0, 1], B{rank=2, dim=3}, "
                  "[1, 2])",
                  syms);
      },
      lexical_error);
}

TEST(ParserGrammar, BracketListAsToplevelExpressionThrows) {
  // A bare bracket-list at the top is not an expression.
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse("[1, 2, 3]", syms); }, parse_error);
}

TEST(ParserGrammar, BracketListPassedToNonContractionThrows) {
  // sin doesn't take an index list — passing one is a type mismatch.
  symbol_table syms;
  EXPECT_THROW(
      { [[maybe_unused]] auto e = parse("sin([1, 2])", syms); },
      type_mismatch_error);
}

TEST(ParserGrammar, InnerProductWithExpressionWhereIndexExpectedThrows) {
  // inner_product expects arg-positions 1 and 3 to be bracket-lists,
  // not expressions. Passing an expression there is a type mismatch.
  symbol_table syms;
  EXPECT_THROW(
      {
        [[maybe_unused]] auto e =
            parse("inner_product(A{rank=2, dim=3}, 1 + 2, B{rank=2, dim=3}, "
                  "[1, 2])",
                  syms);
      },
      type_mismatch_error);
}

// ─── Phase 2e review-pass additions ────────────────────────────────

// Local tensor-data factory — TensorEvaluatorTest.h's `make_test_data`
// lives in an anonymous namespace and we appear alphabetically before
// it in main.cpp's include block, so we can't reuse the symbol.
// Same shape; duplication is two lines.
namespace parser_test_detail {
template <std::size_t Dim, std::size_t Rank>
inline auto make_tensor_data(std::initializer_list<double> values) {
  auto ptr = std::make_shared<numsim::cas::tensor_data<double, Dim, Rank>>();
  auto *raw = ptr->raw_data();
  std::size_t i = 0;
  for (auto v : values) {
    raw[i++] = v;
  }
  return ptr;
}
} // namespace parser_test_detail

TEST(ParserGrammar, DotProductEvaluatorRoundTrip) {
  // Review finding (gap 1): pin the end-to-end value, not just the
  // variant alternative. dot_product over matching indices computes
  // sum_ij A_ij * A_ij for A = [[1, 2], [3, 4]]:
  //   1*1 + 2*2 + 3*3 + 4*4 = 1 + 4 + 9 + 16 = 30.
  //
  // Catches any off-by-one in to_sequence (1-based → 0-based), any
  // lhs/rhs swap in the dispatch lambda, and any index-list flow
  // breakage between the action stack and the function call.
  symbol_table syms;
  auto expr = parse_t2s(
      "dot_product(A{rank=2, dim=2}, [1, 2], A{rank=2, dim=2}, [1, 2])", syms);

  // Recover the parser-declared tensor holder so we can bind data
  // to the SAME holder the parsed expression references.
  auto lookup = syms.get("A");
  ASSERT_TRUE(lookup.has_value());
  auto *A_ptr = std::get_if<expression_holder<tensor_expression>>(&*lookup);
  ASSERT_NE(A_ptr, nullptr);

  numsim::cas::tensor_to_scalar_evaluator<double> ev;
  ev.set(*A_ptr,
         parser_test_detail::make_tensor_data<2, 2>({1.0, 2.0, 3.0, 4.0}));
  EXPECT_NEAR(ev.apply(expr), 30.0, 1e-12);
}

TEST(ParserGrammar, DotProductEvaluatorRoundTripAsymmetricMatrix) {
  // Round-trip with an asymmetric matrix to confirm the index
  // conversion doesn't accidentally transpose. For B = [[1, 2],
  // [3, 4]] paired against C = [[5, 6], [7, 8]]:
  //   dot_product(B, [1, 2], C, [1, 2]) = sum_ij B_ij C_ij
  //   = 1*5 + 2*6 + 3*7 + 4*8 = 5 + 12 + 21 + 32 = 70.
  symbol_table syms;
  auto expr = parse_t2s(
      "dot_product(B{rank=2, dim=2}, [1, 2], C{rank=2, dim=2}, [1, 2])", syms);
  auto b_lookup = syms.get("B");
  auto c_lookup = syms.get("C");
  ASSERT_TRUE(b_lookup.has_value() && c_lookup.has_value());
  auto *B_ptr = std::get_if<expression_holder<tensor_expression>>(&*b_lookup);
  auto *C_ptr = std::get_if<expression_holder<tensor_expression>>(&*c_lookup);
  ASSERT_NE(B_ptr, nullptr);
  ASSERT_NE(C_ptr, nullptr);
  numsim::cas::tensor_to_scalar_evaluator<double> ev;
  ev.set(*B_ptr,
         parser_test_detail::make_tensor_data<2, 2>({1.0, 2.0, 3.0, 4.0}));
  ev.set(*C_ptr,
         parser_test_detail::make_tensor_data<2, 2>({5.0, 6.0, 7.0, 8.0}));
  EXPECT_NEAR(ev.apply(expr), 70.0, 1e-12);
}

TEST(ParserGrammar, BracketListTrailingCommaThrows) {
  // Review finding (gap 2): `[1, 2,]` — trailing comma.
  symbol_table syms;
  EXPECT_THROW(
      {
        [[maybe_unused]] auto e = parse(
            "dot_product(A{rank=2, dim=3}, [1, 2,], B{rank=2, dim=3}, [1, 2])",
            syms);
      },
      parse_error);
}

TEST(ParserGrammar, BracketListMissingCommaThrows) {
  // `[1 2]` — missing comma.
  symbol_table syms;
  EXPECT_THROW(
      {
        [[maybe_unused]] auto e = parse(
            "dot_product(A{rank=2, dim=3}, [1 2], B{rank=2, dim=3}, [1, 2])",
            syms);
      },
      parse_error);
}

TEST(ParserGrammar, BracketListDoubleCommaThrows) {
  // `[1,,2]` — double comma.
  symbol_table syms;
  EXPECT_THROW(
      {
        [[maybe_unused]] auto e = parse(
            "dot_product(A{rank=2, dim=3}, [1,,2], B{rank=2, dim=3}, [1, 2])",
            syms);
      },
      parse_error);
}

// ─── Phase 3a: integration tests ───────────────────────────────────
//
// End-to-end checks that parse-produced ASTs participate correctly in
// the rest of the library: differentiation, evaluation, and
// locale-independent number parsing. Each test parses a non-trivial
// input, hands the result to an existing-library API, and asserts the
// numeric or printed-form outcome. These are the first tests that
// exercise the parser the way real callers will — grammar tests
// confirm the parse succeeds; these confirm the AST is well-formed
// enough for downstream visitors.

TEST(ParserIntegration, PythagoreanDerivativeIsZero) {
  // d/dx (sin(x)^2 + cos(x)^2) ≡ 0 for all x. Catches mistakes in the
  // scalar function-call dispatch (sin/cos registry entries), `^` AST
  // shape, and any construction-time simplification that would corrupt
  // the chain rule downstream.
  symbol_table syms;
  auto e = parse_scalar("sin(x)^2 + cos(x)^2", syms);
  auto x = syms.get_or_declare_scalar("x");
  auto d = numsim::cas::diff(e, x);

  numsim::cas::scalar_evaluator<double> ev;
  for (double val : {0.0, 0.5, 1.0, 1.5, -0.7, 3.14159, 100.0}) {
    ev.set(x, val);
    EXPECT_NEAR(ev.apply(d), 0.0, 1e-12)
        << "Pythagorean derivative should be 0 at x = " << val;
  }
}

TEST(ParserIntegration, TraceGradientEqualsIdentity) {
  // Two assertions woven into one test:
  //
  //  (a) The bare-`A` reference after `A{rank=2, dim=3}` resolves to
  //      the SAME holder that lives inside the trace expression. The
  //      diff result alone can't prove this — `diff(trace(A_x), A_y)`
  //      would yield `identity_tensor` regardless of whether A_x and
  //      A_y are literally the same holder, since the diff visitor
  //      compares by value-equality on named tensors. The EXPECT_EQ
  //      below is what actually pins phase 2d's lookup-first contract.
  //  (b) d(trace(A))/dA = I (rank-2 identity tensor of matching dim).
  //      Pins the (t2s, tensor) diff routing via tag_invoke.
  symbol_table syms;
  auto trace_expr = parse_t2s("trace(A{rank=2, dim=3})", syms);
  auto A_bare = parse_tensor("A", syms);
  auto A_lookup = syms.get("A");
  ASSERT_TRUE(A_lookup.has_value());
  auto *A_in_syms =
      std::get_if<expression_holder<tensor_expression>>(&*A_lookup);
  ASSERT_NE(A_in_syms, nullptr);
  EXPECT_EQ(A_bare, *A_in_syms)
      << "Bare `A` must resolve to the SAME holder as the declared tensor.";

  auto d = diff(trace_expr, A_bare);
  auto I = make_expression<identity_tensor>(std::size_t{3}, std::size_t{2});
  EXPECT_SAME_PRINT(d, I);
}

TEST(ParserIntegration, MultiVariablePolynomialEvaluatesAtSampledPoints) {
  // Parse `a*x^2 + b*x + c`, implicitly declaring four scalars in one
  // go, then evaluate at sample (a, b, c, x). With (1, -3, 2):
  //   polynomial = x^2 - 3x + 2 = (x-1)(x-2), roots at x=1, x=2.
  //
  // Verifies the parser composes literals + named scalars + mixed
  // operators (* with both name-name and name-literal, then `+`)
  // into an AST the scalar_evaluator can walk, AND that the
  // symbol_table threads four implicit scalar declarations through
  // one parse without cross-pollution.
  symbol_table syms;
  auto e = parse_scalar("a*x^2 + b*x + c", syms);
  EXPECT_DOUBLE_EQ(
      eval_scalar(e, syms, {{"a", 1}, {"b", -3}, {"c", 2}, {"x", 1.0}}), 0.0);
  EXPECT_DOUBLE_EQ(
      eval_scalar(e, syms, {{"a", 1}, {"b", -3}, {"c", 2}, {"x", 2.0}}), 0.0);
  EXPECT_DOUBLE_EQ(
      eval_scalar(e, syms, {{"a", 1}, {"b", -3}, {"c", 2}, {"x", 0.0}}), 2.0);
  EXPECT_DOUBLE_EQ(
      eval_scalar(e, syms, {{"a", 1}, {"b", -3}, {"c", 2}, {"x", 4.0}}), 6.0);
  EXPECT_DOUBLE_EQ(
      eval_scalar(e, syms, {{"a", 1}, {"b", -3}, {"c", 2}, {"x", -1.0}}), 6.0);
}

TEST(ParserIntegration, NumberLiteralIsLocaleIndependent) {
  // Number-literal parsing uses std::from_chars, which ignores the
  // global C locale. Set a comma-decimal locale, parse "1.5", and
  // verify the value is 1.5 — proving we don't silently fall through
  // to std::strtod / std::stod (both honor the global locale and
  // would either misparse or throw in de_DE).
  //
  // CI runners may not have de_DE.UTF-8 installed; if no comma-decimal
  // locale is available, skip rather than fail.
  // Try a broad set of comma-decimal locales — most Linux/macOS CI
  // images ship at least one of these, even if `de_DE` is missing.
  // en_DK.utf8 is unusual because it's Danish English (so it's
  // installed on hosts that don't have any continental European
  // locales) but it still uses comma-decimal.
  std::locale saved;
  bool locale_set = false;
  for (auto const *loc_name : {"de_DE.UTF-8", "de_DE.utf8", "de_DE", //
                               "fr_FR.UTF-8", "fr_FR.utf8", "fr_FR", //
                               "it_IT.UTF-8", "it_IT.utf8",          //
                               "es_ES.UTF-8", "es_ES.utf8",          //
                               "nl_NL.UTF-8", "nl_NL.utf8",          //
                               "pt_PT.UTF-8", "pt_PT.utf8",          //
                               "ru_RU.UTF-8", "ru_RU.utf8",          //
                               "en_DK.UTF-8", "en_DK.utf8"}) {
    try {
      saved = std::locale::global(std::locale(loc_name));
      locale_set = true;
      break;
    } catch (std::runtime_error const &) {
      // try the next candidate name
    }
  }
  if (!locale_set) {
    GTEST_SKIP() << "no comma-decimal locale installed; skipping";
  }

  // RAII-restore the locale even if the EXPECT below throws.
  struct LocaleRestorer {
    std::locale saved;
    ~LocaleRestorer() { std::locale::global(saved); }
  } restorer{saved};

  symbol_table syms;
  EXPECT_DOUBLE_EQ(eval_scalar(parse_scalar("1.5", syms), syms), 1.5)
      << "Parser must read '1.5' as 1.5 regardless of the global C "
         "locale's decimal separator.";
}

// ─── Phase 3b: error-coverage tests ────────────────────────────────
//
// Phase 2 confirmed every error path raises *some* `parse_error`.
// These tests tighten that: the right subclass fires, `position()`
// points at the offending byte (verified inside multi-line input for
// at least one path), `what()` mentions a relevant token, and the
// subclass-specific accessors (`name()`, `expected_arity()`, …)
// carry the values the throw site supplied. A regression that
// broadened a throw to the base `parse_error` would still pass the
// existing `EXPECT_THROW(..., parse_error)` checks; these don't.

TEST(ParserErrorCoverage, BracketListZeroIndexFiresLexicalErrorWithPosition) {
  symbol_table syms;
  std::string src =
      "inner_product(A{rank=2, dim=3}, [0, 1], B{rank=2, dim=3}, [1, 2])";
  try {
    [[maybe_unused]] auto e = parse(src, syms);
    FAIL() << "Expected lexical_error.";
  } catch (lexical_error const &e) {
    EXPECT_EQ(e.position(), src.find('0'))
        << "Position should point at the offending '0'.";
    EXPECT_NE(std::string(e.what()).find("1-based"), std::string::npos)
        << "Message should mention the 1-based constraint. what():\n"
        << e.what();
    EXPECT_TRUE(e.has_position());
  }
}

TEST(ParserErrorCoverage, BracketListOverflowFiresLexicalErrorWithMessage) {
  symbol_table syms;
  std::string src =
      "inner_product(A{rank=2, dim=3}, [99999999999999999999, 1], "
      "B{rank=2, dim=3}, [1, 2])";
  try {
    [[maybe_unused]] auto e = parse(src, syms);
    FAIL() << "Expected lexical_error (overflow path).";
  } catch (lexical_error const &e) {
    EXPECT_NE(std::string(e.what()).find("exceeds"), std::string::npos)
        << "Overflow message should differ from the generic 'must be "
           "positive 1-based' message. what():\n"
        << e.what();
  }
}

TEST(ParserErrorCoverage, ZeroRankTensorFiresSyntaxErrorAtDeclaration) {
  symbol_table syms;
  std::string src = "trace(A{rank=0, dim=3})";
  try {
    [[maybe_unused]] auto e = parse(src, syms);
    FAIL() << "Expected syntax_error.";
  } catch (syntax_error const &e) {
    // Action raises with the tensor_decl start position (the `A`).
    EXPECT_EQ(e.position(), src.find('A'));
    EXPECT_NE(std::string(e.what()).find("rank"), std::string::npos);
    EXPECT_NE(std::string(e.what()).find(">= 1"), std::string::npos);
  }
}

TEST(ParserErrorCoverage, ArityErrorCarriesFunctionNameAndCounts) {
  symbol_table syms;
  std::string src = "sin(x, y)";
  try {
    [[maybe_unused]] auto e = parse_scalar(src, syms);
    FAIL() << "Expected arity_error.";
  } catch (arity_error const &e) {
    EXPECT_EQ(e.function(), "sin");
    EXPECT_EQ(e.expected_arity(), 1u);
    EXPECT_EQ(e.actual_arity(), 2u);
    EXPECT_NE(std::string(e.what()).find("sin"), std::string::npos);
    EXPECT_TRUE(e.has_position());
  }
}

TEST(ParserErrorCoverage, UnknownFunctionErrorCarriesName) {
  symbol_table syms;
  std::string src = "no_such_fn(x, y)";
  try {
    [[maybe_unused]] auto e = parse_scalar(src, syms);
    FAIL() << "Expected unknown_function_error.";
  } catch (unknown_function_error const &e) {
    EXPECT_EQ(e.name(), "no_such_fn");
    EXPECT_NE(std::string(e.what()).find("no_such_fn"), std::string::npos);
    EXPECT_TRUE(e.has_position());
  }
}

TEST(ParserErrorCoverage, TypeMismatchMessageMentionsOperand) {
  // `sin(A{...})` — sin's arg_kind is scalar, A is tensor. The
  // function-call action's type-check should reject with the call
  // name and argument index in the message so the user can locate
  // the mistake.
  symbol_table syms;
  std::string src = "sin(A{rank=2, dim=3})";
  try {
    [[maybe_unused]] auto e = parse(src, syms);
    FAIL() << "Expected type_mismatch_error.";
  } catch (type_mismatch_error const &e) {
    EXPECT_NE(std::string(e.what()).find("sin"), std::string::npos);
    EXPECT_NE(std::string(e.what()).find("argument"), std::string::npos);
    EXPECT_TRUE(e.has_position());
  }
}

TEST(ParserErrorCoverage, TypeCollisionScalarThenTensorInSameParse) {
  // `x` is implicitly declared scalar by the first reference. The
  // later `x{rank=2, dim=3}` tries to declare it as a tensor and
  // hits symbol_table's collision check, which the parser re-throws
  // as a type_collision_error with the call site's position.
  symbol_table syms;
  std::string src = "x + x{rank=2, dim=3}";
  try {
    [[maybe_unused]] auto e = parse(src, syms);
    FAIL() << "Expected type_collision_error.";
  } catch (type_collision_error const &e) {
    EXPECT_NE(std::string(e.what()).find("x"), std::string::npos);
    EXPECT_TRUE(e.has_position());
  }
}

TEST(ParserErrorCoverage, RedeclarationDifferentShapeInSameParse) {
  // Two tensor decls in one expression with mismatched (rank, dim).
  symbol_table syms;
  std::string src = "trace(A{rank=2, dim=3}) + dot(A{rank=4, dim=3})";
  try {
    [[maybe_unused]] auto e = parse(src, syms);
    FAIL() << "Expected redeclaration_error.";
  } catch (redeclaration_error const &e) {
    EXPECT_NE(std::string(e.what()).find("A"), std::string::npos);
    EXPECT_TRUE(e.has_position());
  }
}

TEST(ParserErrorCoverage, MultiLineInputPreservesLineAndColumn) {
  // Multi-line input where the error fires on line 3 (the zero-rank
  // tensor). Verifies the byte-offset → line:column translation in
  // parse_error.cpp survives whitespace-padded multi-line input.
  symbol_table syms;
  std::string src = "1 +\n  2 *\n  trace(A{rank=0, dim=3})\n";
  try {
    [[maybe_unused]] auto e = parse(src, syms);
    FAIL() << "Expected syntax_error.";
  } catch (syntax_error const &e) {
    EXPECT_EQ(e.line(), 3u) << "Error should land on line 3. what():\n"
                            << e.what();
    EXPECT_TRUE(e.has_position());
    // Snippet should include line 3's text, not lines 1 or 2.
    std::string what = e.what();
    EXPECT_NE(what.find("trace(A{rank=0, dim=3})"), std::string::npos);
    EXPECT_EQ(what.find("1 +"), std::string::npos)
        << "Line 1 should not appear in the snippet. what():\n"
        << what;
  }
}

TEST(ParserErrorCoverage, PegtlTranslatedErrorPropagatesAsSyntaxError) {
  // Regression test: parser.cpp's `translate_pegtl_error` used to
  // return by value of base type `parse_error`, which sliced the
  // `syntax_error` constructed inside it down to the base before the
  // throw — so PEGTL-originated must-failures (missing close paren,
  // unexpected EOF) arrived at the catch site as base `parse_error`
  // instead of `syntax_error`. Now the function returns by `syntax_error`,
  // preserving the type through the throw. Lock that in:
  //
  // `sin(x` triggers PEGTL's `must<function_call_close>` failure path,
  // which goes through translate_pegtl_error. Must arrive as the
  // narrow subclass.
  symbol_table syms;
  try {
    [[maybe_unused]] auto e = parse_scalar("sin(x", syms);
    FAIL() << "Expected syntax_error.";
  } catch (syntax_error const &) {
    SUCCEED();
  } catch (parse_error const &e) {
    FAIL() << "PEGTL-translated error was sliced to base parse_error. what():\n"
           << e.what();
  }
}

TEST(ParserErrorCoverage, UnknownSymbolErrorIsUnreachableFromParser) {
  // Documentation test: the parser never raises unknown_symbol_error
  // because every bare identifier in scalar position is implicitly
  // declared by `get_or_declare_scalar`. The error class exists for
  // potential future use (e.g. a strict mode that disables implicit
  // declaration) and is exercised at the constructor level by
  // ParseError.UnknownSymbolErrorCarriesName.
  //
  // This test pins the current behavior: a bare reference to a
  // never-declared name in scalar position succeeds and implicitly
  // declares the name as scalar.
  symbol_table syms;
  EXPECT_NO_THROW({
    [[maybe_unused]] auto e = parse_scalar("never_seen_before + 1", syms);
  });
  EXPECT_TRUE(syms.has("never_seen_before"));
}

} // namespace numsim::cas::parser_test

#endif // NUMSIM_CAS_PARSER_ENABLED

#endif // PARSERTEST_H
