#include <numsim_cas/numsim_cas.h>

#include <iostream>
#include <tuple>

using namespace numsim::cas;
using std::cos;
using std::exp;
using std::log;
using std::pow;
using std::sin;
using std::sqrt;
using std::tan;

int main() {
  auto [x, y] = make_scalar_variable("x", "y");
  auto [_2, _3] = make_scalar_constant(2, 3);

  std::cout << "=== Scalar Differentiation ===\n\n";

  // Basic derivatives
  std::cout << "--- Basic Rules ---\n";
  std::cout << "d/dx(x)         = " << to_string(diff(x, x)) << "\n";
  std::cout << "d/dx(y)         = " << to_string(diff(y, x)) << "\n";
  std::cout << "d/dx(5)         = " << to_string(diff(make_scalar_constant(5), x)) << "\n";

  // Sum rule
  std::cout << "\n--- Sum Rule ---\n";
  std::cout << "d/dx(x + y)     = " << to_string(diff(x + y, x)) << "\n";
  std::cout << "d/dx(x + x)     = " << to_string(diff(x + x, x)) << "\n";

  // Product rule: d/dx(x*y) = y
  std::cout << "\n--- Product Rule ---\n";
  std::cout << "d/dx(x * y)     = " << to_string(diff(x * y, x)) << "\n";
  std::cout << "d/dx(x * x)     = " << to_string(diff(x * x, x)) << "\n";

  // Power rule: d/dx(x^3) = 3*x^2
  std::cout << "\n--- Power Rule ---\n";
  std::cout << "d/dx(x^2)       = " << to_string(diff(pow(x, _2), x)) << "\n";
  std::cout << "d/dx(x^3)       = " << to_string(diff(pow(x, _3), x)) << "\n";

  // Quotient rule: d/dx(x/y) = 1/y
  std::cout << "\n--- Quotient Rule ---\n";
  std::cout << "d/dx(x / y)     = " << to_string(diff(x / y, x)) << "\n";

  // Trigonometric functions
  std::cout << "\n--- Trigonometric ---\n";
  std::cout << "d/dx(sin(x))    = " << to_string(diff(sin(x), x)) << "\n";
  std::cout << "d/dx(cos(x))    = " << to_string(diff(cos(x), x)) << "\n";

  // Chain rule
  std::cout << "\n--- Chain Rule ---\n";
  std::cout << "d/dx(sin(x^2))  = " << to_string(diff(sin(pow(x, _2)), x)) << "\n";
  std::cout << "d/dx(exp(x))    = " << to_string(diff(exp(x), x)) << "\n";
  std::cout << "d/dx(log(x))    = " << to_string(diff(log(x), x)) << "\n";
}
