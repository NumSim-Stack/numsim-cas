#ifndef NUMSIM_CAS_PARSER_FUNCTION_REGISTRY_H
#define NUMSIM_CAS_PARSER_FUNCTION_REGISTRY_H

// Static registry mapping function names (as parsed strings) to
// arity + dispatch callable. SRC-PRIVATE.
//
// Phase 2c registers scalar→scalar functions that are CURRENTLY on
// `main`: trig family, hyperbolic family, exp/log/sqrt/abs/sign,
// pow, and the six comparison functions. Notably absent for now:
// max, min, if_then_else, macauley_plus/minus, heaviside,
// smoothed_macauley — these live on unmerged PRs (#207, #208, #209).
// Once those PRs land on main this registry should be extended
// (one entry per function in the same pattern). Until then, calls
// to those names will fire `unknown_function_error` — which is
// exactly the right surface: the parser fails fast on functions
// the library can't yet build.
//
// Phase 2d will add tensor→tensor and tensor→t2s functions in
// either this registry or a sibling.

#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace numsim::cas::parser::registry {

using scalar_expr = expression_holder<scalar_expression>;
using arg_vec = std::vector<scalar_expr>;

struct function_entry {
  std::size_t arity;
  std::function<scalar_expr(arg_vec)> dispatch;
};

namespace detail {

inline function_entry unary(auto fn) {
  return {1, [fn = std::move(fn)](arg_vec a) -> scalar_expr {
            return fn(std::move(a[0]));
          }};
}
inline function_entry binary(auto fn) {
  return {2, [fn = std::move(fn)](arg_vec a) -> scalar_expr {
            return fn(std::move(a[0]), std::move(a[1]));
          }};
}
inline function_entry ternary(auto fn) {
  return {3, [fn = std::move(fn)](arg_vec a) -> scalar_expr {
            return fn(std::move(a[0]), std::move(a[1]), std::move(a[2]));
          }};
}

} // namespace detail

// Function-name → (arity, dispatch) lookup. Returns a reference to a
// process-wide static — registry is read-only after first call.
inline std::unordered_map<std::string, function_entry> const &
scalar_function_registry() {
  static auto const r = [] {
    std::unordered_map<std::string, function_entry> m;
    using detail::binary;
    using detail::ternary;
    using detail::unary;

    // Trig
    m.emplace("sin", unary([](auto x) { return sin(x); }));
    m.emplace("cos", unary([](auto x) { return cos(x); }));
    m.emplace("tan", unary([](auto x) { return tan(x); }));
    m.emplace("asin", unary([](auto x) { return asin(x); }));
    m.emplace("acos", unary([](auto x) { return acos(x); }));
    m.emplace("atan", unary([](auto x) { return atan(x); }));

    // Hyperbolic
    m.emplace("sinh", unary([](auto x) { return sinh(x); }));
    m.emplace("cosh", unary([](auto x) { return cosh(x); }));
    m.emplace("tanh", unary([](auto x) { return tanh(x); }));
    m.emplace("asinh", unary([](auto x) { return asinh(x); }));
    m.emplace("acosh", unary([](auto x) { return acosh(x); }));
    m.emplace("atanh", unary([](auto x) { return atanh(x); }));

    // Exp / log / sqrt
    m.emplace("exp", unary([](auto x) { return exp(x); }));
    m.emplace("log", unary([](auto x) { return log(x); }));
    m.emplace("log10", unary([](auto x) { return log10(x); }));
    m.emplace("sqrt", unary([](auto x) { return sqrt(x); }));

    // Algebra
    m.emplace("abs", unary([](auto x) { return abs(x); }));
    m.emplace("sign", unary([](auto x) { return sign(x); }));

    // Binary
    m.emplace("pow", binary([](auto a, auto b) { return pow(a, b); }));

    // Comparison functions (also reachable via the comparison operators
    // from phase 2b; allowing the function form lets users be explicit
    // when chaining, e.g. `eq(x, y) * weight`).
    m.emplace("lt", binary([](auto a, auto b) { return lt(a, b); }));
    m.emplace("le", binary([](auto a, auto b) { return le(a, b); }));
    m.emplace("gt", binary([](auto a, auto b) { return gt(a, b); }));
    m.emplace("ge", binary([](auto a, auto b) { return ge(a, b); }));
    m.emplace("eq", binary([](auto a, auto b) { return eq(a, b); }));
    m.emplace("ne", binary([](auto a, auto b) { return ne(a, b); }));

    // NOTE: max, min, if_then_else, macauley_plus/minus, heaviside,
    // smoothed_macauley intentionally absent — see the file header.
    // They land here once #207/#208/#209 merge.

    return m;
  }();
  return r;
}

} // namespace numsim::cas::parser::registry

#endif // NUMSIM_CAS_PARSER_FUNCTION_REGISTRY_H
