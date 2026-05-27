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
#include <numsim_cas/parser/symbol_table.h>

#include <string>

namespace numsim::cas::parser_test {

// Bring the parser's public types into scope unqualified to keep test
// bodies readable. Restricted to this anonymous-ish translation-unit
// namespace; doesn't leak to other test files.
using numsim::cas::parser::arity_error;
using numsim::cas::parser::parse_error;
using numsim::cas::parser::redeclaration_error;
using numsim::cas::parser::symbol_table;
using numsim::cas::parser::type_collision_error;
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

} // namespace numsim::cas::parser_test

#endif // NUMSIM_CAS_PARSER_ENABLED

#endif // PARSERTEST_H
