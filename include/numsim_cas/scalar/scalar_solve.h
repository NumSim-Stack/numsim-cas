#ifndef SCALAR_SOLVE_H
#define SCALAR_SOLVE_H

#include <map>
#include <optional>
#include <vector>

#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

// Why a solve produced the solutions it did. An empty solution list alone
// cannot tell "no real root" from "this form is not handled".
enum class solve_outcome {
  solved,           // every listed solution is a real root
  no_real_solution, // provably no real root (e.g. x^2 + 1)
  all_values,       // the equation is identically zero
  no_variable,      // x does not occur in the equation
  unsupported       // not a polynomial of degree <= 2 in x
};

struct solve_result {
  solve_outcome outcome;
  std::vector<expression_holder<scalar_expression>> solutions;
};

class polynomial_solver {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using poly_map = std::map<long long, expr_holder_t>;

  polynomial_solver(expr_holder_t const &expr, expr_holder_t const &x);

  std::optional<poly_map> polynomial_coefficients() const;
  std::vector<expr_holder_t> solve() const;
  solve_result solve_with_outcome() const;

private:
  std::optional<poly_map> classify_term(expr_holder_t const &expr) const;

  static void merge_into(poly_map &pm, long long degree,
                         expr_holder_t const &coeff);
  static poly_map negate_poly(poly_map const &pm);
  static poly_map multiply_poly(poly_map const &a, poly_map const &b);
  static poly_map add_poly(poly_map const &a, poly_map const &b);
  static bool is_numeric_zero(expr_holder_t const &expr);

  expr_holder_t m_expr;
  expr_holder_t m_x;
};

// Extract polynomial coefficients: expr decomposed as sum of coeff_i * x^i.
// Returns nullopt if not polynomial in x (e.g., sin(x), pow(x, non-integer)).
std::optional<std::map<long long, expression_holder<scalar_expression>>>
polynomial_coefficients(expression_holder<scalar_expression> const &expr,
                        expression_holder<scalar_expression> const &x);

// Solve expr == 0 for x over the reals; supports degree <= 2 in x.
// A quadratic with a discriminant of unknown sign yields the two-root form,
// which is real only where the discriminant is nonnegative.
solve_result
solve_with_outcome(expression_holder<scalar_expression> const &expr,
                   expression_holder<scalar_expression> const &x);

// The solutions of solve_with_outcome; empty for every outcome but `solved`.
std::vector<expression_holder<scalar_expression>>
solve(expression_holder<scalar_expression> const &expr,
      expression_holder<scalar_expression> const &x);

} // namespace numsim::cas

#endif // SCALAR_SOLVE_H
