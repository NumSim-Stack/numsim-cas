#include <numsim_cas/numsim_cas.h>

#include <iostream>
#include <tuple>

using namespace numsim::cas;
using std::abs;
using std::cos;
using std::exp;
using std::log;
using std::pow;
using std::sin;
using std::sqrt;
using std::tan;

int main() {
  // Create symbolic variables
  auto [x, y, z] = make_scalar_variable("x", "y", "z");

  // Create symbolic constants
  auto [_2, _3] = make_scalar_constant(2, 3);

  std::cout << "=== Scalar Basics ===\n\n";

  // Basic arithmetic
  std::cout << "x + y       = " << to_string(x + y) << "\n";
  std::cout << "x * y       = " << to_string(x * y) << "\n";
  std::cout << "x - y       = " << to_string(x - y) << "\n";
  std::cout << "x / y       = " << to_string(x / y) << "\n";
  std::cout << "-x          = " << to_string(-x) << "\n";

  // Automatic simplification
  std::cout << "\n--- Simplification ---\n";
  std::cout << "x + x       = " << to_string(x + x) << "\n";
  std::cout << "2*x + 3*x   = " << to_string(_2 * x + _3 * x) << "\n";
  std::cout << "x * x       = " << to_string(x * x) << "\n";
  std::cout << "x - x       = " << to_string(x - x) << "\n";
  std::cout << "x + 0       = " << to_string(x + get_scalar_zero()) << "\n";
  std::cout << "x * 1       = " << to_string(x * get_scalar_one()) << "\n";
  std::cout << "x * 0       = " << to_string(x * get_scalar_zero()) << "\n";

  // Powers and roots
  std::cout << "\n--- Powers & Roots ---\n";
  std::cout << "pow(x, 2)   = " << to_string(pow(x, _2)) << "\n";
  std::cout << "pow(x, 3)   = " << to_string(pow(x, _3)) << "\n";
  std::cout << "sqrt(x)     = " << to_string(sqrt(x)) << "\n";

  // Standard math functions
  std::cout << "\n--- Functions ---\n";
  std::cout << "sin(x)      = " << to_string(sin(x)) << "\n";
  std::cout << "cos(x)      = " << to_string(cos(x)) << "\n";
  std::cout << "tan(x)      = " << to_string(tan(x)) << "\n";
  std::cout << "exp(x)      = " << to_string(exp(x)) << "\n";
  std::cout << "log(x)      = " << to_string(log(x)) << "\n";
  std::cout << "abs(x)      = " << to_string(abs(x)) << "\n";

  // Compound expressions
  std::cout << "\n--- Compound Expressions ---\n";
  auto poly = pow(x, _3) + _2 * pow(x, _2) + x + _3;
  std::cout << "x^3 + 2*x^2 + x + 3 = " << to_string(poly) << "\n";

  auto frac = (x + y) / (x - y);
  std::cout << "(x+y)/(x-y)         = " << to_string(frac) << "\n";

  auto nested = sin(x * y) + cos(pow(x, _2));
  std::cout << "sin(x*y)+cos(x^2)   = " << to_string(nested) << "\n";
}
