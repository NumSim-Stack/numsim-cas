#ifndef FUZZYHARNESSTEST_H
#define FUZZYHARNESSTEST_H

#include "FuzzyDiffBase.h"

#include <gtest/gtest-spi.h>

#include <functional>
#include <stdexcept>

namespace numsim::cas::fuzzy_detail {
namespace {

enum class ThrowPhase { Generation, Differentiation, Verification };

struct StubExprInfo {
  std::set<std::string> used_vars;
};

struct StubVarEntry {
  std::string name;
};

// Minimal machine whose hooks throw a chosen exception in a chosen phase.
class ThrowingMachine : public FuzzyDiffBase<ThrowingMachine, StubExprInfo> {
  using Base = FuzzyDiffBase<ThrowingMachine, StubExprInfo>;
  friend Base;

public:
  ThrowingMachine(ThrowPhase phase, std::function<void()> thrower)
      : Base(0, 0), m_phase(phase), m_thrower(std::move(thrower)) {}

  StubExprInfo generate_leaf() {
    maybe_throw(ThrowPhase::Generation);
    return {{"x"}};
  }
  StubExprInfo negation_fallback(std::size_t) { return {{"x"}}; }
  std::vector<StubVarEntry> const &get_diff_vars() const { return m_vars; }
  static std::string get_var_name(StubVarEntry const &v) { return v.name; }
  static bool can_diff_wrt(StubExprInfo const &, StubVarEntry const &) {
    return true;
  }
  std::optional<int> do_diff(StubExprInfo const &, StubVarEntry const &) {
    maybe_throw(ThrowPhase::Differentiation);
    return 0;
  }
  VerifyResult verify(StubExprInfo const &, StubVarEntry const &, int) {
    maybe_throw(ThrowPhase::Verification);
    return {true, {}};
  }
  void report_failure(std::string const &diagnostic, StubExprInfo const &,
                      StubVarEntry const * = nullptr, int const * = nullptr) {
    ADD_FAILURE() << diagnostic;
  }

private:
  void maybe_throw(ThrowPhase phase) {
    if (phase == m_phase)
      m_thrower();
  }

  ThrowPhase m_phase;
  std::function<void()> m_thrower;
  std::vector<StubVarEntry> m_vars{{"x"}};
};

class FuzzyHarnessExceptionTest : public ::testing::TestWithParam<ThrowPhase> {
};

// A non-CAS exception is a library bug, not a capability limit.
TEST_P(FuzzyHarnessExceptionTest, NonCasExceptionFails) {
  ThrowingMachine machine(
      GetParam(), [] { throw std::out_of_range("sentinel out_of_range"); });
  TestResult result = TestResult::Pass;
  EXPECT_NONFATAL_FAILURE(result = machine.run_one_test(),
                          "sentinel out_of_range");
  EXPECT_EQ(static_cast<int>(result), static_cast<int>(TestResult::Fail));
}

TEST_P(FuzzyHarnessExceptionTest, InternalErrorFails) {
  ThrowingMachine machine(GetParam(),
                          [] { throw internal_error("sentinel invariant"); });
  TestResult result = TestResult::Pass;
  EXPECT_NONFATAL_FAILURE(result = machine.run_one_test(),
                          "sentinel invariant");
  EXPECT_EQ(static_cast<int>(result), static_cast<int>(TestResult::Fail));
}

TEST_P(FuzzyHarnessExceptionTest, CapabilityLimitSkips) {
  ThrowingMachine machine(GetParam(), [] {
    throw not_implemented_error("sentinel capability limit");
  });
  EXPECT_EQ(static_cast<int>(machine.run_one_test()),
            static_cast<int>(TestResult::Skip));
  EXPECT_NE(machine.skip_reason().find("sentinel capability limit"),
            std::string::npos);
}

INSTANTIATE_TEST_SUITE_P(
    Phases, FuzzyHarnessExceptionTest,
    ::testing::Values(ThrowPhase::Generation, ThrowPhase::Differentiation,
                      ThrowPhase::Verification),
    [](::testing::TestParamInfo<ThrowPhase> const &info) -> std::string {
      switch (info.param) {
      case ThrowPhase::Generation:
        return "Generation";
      case ThrowPhase::Differentiation:
        return "Differentiation";
      case ThrowPhase::Verification:
        return "Verification";
      }
      return "Unknown";
    });

} // namespace
} // namespace numsim::cas::fuzzy_detail

#endif // FUZZYHARNESSTEST_H
