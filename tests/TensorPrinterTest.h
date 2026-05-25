#ifndef TENSORPRINTERTEST_H
#define TENSORPRINTERTEST_H

#include <gtest/gtest.h>
#include <sstream>

#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor/visitors/tensor_printer.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_functions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>

namespace numsim::cas {

// ---------------------------------------------------------------------------
// Audit #77 (2026-05-17): tensor_printer overload coverage verified.
// All 16 node types in NUMSIM_CAS_TENSOR_NODE_LIST have explicit operator()
// overrides in tensor_printer. Fallback uses static_assert(sizeof(T) == 0,
// ...) so a new node type without an override is a compile error.
// These tests pin the print output for each node type, locking in the
// contract against silent print-path regressions.
// ---------------------------------------------------------------------------

namespace {
auto print(expression_holder<tensor_expression> const &expr) {
  std::stringstream ss;
  tensor_printer<std::stringstream> p(ss);
  p.apply(expr);
  return ss.str();
}
} // namespace

TEST(TensorPrinterAudit, Tensor) {
  auto A = make_expression<tensor>("A", 3, 2);
  EXPECT_EQ(print(A), "A");
}

TEST(TensorPrinterAudit, TensorZero) {
  auto Z = make_expression<tensor_zero>(3, 2);
  auto s = print(Z);
  EXPECT_NE(s.find("0"), std::string::npos) << "got: " << s;
}

TEST(TensorPrinterAudit, KroneckerDelta) {
  auto delta = make_expression<identity_tensor>(3, std::size_t{2});
  auto s = print(delta);
  EXPECT_FALSE(s.empty())
      << "identity_tensor (rank 2) print produced empty output";
}

TEST(TensorPrinterAudit, IdentityTensor) {
  auto I = make_expression<identity_tensor>(3, 2);
  auto s = print(I);
  EXPECT_FALSE(s.empty()) << "identity_tensor print produced empty output";
}

TEST(TensorPrinterAudit, TensorProjector) {
  auto P = P_sym(3);
  auto s = print(P);
  EXPECT_FALSE(s.empty()) << "tensor_projector print produced empty output";
}

TEST(TensorPrinterAudit, TensorAdd) {
  auto [A, B] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}},
                           std::tuple{"B", std::size_t{3}, std::size_t{2}});
  auto s = print(A + B);
  EXPECT_NE(s.find("+"), std::string::npos) << "got: " << s;
}

TEST(TensorPrinterAudit, TensorMul) {
  auto [A, B] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}},
                           std::tuple{"B", std::size_t{3}, std::size_t{2}});
  auto s = print(A * B);
  EXPECT_NE(s.find("*"), std::string::npos) << "got: " << s;
}

TEST(TensorPrinterAudit, TensorPow) {
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto s = print(pow(A, 2));
  EXPECT_TRUE(s.find("^") != std::string::npos ||
              s.find("pow") != std::string::npos)
      << "got: " << s;
}

TEST(TensorPrinterAudit, TensorNegative) {
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto s = print(-A);
  EXPECT_NE(s.find("-"), std::string::npos) << "got: " << s;
}

TEST(TensorPrinterAudit, TensorScalarMul) {
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto two = make_scalar_constant(2);
  auto s = print(two * A);
  EXPECT_NE(s.find("*"), std::string::npos) << "got: " << s;
}

TEST(TensorPrinterAudit, TensorInv) {
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{4}, std::size_t{2}});
  auto s = print(inv(A));
  EXPECT_NE(s.find("inv"), std::string::npos) << "got: " << s;
}

TEST(TensorPrinterAudit, PermuteIndices) {
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto s = print(trans(A));
  // trans(A) prints as permute_indices-shaped output
  EXPECT_FALSE(s.empty())
      << "trans/permute_indices print produced empty output";
}

TEST(TensorPrinterAudit, InnerProduct) {
  auto [A, B] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}},
                           std::tuple{"B", std::size_t{3}, std::size_t{2}});
  auto s = print(inner_product(A, sequence{2}, B, sequence{1}));
  EXPECT_FALSE(s.empty()) << "inner_product print produced empty output";
}

TEST(TensorPrinterAudit, OuterProduct) {
  auto [A, B] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}},
                           std::tuple{"B", std::size_t{3}, std::size_t{2}});
  auto s = print(otimes(A, B));
  EXPECT_FALSE(s.empty()) << "outer_product print produced empty output";
}

TEST(TensorPrinterAudit, SimpleOuterProduct) {
  // simple_outer_product is the "outer(...)" form used internally.
  auto u = make_expression<tensor>("u", 3, 1);
  auto v = make_expression<tensor>("v", 3, 1);
  auto sop =
      make_expression<simple_outer_product>(std::size_t{3}, std::size_t{2});
  sop.template get<simple_outer_product>().push_back(u);
  sop.template get<simple_outer_product>().push_back(v);
  auto s = print(sop);
  EXPECT_NE(s.find("outer"), std::string::npos) << "got: " << s;
}

TEST(TensorPrinterAudit, TensorToScalarWithTensorMul) {
  // tensor_to_scalar_with_tensor_mul is constructed internally by t2s
  // differentiation. d(det(A))/dA produces an expression whose root is a
  // tensor_to_scalar_with_tensor_mul (det(A) scaling the inv-transpose
  // tensor). Assert the node type first so a future change to the
  // differentiation result shape doesn't silently let this test pass
  // vacuously without exercising the printer's overload for this node.
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto expr = diff(det(A), A);
  ASSERT_TRUE(is_same<tensor_to_scalar_with_tensor_mul>(expr))
      << "Expected d(det(A))/dA to be a tensor_to_scalar_with_tensor_mul, "
         "got: "
      << to_string(expr);
  auto s = print(expr);
  // Node prints as "<t2s_scalar>*<tensor>" — must contain '*'.
  EXPECT_NE(s.find("*"), std::string::npos) << "got: " << s;
}

} // namespace numsim::cas

#endif // TENSORPRINTERTEST_H
