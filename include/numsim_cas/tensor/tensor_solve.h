#ifndef TENSOR_SOLVE_H
#define TENSOR_SOLVE_H

#include <vector>

#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

class tensor_solver {
public:
  using expr_holder_t = expression_holder<tensor_expression>;

  tensor_solver(expr_holder_t const &expr, expr_holder_t const &x);

  std::vector<expr_holder_t> solve() const;

private:
  expr_holder_t m_expr;
  expr_holder_t m_x;
};

// Solve tensor equation expr == 0 for X.
// Returns solutions (empty if not linear or unsolvable).
// Currently supports: linear equations (degree 1).
std::vector<expression_holder<tensor_expression>>
solve(expression_holder<tensor_expression> const &expr,
      expression_holder<tensor_expression> const &x);

} // namespace numsim::cas

#endif // TENSOR_SOLVE_H
