#include <numsim_cas/numsim_cas.h>

#include <cmath>
#include <iostream>
#include <numbers>
#include <tuple>

using namespace numsim::cas;
using std::cos;
using std::exp;
using std::pow;
using std::sin;
using std::sqrt;

int main() {
  auto [x, y] = make_scalar_variable("x", "y");
  auto [_2, _3] = make_scalar_constant(2, 3);

  std::cout << "=== Scalar Evaluation ===\n\n";

  // Simple evaluation
  {
    scalar_evaluator<double> ev;
    ev.set(x, 3.0);

    auto expr = pow(x, _2) + _2 * x + make_scalar_constant(1);
    std::cout << "f(x)   = " << to_string(expr) << "\n";
    std::cout << "f(3)   = " << ev.apply(expr) << "\n";
    std::cout << "  (expected: 9 + 6 + 1 = 16)\n\n";
  }

  // Multi-variable evaluation
  {
    scalar_evaluator<double> ev;
    ev.set(x, 2.0);
    ev.set(y, 3.0);

    auto expr = x * y + pow(x, _2);
    std::cout << "g(x,y) = " << to_string(expr) << "\n";
    std::cout << "g(2,3) = " << ev.apply(expr) << "\n";
    std::cout << "  (expected: 6 + 4 = 10)\n\n";
  }

  // Trigonometric identity: sin^2(x) + cos^2(x) = 1
  {
    scalar_evaluator<double> ev;
    ev.set(x, 1.23);

    auto identity = sin(x) * sin(x) + cos(x) * cos(x);
    std::cout << "sin^2(x) + cos^2(x) = " << to_string(identity) << "\n";
    std::cout << "  at x=1.23: " << ev.apply(identity) << "\n";
    std::cout << "  (expected: 1.0)\n\n";
  }

  // Evaluate a derivative numerically
  {
    auto expr = pow(x, _3);     // x^3
    auto dexpr = diff(expr, x); // 3*x^2

    scalar_evaluator<double> ev;
    ev.set(x, 2.0);

    std::cout << "f(x)     = " << to_string(expr) << "\n";
    std::cout << "f'(x)    = " << to_string(dexpr) << "\n";
    std::cout << "f(2)     = " << ev.apply(expr) << "\n";
    std::cout << "f'(2)    = " << ev.apply(dexpr) << "\n";
    std::cout << "  (expected: f(2)=8, f'(2)=12)\n";
  }
}
