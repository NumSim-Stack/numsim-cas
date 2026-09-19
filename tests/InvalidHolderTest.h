#ifndef INVALIDHOLDERTEST_H
#define INVALIDHOLDERTEST_H

#include "cas_test_helpers.h"
#include "numsim_cas/core/contains_expression.h"
#include "numsim_cas/core/substitute.h"
#include "numsim_cas/numsim_cas.h"
#include "numsim_cas/scalar/scalar_latex_io.h"
#include "numsim_cas/scalar/visitors/scalar_limit_visitor.h"
#include "numsim_cas/scalar/visitors/scalar_substitution.h"
#include "numsim_cas/tensor/tensor_latex_io.h"
#include "numsim_cas/tensor/visitors/tensor_substitution.h"
#include "numsim_cas/tensor_to_scalar/tensor_to_scalar_latex_io.h"
#include "numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_limit_visitor.h"
#include "numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_substitution.h"
#include "gtest/gtest.h"

namespace numsim::cas {

// A default-constructed holder must be rejected by every computational entry
// point, in every domain, instead of yielding a plausible result.
struct InvalidHolderFixture : ::testing::Test {
  expression_holder<scalar_expression> s;
  expression_holder<tensor_expression> t;
  expression_holder<tensor_to_scalar_expression> u;
  expression_holder<scalar_expression> x = make_expression<scalar>("x");
  expression_holder<tensor_expression> A = make_expression<tensor>("A", 3, 2);
};

TEST_F(InvalidHolderFixture, EvaluatorsThrow) {
  scalar_evaluator<double> se;
  tensor_evaluator<double> te;
  tensor_to_scalar_evaluator<double> ue;
  EXPECT_THROW((void)se.apply(s), invalid_expression_error);
  EXPECT_THROW((void)te.apply(t), invalid_expression_error);
  EXPECT_THROW((void)ue.apply(u), invalid_expression_error);
}

TEST_F(InvalidHolderFixture, DiffThrowsOnEitherOperand) {
  EXPECT_THROW((void)diff(s, x), invalid_expression_error);
  EXPECT_THROW((void)diff(t, A), invalid_expression_error);
  EXPECT_THROW((void)diff(u, A), invalid_expression_error);
  EXPECT_THROW((void)diff(t, x), invalid_expression_error);
  EXPECT_THROW((void)diff(u, x), invalid_expression_error);
  EXPECT_THROW((void)diff(x, s), invalid_expression_error);
}

TEST_F(InvalidHolderFixture, SubstituteThrowsOnAnyOperand) {
  EXPECT_THROW((void)substitute(s, x, x), invalid_expression_error);
  EXPECT_THROW((void)substitute(t, A, A), invalid_expression_error);
  EXPECT_THROW((void)substitute(u, A, A), invalid_expression_error);
  EXPECT_THROW((void)substitute(x, s, x), invalid_expression_error);
  EXPECT_THROW((void)substitute(x, x, s), invalid_expression_error);
}

TEST_F(InvalidHolderFixture, LimitsThrow) {
  scalar_limit_visitor sv(x, limit_target{limit_target::point::zero_plus});
  EXPECT_THROW((void)sv.apply(s), invalid_expression_error);
  auto tr = trace(A);
  tensor_to_scalar_limit_visitor uv(
      tr, limit_target{limit_target::point::zero_plus});
  EXPECT_THROW((void)uv.apply(u), invalid_expression_error);
}

TEST_F(InvalidHolderFixture, ContainsThrowsOnInvalidHaystack) {
  EXPECT_THROW((void)contains_expression(s, x), invalid_expression_error);
  EXPECT_THROW((void)contains_expression(t, A), invalid_expression_error);
  EXPECT_THROW((void)depends_on_tensor(u, A), invalid_expression_error);
  EXPECT_THROW((void)contains_expression(x, s), invalid_expression_error);
}

TEST_F(InvalidHolderFixture, AssumptionQueriesThrow) {
  EXPECT_THROW((void)is_positive(s), invalid_expression_error);
  EXPECT_THROW((void)is_negative(s), invalid_expression_error);
  EXPECT_THROW((void)is_nonzero(s), invalid_expression_error);
  EXPECT_THROW((void)is_real(s), invalid_expression_error);
  EXPECT_THROW((void)is_symmetric(t), invalid_expression_error);
  EXPECT_THROW((void)is_orthogonal(t), invalid_expression_error);
}

// Printing is a diagnostic path: an invalid holder renders as a marked
// sentinel rather than throwing, so a null child inside a larger expression
// shows up exactly where it is.
TEST_F(InvalidHolderFixture, PrintersRenderSentinel) {
  EXPECT_EQ(to_string(s), "<invalid>");
  EXPECT_EQ(to_string(t), "<invalid>");
  EXPECT_EQ(to_string(u), "<invalid>");
  EXPECT_EQ(to_latex(s), "<invalid>");
  EXPECT_EQ(to_latex(t), "<invalid>");
  EXPECT_EQ(to_latex(u), "<invalid>");
  std::ostringstream os;
  os << s;
  EXPECT_EQ(os.str(), "<invalid>");
  // a null child is reported in place
  auto node = make_expression<scalar_sin>(s);
  EXPECT_EQ(to_string(node), "sin(<invalid>)");
}

// The wrong-type downcast is checked in every build, not only under assert.
TEST_F(InvalidHolderFixture, WrongTypeGetThrows) {
  EXPECT_THROW((void)x.get<scalar_sin>(), internal_error);
  EXPECT_THROW((void)A.get<tensor_negative>(), internal_error);
  EXPECT_NO_THROW((void)x.get<scalar>());
  EXPECT_NO_THROW((void)x.get<scalar_visitable_t>());
}

} // namespace numsim::cas

#endif // INVALIDHOLDERTEST_H
