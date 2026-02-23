#include <numsim_cas/scalar/scalar_solve.h>

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/scalar/scalar_all.h>
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

std::vector<polynomial_solver::expr_holder_t> polynomial_solver::solve() const {
  auto coeffs_opt = classify_term(m_expr);
  if (!coeffs_opt)
    return {};

  auto &coeffs = *coeffs_opt;

  // Remove zero-valued coefficients
  for (auto it = coeffs.begin(); it != coeffs.end();) {
    if (is_numeric_zero(it->second)) {
      it = coeffs.erase(it);
    } else {
      ++it;
    }
  }

  if (coeffs.empty())
    return {}; // 0 == 0, infinitely many solutions — return empty

  long long max_degree = coeffs.rbegin()->first;

  if (max_degree == 0) {
    // Constant equation, no variable present
    return {};
  }

  auto get_coeff = [&](long long deg) -> expr_holder_t {
    auto it = coeffs.find(deg);
    if (it == coeffs.end())
      return get_scalar_zero();
    return it->second;
  };

  if (max_degree == 1) {
    // a*x + b = 0 → x = -b/a
    auto a = get_coeff(1);
    auto b = get_coeff(0);
    return {-b / a};
  }

  if (max_degree == 2) {
    // a*x² + b*x + c = 0
    auto a = get_coeff(2);
    auto b = get_coeff(1);
    auto c = get_coeff(0);

    auto two = get_scalar_one() + get_scalar_one();
    auto four = two + two;

    auto disc = b * b - four * a * c;

    // Check if discriminant is numerically zero
    if (is_numeric_zero(disc)) {
      // Double root: -b / (2a)
      return {-b / (two * a)};
    }

    auto sqrt_disc = sqrt(disc);
    auto two_a = two * a;
    return {(-b + sqrt_disc) / two_a, (-b - sqrt_disc) / two_a};
  }

  // Degree > 2: not supported
  return {};
}

std::optional<std::map<long long, expression_holder<scalar_expression>>>
polynomial_coefficients(expression_holder<scalar_expression> const &expr,
                        expression_holder<scalar_expression> const &x) {
  return polynomial_solver(expr, x).polynomial_coefficients();
}

std::vector<expression_holder<scalar_expression>>
solve(expression_holder<scalar_expression> const &expr,
      expression_holder<scalar_expression> const &x) {
  return polynomial_solver(expr, x).solve();
}

} // namespace numsim::cas
