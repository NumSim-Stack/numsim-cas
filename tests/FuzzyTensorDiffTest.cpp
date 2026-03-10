// Standalone fuzzy expression tree generator for differentiation testing.
//
// Generates random composite expression trees (seeded RNG) and verifies
// symbolic derivatives against numerical finite differences.
//
// Build target: numsim_cas_fuzz_test (separate from the main unit tests)

#include "FuzzyTensorDiffTest.h"
#include "FuzzyScalarDiffTest.h"
#include "FuzzyT2sDiffTest.h"

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  numsim::cas::fuzzy_detail::print_coverage_summary();
  return result;
}
