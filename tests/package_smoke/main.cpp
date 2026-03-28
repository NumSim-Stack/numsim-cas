#include <numsim_cas/numsim_cas.h>

#include <iostream>

int main() {
  using namespace numsim::cas;

  auto [x, y] = make_scalar_variable("x", "y");
  auto expr = x + 2 * y;

  scalar_evaluator<double> evaluator;
  evaluator.set(x, 1.0);
  evaluator.set(y, 3.0);

  auto value = evaluator.apply(expr);
  std::cout << value << '\n';

  return value == 7.0 ? 0 : 1;
}
