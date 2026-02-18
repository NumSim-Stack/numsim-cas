#include "ScalarDifferentiationTest.h"
#include "ScalarEvaluatorTest.h"
#include "ScalarExpressionTest.h"
#include "TensorEvaluatorTest.h"
#include "ScalarSubstitutionTest.h"
#include "TensorExpressionTest.h"
#include "TensorSubstitutionTest.h"
#include "TensorToScalarDifferentiationTest.h"
#include "TensorToScalarExpressionTest.h"
#include "TensorToScalarSubstitutionTest.h"
#include "symTM_diff_test.h"
#include "symTM_print_test.h"
#include "symTM_test.h"
#include "gtest/gtest.h"

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
