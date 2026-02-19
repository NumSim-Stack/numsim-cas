#ifndef SIMPLIFIER_ADD_H
#define SIMPLIFIER_ADD_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/scalar_number.h>
#include <numsim_cas/functions.h>
#include <ranges>
#include <set>

namespace numsim::cas {
namespace detail {

//==============================================================================
// add_dispatch<Traits, Derived> — Base algorithm for A + B
//==============================================================================
template <typename Traits, typename Derived = void> class add_dispatch {
public:
  using expr_holder_t = typename Traits::expr_holder_t;

  add_dispatch(expr_holder_t lhs, expr_holder_t rhs)
      : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

  expr_holder_t get_default() {
    if constexpr (!std::is_void_v<typename Traits::mul_type>) {
      using add_type = typename Traits::add_type;
      using mul_type = typename Traits::mul_type;

      const auto lhs_numeric{is_numeric_expr(m_lhs)};
      const auto rhs_numeric{is_numeric_expr(m_rhs)};

      // Both numeric: combine directly (must precede same-expression check)
      if (lhs_numeric && rhs_numeric) {
        auto lhs_val = Traits::try_numeric(m_lhs);
        auto rhs_val = Traits::try_numeric(m_rhs);
        auto sum = *lhs_val + *rhs_val;
        if (sum == scalar_number{0})
          return Traits::zero(m_lhs);
        return Traits::make_constant(sum);
      }

      // Same expression: expr + expr → 2*expr
      if (m_lhs == m_rhs) {
        auto mul_expr{make_expression<mul_type>()};
        auto &m{mul_expr.template get<mul_type>()};
        m.set_coeff(Traits::make_constant(scalar_number(2)));
        m.push_back(m_rhs);
        return mul_expr;
      }

      auto add_new{make_expression<add_type>()};
      auto &add{add_new.template get<add_type>()};
      if (lhs_numeric) {
        add.set_coeff(m_lhs);
      } else {
        add.push_back(m_lhs);
      }
      if (rhs_numeric) {
        add.set_coeff(m_rhs);
      } else {
        add.push_back(m_rhs);
      }
      return add_new;
    } else {
      // Domains without n_ary_tree mul_type (e.g. tensor) override get_default()
      return m_lhs;
    }
  }

  static bool is_numeric_expr(expr_holder_t const &expr) {
    return Traits::try_numeric(expr).has_value();
  }

  template <typename Expr> expr_holder_t dispatch(Expr const &) {
    return get_default();
  }

  // expr + 0 --> expr
  expr_holder_t dispatch(typename Traits::zero_type const &) { return m_lhs; }

  // expr + (-expr) --> 0
  expr_holder_t dispatch(typename Traits::negative_type const &neg) {
    if (m_lhs == neg.expr()) {
      return Traits::zero(m_lhs);
    }
    return get_default();
  }

  // non-add + add --> swap so add is LHS (triggers n_ary_add_dispatch)
  expr_holder_t dispatch(typename Traits::add_type const &) {
    if constexpr (!std::is_void_v<typename Traits::mul_type>) {
      return m_rhs + m_lhs;
    } else {
      return get_default();
    }
  }

  template <typename ExprT, typename ValueTypeT>
  scalar_number get_coefficient(ExprT const &expr, ValueTypeT const &value) {
    if constexpr (is_detected_v<has_coefficient, ExprT>) {
      auto const &coeff = expr.coeff();
      if (coeff.is_valid()) {
        auto val = Traits::try_numeric(coeff);
        if (val)
          return *val;
      }
      return value;
    }
    return value;
  }

protected:
  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

//==============================================================================
// constant_add_dispatch<Traits> — LHS is constant
//==============================================================================
template <typename Traits>
class constant_add_dispatch
    : public add_dispatch<Traits, constant_add_dispatch<Traits>> {
  using base = add_dispatch<Traits, constant_add_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  constant_add_dispatch(expr_holder_t lhs, expr_holder_t rhs)
      : base(std::move(lhs), std::move(rhs)) {}

  // constant + constant
  expr_holder_t dispatch(typename Traits::constant_type const &) {
    auto lhs_val = Traits::try_numeric(base::m_lhs);
    auto rhs_val = Traits::try_numeric(base::m_rhs);
    if (lhs_val && rhs_val) {
      const auto value{*lhs_val + *rhs_val};
      if (value == scalar_number{0})
        return Traits::zero();
      return Traits::make_constant(value);
    }
    return base::get_default();
  }

  // constant + (coeff + x)
  expr_holder_t
  dispatch([[maybe_unused]] typename Traits::add_type const &rhs) {
    auto lhs_val = Traits::try_numeric(base::m_lhs);
    if (lhs_val) {
      const auto value{base::get_coefficient(rhs, 0) + *lhs_val};
      if (value != 0) {
        auto add_expr{make_expression<typename Traits::add_type>(rhs)};
        auto &add{add_expr.template get<typename Traits::add_type>()};
        add.set_coeff(Traits::make_constant(value));
        return add_expr;
      }
      return base::m_rhs;
    }
    return base::get_default();
  }

  // constant + 1
  expr_holder_t dispatch(typename Traits::one_type const &) {
    auto lhs_val = Traits::try_numeric(base::m_lhs);
    if (lhs_val) {
      const auto value{*lhs_val + scalar_number{1}};
      if (value == scalar_number{0})
        return Traits::zero();
      return Traits::make_constant(value);
    }
    return base::get_default();
  }

  // constant + (-expr)
  expr_holder_t dispatch(typename Traits::negative_type const &) {
    auto lhs_val = Traits::try_numeric(base::m_lhs);
    auto rhs_val = Traits::try_numeric(base::m_rhs);
    if (lhs_val && rhs_val) {
      auto value = *lhs_val + *rhs_val;
      if (value == scalar_number{0})
        return Traits::zero();
      return Traits::make_constant(value);
    }
    return base::get_default();
  }
};

//==============================================================================
// one_add_dispatch<Traits> — LHS is one
//==============================================================================
template <typename Traits>
class one_add_dispatch
    : public add_dispatch<Traits, one_add_dispatch<Traits>> {
  using base = add_dispatch<Traits, one_add_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  one_add_dispatch(expr_holder_t lhs, expr_holder_t rhs)
      : base(std::move(lhs), std::move(rhs)) {}

  // 1 + constant
  expr_holder_t dispatch(typename Traits::constant_type const &) {
    auto rhs_val = Traits::try_numeric(base::m_rhs);
    if (rhs_val) {
      const auto value{scalar_number{1} + *rhs_val};
      if (value == scalar_number{0})
        return Traits::zero();
      return Traits::make_constant(value);
    }
    return base::get_default();
  }

  // 1 + (coeff + x)
  expr_holder_t
  dispatch([[maybe_unused]] typename Traits::add_type const &rhs) {
    const auto value{base::get_coefficient(rhs, 0) + 1};
    if (value != 0) {
      auto add_expr{make_expression<typename Traits::add_type>(rhs)};
      auto &add{add_expr.template get<typename Traits::add_type>()};
      add.set_coeff(Traits::make_constant(value));
      return add_expr;
    }
    return base::m_rhs;
  }

  // 1 + 1
  expr_holder_t dispatch(typename Traits::one_type const &) {
    return Traits::make_constant(scalar_number{1} + scalar_number{1});
  }

  // 1 + (-expr)
  expr_holder_t dispatch(typename Traits::negative_type const &) {
    auto rhs_val = Traits::try_numeric(base::m_rhs);
    if (rhs_val) {
      auto value = scalar_number{1} + *rhs_val;
      if (value == scalar_number{0})
        return Traits::zero();
      return Traits::make_constant(value);
    }
    return base::get_default();
  }
};

//==============================================================================
// n_ary_add_dispatch<Traits> — LHS is add
//==============================================================================
template <typename Traits>
class n_ary_add_dispatch
    : public add_dispatch<Traits, n_ary_add_dispatch<Traits>> {
  using base = add_dispatch<Traits, n_ary_add_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  n_ary_add_dispatch(expr_holder_t lhs, expr_holder_t rhs)
      : base(std::move(lhs), std::move(rhs)),
        lhs{base::m_lhs.template get<typename Traits::add_type>()} {}

  // (coeff + terms) + constant
  expr_holder_t
  dispatch([[maybe_unused]] typename Traits::constant_type const &) {
    auto rhs_val = Traits::try_numeric(base::m_rhs);
    if (rhs_val) {
      auto add_expr{make_expression<typename Traits::add_type>(lhs)};
      auto &add{add_expr.template get<typename Traits::add_type>()};
      const auto value{base::get_coefficient(lhs, 0) + *rhs_val};
      add.coeff().free();
      if (value != 0) {
        add.set_coeff(Traits::make_constant(value));
      }
      return add_expr;
    }
    return base::get_default();
  }

  // (coeff + terms) + 1
  expr_holder_t
  dispatch([[maybe_unused]] typename Traits::one_type const &) {
    auto add_expr{make_expression<typename Traits::add_type>(lhs)};
    auto &add{add_expr.template get<typename Traits::add_type>()};
    const auto value{base::get_coefficient(add, 0) + 1};
    add.coeff().free();
    if (value != 0) {
      add.set_coeff(Traits::make_constant(value));
    }
    return add_expr;
  }

  // x+y+z + x --> 2*x+y+z  (symbol domains only)
  template <typename SymbolType = typename Traits::symbol_type>
  requires(!std::is_void_v<SymbolType>)
  expr_holder_t dispatch(SymbolType const &) {
    auto expr_add{make_expression<typename Traits::add_type>(lhs)};
    auto &add{expr_add.template get<typename Traits::add_type>()};
    auto pos{add.hash_map().find(base::m_rhs)};
    if (pos != add.hash_map().end()) {
      auto expr{pos->second + base::m_rhs};
      add.hash_map().erase(pos);
      add.push_back(std::move(expr));
      return expr_add;
    }
    add.push_back(base::m_rhs);
    return expr_add;
  }

  // merge two add expressions
  expr_holder_t dispatch(typename Traits::add_type const &rhs) {
    auto expr{make_expression<typename Traits::add_type>()};
    auto &add{expr.template get<typename Traits::add_type>()};
    merge_add(lhs, rhs, add);
    return expr;
  }

  // (coeff + terms) + (-expr)
  expr_holder_t dispatch(typename Traits::negative_type const &rhs) {
    const auto pos{lhs.hash_map().find(rhs.expr())};
    if (pos != lhs.hash_map().end()) {
      auto expr{make_expression<typename Traits::add_type>(lhs)};
      auto &add{expr.template get<typename Traits::add_type>()};
      add.hash_map().erase(rhs.expr());
      return expr;
    }

    auto inner_val = Traits::try_numeric(rhs.expr());
    if (inner_val) {
      auto add_expr{make_expression<typename Traits::add_type>(lhs)};
      auto &add{add_expr.template get<typename Traits::add_type>()};
      const auto value{base::get_coefficient(lhs, 0) - *inner_val};
      add.coeff().free();
      if (value != 0) {
        add.set_coeff(Traits::make_constant(value));
      }
      return add_expr;
    }

    return base::get_default();
  }

protected:
  using base::m_lhs;
  using base::m_rhs;
  typename Traits::add_type const &lhs;
};

//==============================================================================
// n_ary_mul_add_dispatch<Traits> — LHS is mul
//==============================================================================
template <typename Traits>
class n_ary_mul_add_dispatch
    : public add_dispatch<Traits, n_ary_mul_add_dispatch<Traits>> {
  using base = add_dispatch<Traits, n_ary_mul_add_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  n_ary_mul_add_dispatch(expr_holder_t lhs, expr_holder_t rhs)
      : base(std::move(lhs), std::move(rhs)),
        lhs{base::m_lhs.template get<typename Traits::mul_type>()} {}

  // constant*expr + expr --> (constant+1)*expr  (symbol domains only)
  template <typename SymbolType = typename Traits::symbol_type>
  requires(!std::is_void_v<SymbolType>)
  expr_holder_t dispatch(SymbolType const &) {
    const auto pos{lhs.hash_map().find(base::m_rhs)};
    if (pos != lhs.hash_map().end() && lhs.hash_map().size() == 1) {
      auto expr{make_expression<typename Traits::mul_type>(lhs)};
      auto &mul{expr.template get<typename Traits::mul_type>()};
      mul.set_coeff(Traits::make_constant(
          base::get_coefficient(lhs, 1) + 1));
      return expr;
    }
    return get_default();
  }

  /// expr + expr --> 2*expr
  expr_holder_t dispatch(typename Traits::mul_type const &rhs) {
    const auto &hash_rhs{rhs.hash_value()};
    const auto &hash_lhs{lhs.hash_value()};
    if (hash_rhs == hash_lhs) {
      const auto fac_lhs{base::get_coefficient(lhs, 1)};
      const auto fac_rhs{base::get_coefficient(rhs, 1)};
      auto expr{make_expression<typename Traits::mul_type>(lhs)};
      auto &mul{expr.template get<typename Traits::mul_type>()};
      mul.set_coeff(Traits::make_constant(fac_lhs + fac_rhs));
      return expr;
    }
    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  typename Traits::mul_type const &lhs;
};

//==============================================================================
// symbol_add_dispatch<Traits> — LHS is symbol (only for non-void symbol_type)
//==============================================================================
template <typename Traits>
class symbol_add_dispatch
    : public add_dispatch<Traits, symbol_add_dispatch<Traits>> {
  using base = add_dispatch<Traits, symbol_add_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  symbol_add_dispatch(expr_holder_t lhs, expr_holder_t rhs)
      : base(std::move(lhs), std::move(rhs)),
        lhs{base::m_lhs.template get<typename Traits::symbol_type>()} {}

  /// x+x --> 2*x
  expr_holder_t dispatch(typename Traits::symbol_type const &rhs) {
    if (lhs == rhs) {
      auto mul{make_expression<typename Traits::mul_type>()};
      mul.template get<typename Traits::mul_type>().set_coeff(
          Traits::make_constant(scalar_number{2}));
      mul.template get<typename Traits::mul_type>().push_back(base::m_rhs);
      return mul;
    }
    return get_default();
  }

  // x + c*x --> (c+1)*x
  expr_holder_t dispatch(typename Traits::mul_type const &rhs) {
    const auto pos{rhs.hash_map().find(base::m_lhs)};
    if (pos != rhs.hash_map().end() && rhs.hash_map().size() == 1) {
      auto expr{make_expression<typename Traits::mul_type>(rhs)};
      auto &mul{expr.template get<typename Traits::mul_type>()};
      mul.set_coeff(Traits::make_constant(
          base::get_coefficient(rhs, 1) + 1));
      return expr;
    }
    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  typename Traits::symbol_type const &lhs;
};

//==============================================================================
// negative_add_dispatch<Traits> — LHS is negative
//==============================================================================
template <typename Traits>
class negative_add_dispatch
    : public add_dispatch<Traits, negative_add_dispatch<Traits>> {
  using base = add_dispatch<Traits, negative_add_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  negative_add_dispatch(expr_holder_t lhs, expr_holder_t rhs)
      : base(std::move(lhs), std::move(rhs)),
        lhs{base::m_lhs.template get<typename Traits::negative_type>()} {}

  // (-lhs) + (-rhs) --> -(lhs+rhs)
  expr_holder_t dispatch(typename Traits::negative_type const &rhs) {
    return -(lhs.expr() + rhs.expr());
  }

  // -expr + (coeff + terms)
  expr_holder_t
  dispatch([[maybe_unused]] typename Traits::add_type const &rhs) {
    auto add_expr{make_expression<typename Traits::add_type>(rhs)};
    auto &add{add_expr.template get<typename Traits::add_type>()};

    auto inner_val = Traits::try_numeric(lhs.expr());
    if (inner_val) {
      const auto value{base::get_coefficient(rhs, 0) - *inner_val};
      add.coeff().free();
      if (value != 0) {
        add.set_coeff(Traits::make_constant(value));
      }
      return add_expr;
    }
    add.push_back(base::m_lhs);
    return add_expr;
  }

  // -expr + c
  expr_holder_t dispatch(typename Traits::constant_type const &) {
    auto lhs_val = Traits::try_numeric(base::m_lhs);
    auto rhs_val = Traits::try_numeric(base::m_rhs);
    if (lhs_val && rhs_val) {
      auto value = *lhs_val + *rhs_val;
      if (value == scalar_number{0})
        return Traits::zero();
      return Traits::make_constant(value);
    }
    return base::get_default();
  }

private:
  typename Traits::negative_type const &lhs;
};

} // namespace detail
} // namespace numsim::cas

#endif // SIMPLIFIER_ADD_H
