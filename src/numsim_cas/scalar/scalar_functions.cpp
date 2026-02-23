#include <numsim_cas/scalar/scalar_definitions.h>
#include <numsim_cas/scalar/scalar_functions.h>
#include <numsim_cas/scalar/scalar_operators.h>

namespace numsim::cas {

bool is_scalar_constant(
    expression_holder<scalar_expression> const &expr) noexcept {
  if (is_same<scalar_constant>(expr) || is_same<scalar_zero>(expr) ||
      is_same<scalar_one>(expr)) {
    return true;
  }
  return false;
}

std::optional<scalar_number>
get_scalar_number(expression_holder<scalar_expression> const &expr) {
  if (is_same<scalar_constant>(expr)) {
    return expr.get<scalar_constant>().value();
  }
  if (is_same<scalar_one>(expr)) {
    return scalar_number{1};
  }
  if (is_same<scalar_zero>(expr)) {
    return scalar_number{0};
  }
  return {};
}

// Returns integer value if the expression is exactly an integer constant.
// Supports: scalar_zero, scalar_one, scalar_constant(k),
// scalar_negative(constant/one/zero).
std::optional<long long>
try_int_constant(expression_holder<scalar_expression> const &e) {
  if (!e.is_valid())
    return std::nullopt;

  if (is_same<scalar_zero>(e))
    return 0LL;
  if (is_same<scalar_one>(e))
    return 1LL;

  if (is_same<scalar_negative>(e)) {
    auto const &inner = e.template get<scalar_negative>().expr();
    if (auto v = try_int_constant(inner))
      return -(*v);
    return std::nullopt;
  }

  if (!is_same<scalar_constant>(e))
    return std::nullopt;

  auto const &sn = e.template get<scalar_constant>().value();
  long long out = 0;
  bool ok = false;

  std::visit(
      [&](auto const &x) {
        using X = std::decay_t<decltype(x)>;
        if constexpr (std::is_integral_v<X>) {
          out = static_cast<long long>(x);
          ok = true;
        } else if constexpr (std::is_floating_point_v<X>) {
          // accept only exact integers (no eps)
          auto r = std::llround(x);
          if (static_cast<X>(r) == x) {
            out = r;
            ok = true;
          }
        } else {
          ok = false; // complex etc.
        }
      },
      sn.raw());

  if (!ok)
    return std::nullopt;
  return out;
}

bool is_integer_exponent(expression_holder<scalar_expression> const &exp) {
  return try_int_constant(exp).has_value();
}

// Combines:
//  1) pow(x,a) * pow(x,b) -> pow(x,a+b)
//  2) pow(x,a) * pow(y,a) -> pow(x*y,a)   (only if a is integer)
std::optional<expression_holder<scalar_expression>>
simplify_scalar_pow_pow_mul(scalar_pow const &lhs, scalar_pow const &rhs) {
  auto const &x = lhs.expr_lhs();
  auto const &a = lhs.expr_rhs();
  auto const &y = rhs.expr_lhs();
  auto const &b = rhs.expr_rhs();

  // same base: x^a * x^b -> x^(a+b)
  if (x == y) {
    return pow(x, a + b);
  }

  // assuming x and y are positive
  // same exponent: x^a * y^a -> (x*y)^a  (guarded!)
  if (a == b && is_integer_exponent(a)) {
    return pow(x * y, a);
  }

  return std::nullopt;
}

bool is_constant(expression_holder<scalar_expression> const &expr) {
  if (is_same<scalar_negative>(expr)) {
    const auto &neg_expr{expr.get<scalar_negative>().expr()};
    return is_same<scalar_one>(neg_expr) || is_same<scalar_constant>(neg_expr);
  }
  return is_same<scalar_one>(expr) || is_same<scalar_constant>(expr);
}

} // namespace numsim::cas
