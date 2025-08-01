#include "symTM_diff_test.h"
#include "symTM_print_test.h"
#include "symTM_test.h"
#include "test_scalar_operators.h"

#include "gtest/gtest.h"

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
