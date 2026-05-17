#ifndef TENSORTOSCALARPRINTERTEST_H
#define TENSORTOSCALARPRINTERTEST_H

#include <gtest/gtest.h>
#include <sstream>

#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_functions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_std.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_printer.h>

namespace numsim::cas {

// ---------------------------------------------------------------------------
// Audit #39 (2026-05-17): tensor_to_scalar_printer overload coverage verified.
// All 15 node types in NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST have explicit
// operator() overrides in tensor_to_scalar_printer. Fallback uses
// static_assert(sizeof(T) == 0, ...) so adding a new node type without an
// override is a compile error.
// These tests pin the print output for each node type.
// ---------------------------------------------------------------------------

namespace {
auto print(expression_holder<tensor_to_scalar_expression> const &expr) {
  std::stringstream ss;
  tensor_to_scalar_printer<std::stringstream> p(ss);
  p.apply(expr);
  return ss.str();
}
} // namespace

TEST(TensorToScalarPrinterAudit, Trace) {
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto s = print(trace(A));
  // Trace prints as "tr(A)" — abbreviated form.
  EXPECT_NE(s.find("tr"), std::string::npos) << "got: " << s;
}

TEST(TensorToScalarPrinterAudit, Dot) {
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto s = print(dot(A));
  EXPECT_NE(s.find("dot"), std::string::npos) << "got: " << s;
}

TEST(TensorToScalarPrinterAudit, Det) {
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto s = print(det(A));
  EXPECT_NE(s.find("det"), std::string::npos) << "got: " << s;
}

TEST(TensorToScalarPrinterAudit, Norm) {
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto s = print(norm(A));
  EXPECT_NE(s.find("norm"), std::string::npos) << "got: " << s;
}

TEST(TensorToScalarPrinterAudit, Negative) {
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto s = print(-trace(A));
  EXPECT_NE(s.find("-"), std::string::npos) << "got: " << s;
}

TEST(TensorToScalarPrinterAudit, Add) {
  auto [A, B] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}},
                           std::tuple{"B", std::size_t{3}, std::size_t{2}});
  auto s = print(trace(A) + trace(B));
  EXPECT_NE(s.find("+"), std::string::npos) << "got: " << s;
}

TEST(TensorToScalarPrinterAudit, Mul) {
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto s = print(trace(A) * det(A));
  EXPECT_NE(s.find("*"), std::string::npos) << "got: " << s;
}

TEST(TensorToScalarPrinterAudit, Pow) {
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto s = print(pow(trace(A), 2));
  EXPECT_TRUE(s.find("^") != std::string::npos ||
              s.find("pow") != std::string::npos)
      << "got: " << s;
}

TEST(TensorToScalarPrinterAudit, InnerProductToScalar) {
  auto [A, B] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}},
                           std::tuple{"B", std::size_t{3}, std::size_t{2}});
  auto s = print(dot_product(A, sequence{1, 2}, B, sequence{1, 2}));
  EXPECT_FALSE(s.empty()) << "inner_product_to_scalar print produced empty";
}

TEST(TensorToScalarPrinterAudit, Zero) {
  auto Z = make_expression<tensor_to_scalar_zero>();
  auto s = print(Z);
  EXPECT_NE(s.find("0"), std::string::npos) << "got: " << s;
}

TEST(TensorToScalarPrinterAudit, One) {
  auto O = make_expression<tensor_to_scalar_one>();
  auto s = print(O);
  EXPECT_NE(s.find("1"), std::string::npos) << "got: " << s;
}

TEST(TensorToScalarPrinterAudit, Log) {
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto s = print(log(trace(A)));
  EXPECT_NE(s.find("log"), std::string::npos) << "got: " << s;
}

TEST(TensorToScalarPrinterAudit, Exp) {
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto s = print(exp(trace(A)));
  EXPECT_NE(s.find("exp"), std::string::npos) << "got: " << s;
}

TEST(TensorToScalarPrinterAudit, Sqrt) {
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto s = print(sqrt(trace(A)));
  EXPECT_NE(s.find("sqrt"), std::string::npos) << "got: " << s;
}

TEST(TensorToScalarPrinterAudit, ScalarWrapper) {
  auto wrapped = make_expression<tensor_to_scalar_scalar_wrapper>(
      make_expression<scalar_constant>(42.5));
  auto s = print(wrapped);
  EXPECT_NE(s.find("42.5"), std::string::npos) << "got: " << s;
}

} // namespace numsim::cas

#endif // TENSORTOSCALARPRINTERTEST_H
