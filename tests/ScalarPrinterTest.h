#ifndef SCALARPRINTERTEST_H
#define SCALARPRINTERTEST_H

#include <gtest/gtest.h>
#include <sstream>

#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/scalar/visitors/scalar_printer.h>

namespace numsim::cas {

// ---------------------------------------------------------------------------
// Audit #45 (2026-05-17): scalar_printer overload coverage verified.
// All 20 node types in NUMSIM_CAS_SCALAR_NODE_LIST have explicit operator()
// overrides in scalar_printer. Fallback uses static_assert(sizeof(T) == 0,
// ...) so adding a new node type without an override is a compile error.
// These tests pin the print output for each node type. They check that
// the print output is non-empty and contains the expected token(s) so the
// audit catches a future regression where a print path goes silent.
// ---------------------------------------------------------------------------

namespace {
auto print(expression_holder<scalar_expression> const &expr) {
  std::stringstream ss;
  scalar_printer<std::stringstream> p(ss);
  p.apply(expr);
  return ss.str();
}
} // namespace

TEST(ScalarPrinterAudit, ScalarSymbol) {
  auto x = make_expression<scalar>("x");
  EXPECT_EQ(print(x), "x");
}

TEST(ScalarPrinterAudit, ScalarZero) {
  EXPECT_EQ(print(get_scalar_zero()), "0");
}

TEST(ScalarPrinterAudit, ScalarOne) { EXPECT_EQ(print(get_scalar_one()), "1"); }

TEST(ScalarPrinterAudit, ScalarConstant) {
  auto c = make_expression<scalar_constant>(42.5);
  auto s = print(c);
  // Print should contain the numeric value (allow various representations).
  EXPECT_NE(s.find("42.5"), std::string::npos)
      << "Expected '42.5' in output, got: " << s;
}

TEST(ScalarPrinterAudit, ScalarAdd) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  auto s = print(x + y);
  EXPECT_NE(s.find("+"), std::string::npos)
      << "Expected '+' in add output, got: " << s;
}

TEST(ScalarPrinterAudit, ScalarMul) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  auto s = print(x * y);
  EXPECT_NE(s.find("*"), std::string::npos)
      << "Expected '*' in mul output, got: " << s;
}

TEST(ScalarPrinterAudit, ScalarNegative) {
  auto x = make_expression<scalar>("x");
  auto s = print(-x);
  EXPECT_NE(s.find("-"), std::string::npos)
      << "Expected '-' in negative output, got: " << s;
}

TEST(ScalarPrinterAudit, ScalarNamedExpression) {
  auto x = make_expression<scalar>("x");
  auto f = make_expression<scalar_named_expression>("f", x);
  auto s = print(f);
  EXPECT_NE(s.find("f"), std::string::npos)
      << "Expected 'f' in named expression output, got: " << s;
}

TEST(ScalarPrinterAudit, ScalarSin) {
  auto x = make_expression<scalar>("x");
  auto s = print(sin(x));
  EXPECT_NE(s.find("sin"), std::string::npos) << "got: " << s;
}

TEST(ScalarPrinterAudit, ScalarCos) {
  auto x = make_expression<scalar>("x");
  auto s = print(cos(x));
  EXPECT_NE(s.find("cos"), std::string::npos) << "got: " << s;
}

TEST(ScalarPrinterAudit, ScalarTan) {
  auto x = make_expression<scalar>("x");
  auto s = print(tan(x));
  EXPECT_NE(s.find("tan"), std::string::npos) << "got: " << s;
}

TEST(ScalarPrinterAudit, ScalarAsin) {
  auto x = make_expression<scalar>("x");
  auto s = print(asin(x));
  EXPECT_NE(s.find("asin"), std::string::npos) << "got: " << s;
}

TEST(ScalarPrinterAudit, ScalarAcos) {
  auto x = make_expression<scalar>("x");
  auto s = print(acos(x));
  EXPECT_NE(s.find("acos"), std::string::npos) << "got: " << s;
}

TEST(ScalarPrinterAudit, ScalarAtan) {
  auto x = make_expression<scalar>("x");
  auto s = print(atan(x));
  EXPECT_NE(s.find("atan"), std::string::npos) << "got: " << s;
}

TEST(ScalarPrinterAudit, ScalarPow) {
  auto x = make_expression<scalar>("x");
  auto two = make_expression<scalar_constant>(2.0);
  auto s = print(pow(x, two));
  // pow output may use '^' or 'pow(' — accept either.
  EXPECT_TRUE(s.find("^") != std::string::npos ||
              s.find("pow") != std::string::npos)
      << "Expected '^' or 'pow' in output, got: " << s;
}

TEST(ScalarPrinterAudit, ScalarSqrt) {
  auto x = make_expression<scalar>("x");
  auto s = print(sqrt(x));
  EXPECT_NE(s.find("sqrt"), std::string::npos) << "got: " << s;
}

TEST(ScalarPrinterAudit, ScalarLog) {
  auto x = make_expression<scalar>("x");
  auto s = print(log(x));
  EXPECT_NE(s.find("log"), std::string::npos) << "got: " << s;
}

TEST(ScalarPrinterAudit, ScalarExp) {
  auto x = make_expression<scalar>("x");
  auto s = print(exp(x));
  EXPECT_NE(s.find("exp"), std::string::npos) << "got: " << s;
}

TEST(ScalarPrinterAudit, ScalarSign) {
  auto x = make_expression<scalar>("x");
  auto s = print(sign(x));
  EXPECT_NE(s.find("sign"), std::string::npos) << "got: " << s;
}

TEST(ScalarPrinterAudit, ScalarAbs) {
  auto x = make_expression<scalar>("x");
  // abs of a symbolic input produces a scalar_abs node (positive constants
  // would collapse).
  auto s = print(abs(x));
  EXPECT_NE(s.find("abs"), std::string::npos) << "got: " << s;
}

TEST(ScalarPrinterAudit, ScalarMax) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  // max of symbolic inputs prints as function-call notation.
  auto s = print(max(x, y));
  EXPECT_NE(s.find("max"), std::string::npos) << "got: " << s;
}

TEST(ScalarPrinterAudit, ScalarMin) {
  auto x = make_expression<scalar>("x");
  auto y = make_expression<scalar>("y");
  auto s = print(min(x, y));
  EXPECT_NE(s.find("min"), std::string::npos) << "got: " << s;
}

} // namespace numsim::cas

#endif // SCALARPRINTERTEST_H
