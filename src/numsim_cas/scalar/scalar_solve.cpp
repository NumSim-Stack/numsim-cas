#include <numsim_cas/scalar/scalar_solve.h>

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_assume.h>
#include <numsim_cas/scalar/scalar_domain_traits.h>
#include <numsim_cas/scalar/scalar_functions.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>

#include <ranges>

namespace numsim::cas {

// Forward declare only the scalar overload to avoid pulling in tensor headers
// from contains_expression.h.
bool contains_expression(expression_holder<scalar_expression> const &haystack,
                         expression_holder<scalar_expression> const &needle);

polynomial_solver::polynomial_solver(expr_holder_t const &expr,
                                     expr_holder_t const &x)
    : m_expr(expr), m_x(x) {}

void polynomial_solver::merge_into(poly_map &pm, long long degree,
                                   expr_holder_t const &coeff) {
  auto it = pm.find(degree);
  if (it == pm.end()) {
    pm[degree] = coeff;
  } else {
    it->second = it->second + coeff;
  }
}

polynomial_solver::poly_map polynomial_solver::negate_poly(poly_map const &pm) {
  poly_map result;
  for (auto const &[deg, coeff] : pm) {
    result[deg] = -coeff;
  }
  return result;
}

polynomial_solver::poly_map
polynomial_solver::multiply_poly(poly_map const &a, poly_map const &b) {
  poly_map result;
  for (auto const &[da, ca] : a) {
    for (auto const &[db, cb] : b) {
      merge_into(result, da + db, ca * cb);
    }
  }
  return result;
}

polynomial_solver::poly_map polynomial_solver::add_poly(poly_map const &a,
                                                        poly_map const &b) {
  poly_map result = a;
  for (auto const &[deg, coeff] : b) {
    merge_into(result, deg, coeff);
  }
  return result;
}

bool polynomial_solver::is_numeric_zero(expr_holder_t const &expr) {
  auto val = domain_traits<scalar_expression>::try_numeric(expr);
  return val && *val == scalar_number{0};
}

std::optional<polynomial_solver::poly_map>
polynomial_solver::classify_term(expr_holder_t const &expr) const {
  // If expr doesn't contain x, it's a degree-0 constant.
  if (!contains_expression(expr, m_x)) {
    return poly_map{{0, expr}};
  }

  // If expr IS x, it's degree 1 with coefficient 1.
  if (expr == m_x) {
    return poly_map{{1, get_scalar_one()}};
  }

  // pow(base, exp)
  if (is_same<scalar_pow>(expr)) {
    auto const &p = expr.get<scalar_pow>();
    auto const &base = p.expr_lhs();
    auto const &exp = p.expr_rhs();

    // Only handle pow(x, integer_constant) where base is exactly x
    if (base == m_x) {
      auto n = try_int_constant(exp);
      if (n && *n >= 0) {
        return poly_map{{*n, get_scalar_one()}};
      }
    }
    // pow(f(x), n) where f(x) is not just x: not supported
    return std::nullopt;
  }

  // scalar_negative(inner)
  if (is_same<scalar_negative>(expr)) {
    auto const &neg = expr.get<scalar_negative>();
    auto inner = classify_term(neg.expr());
    if (!inner)
      return std::nullopt;
    return negate_poly(*inner);
  }

  // scalar_add: coeff + sum of children
  if (is_same<scalar_add>(expr)) {
    auto const &add = expr.get<scalar_add>();
    poly_map result;

    // Start with the numeric coefficient (if any)
    if (add.coeff().is_valid()) {
      result[0] = add.coeff();
    }

    // Add each child's polynomial map
    for (auto const &child : add.symbol_map() | std::views::values) {
      auto child_poly = classify_term(child);
      if (!child_poly)
        return std::nullopt;
      result = add_poly(result, *child_poly);
    }
    return result;
  }

  // scalar_mul: coeff * product of children
  if (is_same<scalar_mul>(expr)) {
    auto const &mul = expr.get<scalar_mul>();

    // Start with the numeric coefficient
    poly_map result;
    if (mul.coeff().is_valid()) {
      result[0] = mul.coeff();
    } else {
      result[0] = get_scalar_one();
    }

    // Multiply by each child's polynomial map
    for (auto const &child : mul.symbol_map() | std::views::values) {
      auto child_poly = classify_term(child);
      if (!child_poly)
        return std::nullopt;
      result = multiply_poly(result, *child_poly);
    }
    return result;
  }

  // scalar_named_expression: unwrap and recurse
  if (is_same<scalar_named_expression>(expr)) {
    auto const &named = expr.get<scalar_named_expression>();
    return classify_term(named.expr());
  }

  // Transcendental functions containing x: not polynomial
  // (sin, cos, tan, asin, acos, atan, sqrt, log, exp, sign, abs)
  return std::nullopt;
}

std::optional<polynomial_solver::poly_map>
polynomial_solver::polynomial_coefficients() const {
  return classify_term(m_expr);
}

namespace {

// Sign of an expression when it is provable: a numeric value, or an assumed
// sign on a symbolic one.
std::optional<int> known_sign(expression_holder<scalar_expression> const &e) {
  if (auto val = domain_traits<scalar_expression>::try_numeric(e)) {
    scalar_number const zero{0};
    if (numeric_less(*val, zero))
      return -1;
    if (numeric_less(zero, *val))
      return 1;
    return 0;
  }
  if (is_positive(e))
    return 1;
  if (is_negative(e))
    return -1;
  return std::nullopt;
}

} // namespace

std::vector<polynomial_solver::expr_holder_t> polynomial_solver::solve() const {
  return solve_with_outcome().solutions;
}

solve_result polynomial_solver::solve_with_outcome() const {
  if (is_numeric_zero(m_expr))
    return {solve_outcome::all_values, {}};
  if (!contains_expression(m_expr, m_x))
    return {solve_outcome::no_variable, {}};

  auto coeffs_opt = classify_term(m_expr);
  if (!coeffs_opt)
    return {solve_outcome::unsupported, {}};

  auto &coeffs = *coeffs_opt;

  for (auto it = coeffs.begin(); it != coeffs.end();) {
    if (is_numeric_zero(it->second)) {
      it = coeffs.erase(it);
    } else {
      ++it;
    }
  }

  if (coeffs.empty())
    return {solve_outcome::all_values, {}};

  long long max_degree = coeffs.rbegin()->first;

  // x cancelled out and a nonzero constant remains
  if (max_degree == 0)
    return {solve_outcome::no_real_solution, {}};

  auto get_coeff = [&](long long deg) -> expr_holder_t {
    auto it = coeffs.find(deg);
    if (it == coeffs.end())
      return get_scalar_zero();
    return it->second;
  };

  if (max_degree == 1) {
    auto a = get_coeff(1);
    auto b = get_coeff(0);
    return {solve_outcome::solved, {-b / a}};
  }

  if (max_degree == 2) {
    auto a = get_coeff(2);
    auto b = get_coeff(1);
    auto c = get_coeff(0);

    auto two = get_scalar_one() + get_scalar_one();
    auto four = two + two;

    auto disc = b * b - four * a * c;

    if (is_numeric_zero(disc))
      return {solve_outcome::solved, {-b / (two * a)}};

    auto disc_sign = known_sign(disc);
    if (disc_sign && *disc_sign < 0)
      return {solve_outcome::no_real_solution, {}};

    auto sqrt_disc = sqrt(disc);
    auto b_sign = known_sign(b);
    if (!b_sign || *b_sign == 0) {
      // With b = 0 the textbook form has no cancelling pair; with b of
      // unknown sign it is the only closed form available.
      auto two_a = two * a;
      return {solve_outcome::solved,
              {(-b + sqrt_disc) / two_a, (-b - sqrt_disc) / two_a}};
    }

    // q = -(b + sign(b) sqrt(disc)) / 2 adds terms of equal sign, so the
    // roots q/a and c/q do not cancel when b^2 >> 4ac.
    auto signed_sqrt = *b_sign > 0 ? sqrt_disc : -sqrt_disc;
    auto q = -(b + signed_sqrt) / two;
    return {solve_outcome::solved, {q / a, c / q}};
  }

  return {solve_outcome::unsupported, {}};
}

std::optional<std::map<long long, expression_holder<scalar_expression>>>
polynomial_coefficients(expression_holder<scalar_expression> const &expr,
                        expression_holder<scalar_expression> const &x) {
  return polynomial_solver(expr, x).polynomial_coefficients();
}

solve_result
solve_with_outcome(expression_holder<scalar_expression> const &expr,
                   expression_holder<scalar_expression> const &x) {
  return polynomial_solver(expr, x).solve_with_outcome();
}

std::vector<expression_holder<scalar_expression>>
solve(expression_holder<scalar_expression> const &expr,
      expression_holder<scalar_expression> const &x) {
  return polynomial_solver(expr, x).solve();
}

} // namespace numsim::cas
