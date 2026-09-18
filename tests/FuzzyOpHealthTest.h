#ifndef FUZZYOPHEALTHTEST_H
#define FUZZYOPHEALTHTEST_H

// Every registered op must actually produce evaluable, verifiable
// expressions. A capability-limit exception turns into a silent skip, so an
// op whose arguments land outside its domain would otherwise contribute
// nothing while the suite stayed green.

#include "FuzzyScalarDiffTest.h"
#include "FuzzyT2sDiffTest.h"
#include "FuzzyTensorDiffTest.h"

namespace numsim::cas {
namespace {

struct OpOutcome {
  int verified = 0; // derivative actually compared against finite differences
  int skipped = 0;  // capability-limit exception
  int unverifiable = 0; // non-finite values, or a stencil across a kink
};

template <typename Machine, typename Make>
void check_op_health(char const *domain, Make make_machine,
                     std::set<std::string> const &rank_limited = {}) {
  Machine probe = make_machine(1u);
  for (auto const &name : probe.op_names()) {
    OpOutcome outcome;
    for (unsigned seed = 1; seed <= 24u; ++seed) {
      Machine machine = make_machine(seed + 900000u);
      machine.force_root_op(name);
      int const unverified_before = fuzzy_detail::global_unverified();
      int const kink_before = fuzzy_detail::global_kink_skips();
      auto result = machine.run_one_test();
      bool const compared =
          fuzzy_detail::global_unverified() == unverified_before &&
          fuzzy_detail::global_kink_skips() == kink_before;
      if (result == fuzzy_detail::TestResult::Skip)
        ++outcome.skipped;
      else if (compared)
        ++outcome.verified;
      else
        ++outcome.unverifiable;
    }
    // Ops that can exceed the evaluator's rank ceiling skip legitimately;
    // everything else must mostly get through.
    int const floor = rank_limited.count(name) ? 4 : 12;
    EXPECT_GE(outcome.verified, floor)
        << domain << " op '" << name << "': " << outcome.verified
        << " verified, " << outcome.skipped << " skipped, "
        << outcome.unverifiable << " unverifiable of 24 runs";
  }
}

TEST(FuzzyOpHealth, ScalarOpsProduceVerifiableExpressions) {
  check_op_health<fuzzy_detail::FuzzyScalarMachine>("scalar", [](unsigned s) {
    return fuzzy_detail::FuzzyScalarMachine(s, 3);
  });
}

TEST(FuzzyOpHealth, T2sOpsProduceVerifiableExpressions) {
  check_op_health<fuzzy_detail::FuzzyT2sMachine>("t2s", [](unsigned s) {
    return fuzzy_detail::FuzzyT2sMachine(s, 3, true);
  });
}

TEST(FuzzyOpHealth, TensorOpsProduceVerifiableExpressions) {
  // otimes and levi_civita raise the rank of the derivative, so they run
  // into the evaluator's rank ceiling for a share of the seeds.
  check_op_health<fuzzy_detail::FuzzyTensorMachine>(
      "tensor",
      [](unsigned s) { return fuzzy_detail::FuzzyTensorMachine(s, 3); },
      {"otimes", "otimesu_l", "levi_civita", "simple_outer_product",
       "inner_product"});
}

} // namespace
} // namespace numsim::cas

#endif // FUZZYOPHEALTHTEST_H
