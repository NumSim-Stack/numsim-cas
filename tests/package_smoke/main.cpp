// Consumes NumSim_CAS through the umbrella header alone: every feature
// exercised here must stay reachable without naming an internal path.
#include <numsim_cas/numsim_cas.h>

#include <iostream>
#include <tuple>

int main() {
  using namespace numsim::cas;

  auto [x, y] = make_scalar_variable("x", "y");
  auto expr = x + 2 * y;

  scalar_evaluator<double> evaluator;
  evaluator.set(x, 1.0);
  evaluator.set(y, 3.0);

  auto value = evaluator.apply(expr);
  std::cout << value << '\n';

  auto substituted = substitute(expr, y, x);
  auto derivative = diff(expr, x);

  scalar_limit_visitor limit_at_zero(x, {limit_target::point::zero_plus});
  auto limit_of_sin = limit_at_zero.apply(sin(x));

  auto [A] =
      make_tensor_variable(std::tuple{"A", std::size_t{3}, std::size_t{2}});
  A.assumption(Symmetric{});
  const bool trace_uses_A = depends_on_tensor(trace(A), A);
  auto isotropic = exp(A);
  auto latex = to_latex(expr);

  scalar_function_simplifier pass;
  auto folded = pass.apply(cos(asin(x)));

  return (value == 7.0 && substituted.is_valid() && derivative.is_valid() &&
          limit_of_sin.dir == limit_result::direction::zero && trace_uses_A &&
          isotropic.is_valid() && !latex.empty() && folded.is_valid())
             ? 0
             : 1;
}
