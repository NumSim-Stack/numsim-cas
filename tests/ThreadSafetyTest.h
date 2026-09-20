#ifndef THREADSAFETYTEST_H
#define THREADSAFETYTEST_H

#include "cas_test_helpers.h"
#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"

#include <atomic>
#include <thread>
#include <vector>

// Concurrent reads of one shared expression: the lazily cached hash and the
// inferred assumptions are written on first read, so these are only sound if
// that first write is published safely.
namespace numsim::cas {

namespace {
template <typename Fn> void run_threads(std::size_t count, Fn &&fn) {
  std::vector<std::thread> workers;
  workers.reserve(count);
  for (std::size_t t = 0; t < count; ++t)
    workers.emplace_back(fn, t);
  for (auto &w : workers)
    w.join();
}
constexpr std::size_t threads = 4;
constexpr int rounds = 200;
} // namespace

TEST(ThreadSafety, ConcurrentHashReadsOnSharedNode) {
  auto [x] = make_scalar_variable("x");
  // built directly, so nothing has hashed it yet
  auto lazy = make_expression<scalar_sin>(make_expression<scalar_cos>(x));
  auto const expected =
      make_expression<scalar_sin>(make_expression<scalar_cos>(x))
          .get()
          .hash_value();
  std::atomic<int> mismatches{0};
  run_threads(threads, [&](std::size_t) {
    for (int i = 0; i < rounds; ++i)
      if (lazy.get().hash_value() != expected)
        mismatches.fetch_add(1, std::memory_order_relaxed);
  });
  EXPECT_EQ(mismatches.load(), 0);
}

TEST(ThreadSafety, ConcurrentHashReadsOnSharedTensorNode) {
  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  auto lazy = make_expression<tensor_inv>(make_expression<tensor_negative>(A));
  auto const expected =
      make_expression<tensor_inv>(make_expression<tensor_negative>(A))
          .get()
          .hash_value();
  std::atomic<int> mismatches{0};
  run_threads(threads, [&](std::size_t) {
    for (int i = 0; i < rounds; ++i)
      if (lazy.get().hash_value() != expected)
        mismatches.fetch_add(1, std::memory_order_relaxed);
  });
  EXPECT_EQ(mismatches.load(), 0);
}

TEST(ThreadSafety, ConcurrentAssumptionQueriesOnSharedNode) {
  auto [x, y] = make_scalar_variable("x", "y");
  x.assumption(positive{});
  auto const build = [&] {
    return abs(x) + sqrt(y * y + make_expression<scalar_constant>(1));
  };
  // the reference is a separate tree, so the shared one reaches the threads
  // with its facts still underived and they race to infer them
  auto const reference = build();
  bool const pos = is_positive(reference);
  bool const neg = is_negative(reference);
  bool const nonneg = is_nonnegative(reference);

  auto shared = build();
  std::atomic<int> wrong{0};
  run_threads(threads, [&](std::size_t) {
    for (int i = 0; i < rounds; ++i) {
      if (is_positive(shared) != pos || is_negative(shared) != neg ||
          is_nonnegative(shared) != nonneg)
        wrong.fetch_add(1, std::memory_order_relaxed);
    }
  });
  EXPECT_EQ(wrong.load(), 0);
  EXPECT_TRUE(is_positive(x));
}

TEST(ThreadSafety, ConcurrentTensorQueriesOnSharedNode) {
  auto [F] =
      make_tensor_variable(std::tuple{"F", std::size_t{3}, std::size_t{2}});
  auto C = trans(F) * F;
  std::atomic<int> wrong{0};
  run_threads(threads, [&](std::size_t) {
    for (int i = 0; i < rounds; ++i) {
      if (!is_symmetric(C) || is_skew(C))
        wrong.fetch_add(1, std::memory_order_relaxed);
    }
  });
  EXPECT_EQ(wrong.load(), 0);
}

TEST(ThreadSafety, ConcurrentEvaluationWithPerThreadEvaluators) {
  auto [x, y] = make_scalar_variable("x", "y");
  auto f =
      sin(x) * exp(y) + pow(x, 3) / (y + make_expression<scalar_constant>(2));
  std::atomic<int> wrong{0};
  run_threads(threads, [&](std::size_t t) {
    scalar_evaluator<double> ev;
    double const xv = 0.5 + static_cast<double>(t);
    ev.set(x, xv);
    ev.set(y, 1.5);
    double const reference =
        std::sin(xv) * std::exp(1.5) + std::pow(xv, 3) / 3.5;
    for (int i = 0; i < rounds; ++i)
      if (std::abs(ev.apply(f) - reference) > 1e-12)
        wrong.fetch_add(1, std::memory_order_relaxed);
  });
  EXPECT_EQ(wrong.load(), 0);
}

} // namespace numsim::cas

#endif // THREADSAFETYTEST_H
