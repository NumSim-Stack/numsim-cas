#ifndef PREDICATEEXPRESSIONTEST_H
#define PREDICATEEXPRESSIONTEST_H

#include <gtest/gtest.h>
#include <sstream>

#include <numsim_cas/predicate/predicate_definitions.h>
#include <numsim_cas/predicate/visitors/predicate_evaluator.h>
#include <numsim_cas/predicate/visitors/predicate_latex_printer.h>
#include <numsim_cas/predicate/visitors/predicate_printer.h>

namespace numsim::cas {

// ─── Construction and identity ───────────────────────────────────────

TEST(PredicateExpression, TrueAndFalseAreValid) {
  EXPECT_TRUE(get_predicate_true().is_valid());
  EXPECT_TRUE(get_predicate_false().is_valid());
}

TEST(PredicateExpression, GlobalsReturnCachedSingletons) {
  // Repeated calls return the same shared expression — no fresh allocation.
  EXPECT_EQ(&get_predicate_true().get(), &get_predicate_true().get());
  EXPECT_EQ(&get_predicate_false().get(), &get_predicate_false().get());
}

TEST(PredicateExpression, IsSameDistinguishesLiterals) {
  EXPECT_TRUE(is_same<predicate_true>(get_predicate_true()));
  EXPECT_FALSE(is_same<predicate_false>(get_predicate_true()));
  EXPECT_TRUE(is_same<predicate_false>(get_predicate_false()));
  EXPECT_FALSE(is_same<predicate_true>(get_predicate_false()));
}

TEST(PredicateExpression, TrueAndFalseHashesDiffer) {
  // The two literals share no value; their hashes must distinguish them.
  EXPECT_NE(get_predicate_true().get().hash_value(),
            get_predicate_false().get().hash_value());
}

TEST(PredicateExpression, EqualityHoldsForSameLiteral) {
  EXPECT_EQ(get_predicate_true(), get_predicate_true());
  EXPECT_EQ(get_predicate_false(), get_predicate_false());
  EXPECT_NE(get_predicate_true(), get_predicate_false());
}

// ─── Evaluator ───────────────────────────────────────────────────────

TEST(PredicateEvaluator, TrueLiteralEvaluatesToTrue) {
  predicate_evaluator ev;
  EXPECT_TRUE(ev.apply(get_predicate_true()));
}

TEST(PredicateEvaluator, FalseLiteralEvaluatesToFalse) {
  predicate_evaluator ev;
  EXPECT_FALSE(ev.apply(get_predicate_false()));
}

TEST(PredicateEvaluator, InvalidExpressionEvaluatesToFalse) {
  predicate_evaluator ev;
  expression_holder<predicate_expression> null_expr;
  EXPECT_FALSE(ev.apply(null_expr));
}

// ─── Printer ─────────────────────────────────────────────────────────

TEST(PredicatePrinter, TrueLiteralPrintsAsTrue) {
  std::stringstream ss;
  predicate_printer<std::stringstream> p(ss);
  p.apply(get_predicate_true());
  EXPECT_EQ(ss.str(), "true");
}

TEST(PredicatePrinter, FalseLiteralPrintsAsFalse) {
  std::stringstream ss;
  predicate_printer<std::stringstream> p(ss);
  p.apply(get_predicate_false());
  EXPECT_EQ(ss.str(), "false");
}

TEST(PredicatePrinter, InvalidExpressionPrintsNothing) {
  std::stringstream ss;
  predicate_printer<std::stringstream> p(ss);
  expression_holder<predicate_expression> null_expr;
  p.apply(null_expr);
  EXPECT_EQ(ss.str(), "");
}

// ─── LaTeX printer ───────────────────────────────────────────────────

TEST(PredicateLatexPrinter, TrueLiteralPrintsTop) {
  std::stringstream ss;
  predicate_latex_printer<std::stringstream> p(ss);
  p.apply(get_predicate_true());
  EXPECT_EQ(ss.str(), "\\top");
}

TEST(PredicateLatexPrinter, FalseLiteralPrintsBot) {
  std::stringstream ss;
  predicate_latex_printer<std::stringstream> p(ss);
  p.apply(get_predicate_false());
  EXPECT_EQ(ss.str(), "\\bot");
}

} // namespace numsim::cas

#endif // PREDICATEEXPRESSIONTEST_H
