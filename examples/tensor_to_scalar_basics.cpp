#include <numsim_cas/numsim_cas.h>
#include <numsim_cas/tensor/data/tensor_data.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_evaluator.h>

#include <cmath>
#include <cstddef>
#include <iostream>
#include <memory>
#include <tuple>

using namespace numsim::cas;

// Helper: create tensor data from an initializer list of raw values
template <std::size_t Dim, std::size_t Rank>
auto make_test_data(std::initializer_list<double> values) {
  auto ptr = std::make_shared<tensor_data<double, Dim, Rank>>();
  auto *raw = ptr->raw_data();
  std::size_t i = 0;
  for (auto v : values)
    raw[i++] = v;
  return ptr;
}

int main() {
  // Create rank-2 tensor variables in 3D
  auto [X, Y] =
      make_tensor_variable(std::tuple{"X", 3, 2}, std::tuple{"Y", 3, 2});

  // Create scalar variables and constants
  auto [x] = make_scalar_variable("x");
  auto [_2, _3] = make_scalar_constant(2, 3);

  std::cout << "=== Tensor-to-Scalar Basics ===\n\n";

  // --- Basic tensor-to-scalar functions ---
  std::cout << "--- Basic Functions ---\n";
  std::cout << "trace(X)   = " << to_string(trace(X)) << "\n";
  std::cout << "det(X)     = " << to_string(det(X)) << "\n";
  std::cout << "norm(X)    = " << to_string(norm(X)) << "\n";
  std::cout << "dot(X)     = " << to_string(dot(X)) << "\n";

  // General contraction to scalar
  using seq = sequence;
  std::cout << "X:Y        = "
            << to_string(dot_product(X, seq{1, 2}, Y, seq{1, 2})) << "\n";

  // --- Arithmetic on tensor-to-scalar expressions ---
  std::cout << "\n--- Arithmetic ---\n";
  std::cout << "tr(X) + tr(Y)   = " << to_string(trace(X) + trace(Y)) << "\n";
  std::cout << "tr(X) + tr(X)   = " << to_string(trace(X) + trace(X)) << "\n";
  std::cout << "tr(X) * tr(Y)   = " << to_string(trace(X) * trace(Y)) << "\n";
  std::cout << "tr(X) * tr(X)   = " << to_string(trace(X) * trace(X)) << "\n";
  std::cout << "pow(tr(X), 3)   = " << to_string(pow(trace(X), 3)) << "\n";
  std::cout << "log(det(X))     = " << to_string(log(det(X))) << "\n";

  // --- Mixing with scalar variables ---
  std::cout << "\n--- Mixing with Scalars ---\n";
  std::cout << "x + tr(X)       = " << to_string(x + trace(X)) << "\n";
  std::cout << "x * tr(X)       = " << to_string(x * trace(X)) << "\n";
  std::cout << "2*x + tr(X)     = " << to_string(_2 * x + trace(X)) << "\n";

  // --- Numeric evaluation ---
  std::cout << "\n--- Numeric Evaluation ---\n";
  {
    tensor_to_scalar_evaluator<double> ev;

    // X = [1 2 0; 3 4 0; 0 0 5], Y = [5 6 0; 7 8 0; 0 0 1]
    // clang-format off
    ev.set(X, make_test_data<3, 2>({1.0, 2.0, 0.0,
                                     3.0, 4.0, 0.0,
                                     0.0, 0.0, 5.0}));
    ev.set(Y, make_test_data<3, 2>({5.0, 6.0, 0.0,
                                     7.0, 8.0, 0.0,
                                     0.0, 0.0, 1.0}));
    // clang-format on
    ev.set_scalar(x, 3.0);

    // trace(X) = 1 + 4 + 5 = 10
    std::cout << "trace(X)        = " << ev.apply(trace(X))
              << "  (expected: 10)\n";

    // trace(Y) = 5 + 8 + 1 = 14
    std::cout << "trace(Y)        = " << ev.apply(trace(Y))
              << "  (expected: 14)\n";

    // trace(X) + trace(Y) = 24
    std::cout << "tr(X) + tr(Y)   = " << ev.apply(trace(X) + trace(Y))
              << "  (expected: 24)\n";

    // trace(X) * trace(Y) = 140
    std::cout << "tr(X) * tr(Y)   = " << ev.apply(trace(X) * trace(Y))
              << "  (expected: 140)\n";

    // pow(trace(X), 3) = 1000
    std::cout << "pow(tr(X), 3)   = " << ev.apply(pow(trace(X), 3))
              << "  (expected: 1000)\n";

    // det(X) = 1*4*5 - 2*3*5 = 20 - 30 = -10
    std::cout << "det(X)          = " << ev.apply(det(X))
              << "  (expected: -10)\n";

    // log(|det(X)|) — use det of Y which is positive: det(Y) = 5*8-6*7 = -2,
    // so use a positive-definite matrix instead
    // log(det(X)^2) = log(100) ≈ 4.605
    auto log_det_sq = log(pow(det(X), 2));
    std::cout << "log(det(X)^2)   = " << ev.apply(log_det_sq)
              << "  (expected: " << std::log(100.0) << ")\n";

    // dot(X) = X:X = 1+4+0+9+16+0+0+0+25 = 55
    std::cout << "dot(X)          = " << ev.apply(dot(X))
              << "  (expected: 55)\n";

    // norm(X) = sqrt(dot(X)) = sqrt(55)
    std::cout << "norm(X)         = " << ev.apply(norm(X))
              << "  (expected: " << std::sqrt(55.0) << ")\n";

    // X:Y = 1*5+2*6+0+3*7+4*8+0+0+0+5*1 = 5+12+21+32+5 = 75
    std::cout << "X:Y             = "
              << ev.apply(dot_product(X, seq{1, 2}, Y, seq{1, 2}))
              << "  (expected: 75)\n";
  }
}
