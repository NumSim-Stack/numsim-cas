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

#include <cmath>
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
using numsim::cas::parser::unknown_function_error;
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

} // namespace numsim::cas::parser_test

#endif // NUMSIM_CAS_PARSER_ENABLED

#endif // PARSERTEST_H
