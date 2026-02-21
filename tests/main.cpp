#include "CoreBugFixTest.h"
#include "LimitVisitorTest.h"
#include "ScalarAssumptionTest.h"
#include "ScalarDifferentiationTest.h"
#include "ScalarEvaluatorTest.h"
#include "ScalarExpressionTest.h"
#include "ScalarSubstitutionTest.h"
#include "TensorDifferentiationTest.h"
#include "TensorEvaluatorTest.h"
#include "TensorExpressionTest.h"
#include "TensorProjectorDifferentiationTest.h"
#include "TensorSpacePropagationTest.h"
#include "TensorSubstitutionTest.h"
#include "TensorToScalarDifferentiationTest.h"
#include "TensorToScalarEvaluatorTest.h"
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
