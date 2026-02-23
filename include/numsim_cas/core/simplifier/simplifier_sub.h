#ifndef SIMPLIFIER_SUB_H
#define SIMPLIFIER_SUB_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/domain_traits.h>
#include <numsim_cas/core/scalar_number.h>
#include <ranges>
#include <set>

namespace numsim::cas {
namespace detail {

//==============================================================================
// sub_dispatch<Traits, Derived> — Base algorithm for A - B
//==============================================================================
template <typename Traits, typename Derived = void>
requires arithmetic_expression_domain<typename Traits::expression_type>
class sub_dispatch {
public:
  using expr_holder_t = typename Traits::expr_holder_t;

  sub_dispatch(expr_holder_t lhs, expr_holder_t rhs)
      : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

  expr_holder_t get_default() {
    if (m_lhs.get().hash_value() == m_rhs.get().hash_value()) {
      return Traits::zero();
    }

    using add_type = typename Traits::add_type;

    const auto lhs_val{Traits::try_numeric(m_lhs)};
    const auto rhs_val{Traits::try_numeric(m_rhs)};

    // Both numeric: compute directly
    if (lhs_val && rhs_val) {
      auto result = *lhs_val - *rhs_val;
      if (result == scalar_number{0})
        return Traits::zero();
      return Traits::make_constant(result);
    }

    auto add_new{make_expression<add_type>()};
    auto &add{add_new.template get<add_type>()};
    if (lhs_val) {
      add.set_coeff(m_lhs);
    } else {
      add.push_back(m_lhs);
    }

    if (rhs_val) {
      add.set_coeff(negate_constant(m_rhs));
    } else {
      add.push_back(-m_rhs);
    }
    return add_new;
  }

  template <typename Expr> expr_holder_t dispatch(Expr const &) {
    return get_default();
  }

  // expr - 0 --> expr
  expr_holder_t dispatch(typename Traits::zero_type const &) { return m_lhs; }

protected:
  static expr_holder_t negate_constant(expr_holder_t const &c) {
    auto val = Traits::try_numeric(c);
    if (val)
      return Traits::make_constant(-(*val));
    return -c;
  }

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

//==============================================================================
// negative_sub_dispatch<Traits> — LHS is negative
//==============================================================================
template <typename Traits>
requires arithmetic_expression_domain<typename Traits::expression_type>
class negative_sub_dispatch
    : public sub_dispatch<Traits, negative_sub_dispatch<Traits>> {
  using base = sub_dispatch<Traits, negative_sub_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;

  negative_sub_dispatch(expr_holder_t lhs_in, expr_holder_t rhs)
      : base(std::move(lhs_in), std::move(rhs)),
        lhs{base::m_lhs.template get<typename Traits::negative_type>()} {}

  //-expr - (coeff + x) --> -(expr + coeff + x)
  expr_holder_t dispatch(typename Traits::add_type const &rhs) {
    auto add_expr{make_expression<typename Traits::add_type>(rhs)};
    auto &add{add_expr.template get<typename Traits::add_type>()};
    auto inner = lhs.expr();
    auto pos = add.symbol_map().find(inner);
    if (pos != add.symbol_map().end()) {
      auto combined = pos->second + inner;
      add.symbol_map().erase(pos);
      add.push_back(combined);
    } else {
      add.push_back(inner);
    }
    return make_expression<typename Traits::negative_type>(std::move(add_expr));
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  typename Traits::negative_type const &lhs;
};

//==============================================================================
// constant_sub_dispatch<Traits> — LHS is a constant
//==============================================================================
template <typename Traits>
requires arithmetic_expression_domain<typename Traits::expression_type>
class constant_sub_dispatch
    : public sub_dispatch<Traits, constant_sub_dispatch<Traits>> {
  using base = sub_dispatch<Traits, constant_sub_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;

  constant_sub_dispatch(expr_holder_t lhs, expr_holder_t rhs)
      : base(std::move(lhs), std::move(rhs)) {}

  // constant - constant
  expr_holder_t dispatch(typename Traits::constant_type const &) {
    auto lhs_val = Traits::try_numeric(base::m_lhs);
    auto rhs_val = Traits::try_numeric(base::m_rhs);
    if (lhs_val && rhs_val) {
      const auto value{*lhs_val - *rhs_val};
      if (value == scalar_number{0})
        return Traits::zero();
      return Traits::make_constant(value);
    }
    return base::get_default();
  }

  // constant - (coeff + x) --> (constant-coeff) + (-x)
  expr_holder_t dispatch([[maybe_unused]]
                         typename Traits::add_type const &rhs) {
    auto add_expr{make_expression<typename Traits::add_type>()};
    auto &add{add_expr.template get<typename Traits::add_type>()};
    auto coeff{base::m_lhs - rhs.coeff()};
    add.set_coeff(std::move(coeff));
    for (auto &child : rhs.symbol_map() | std::views::values) {
      add.push_back(-child);
    }
    return add_expr;
  }

  // constant - 1
  expr_holder_t dispatch(typename Traits::one_type const &) {
    auto lhs_val = Traits::try_numeric(base::m_lhs);
    if (lhs_val) {
      const auto value{*lhs_val - scalar_number{1}};
      if (value == scalar_number{0})
        return Traits::zero();
      return Traits::make_constant(value);
    }
    return base::get_default();
  }
};

//==============================================================================
// n_ary_sub_dispatch<Traits> — LHS is add (n-ary sum)
//==============================================================================
template <typename Traits>
requires arithmetic_expression_domain<typename Traits::expression_type>
class n_ary_sub_dispatch
    : public sub_dispatch<Traits, n_ary_sub_dispatch<Traits>> {
  using base = sub_dispatch<Traits, n_ary_sub_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;

  n_ary_sub_dispatch(expr_holder_t lhs_in, expr_holder_t rhs)
      : base(std::move(lhs_in), std::move(rhs)),
        lhs{base::m_lhs.template get<typename Traits::add_type>()} {}

  // (coeff + terms) - constant
  expr_holder_t dispatch([[maybe_unused]]
                         typename Traits::constant_type const &) {
    auto rhs_val = Traits::try_numeric(base::m_rhs);
    if (rhs_val) {
      auto add_expr{make_expression<typename Traits::add_type>(lhs)};
      auto &add{add_expr.template get<typename Traits::add_type>()};
      const auto value{get_coefficient<Traits>(lhs, 0) - *rhs_val};
      add.coeff().free();
      if (value != 0) {
        add.set_coeff(Traits::make_constant(value));
      }
      return add_expr;
    }
    return base::get_default();
  }

  // (coeff + terms) - 1
  expr_holder_t dispatch([[maybe_unused]] typename Traits::one_type const &) {
    auto add_expr{make_expression<typename Traits::add_type>(lhs)};
    auto &add{add_expr.template get<typename Traits::add_type>()};
    const auto value{get_coefficient<Traits>(add, 0) - 1};
    add.coeff().free();
    if (value != 0) {
      add.set_coeff(Traits::make_constant(value));
    }
    return add_expr;
  }

  // merge two add expressions
  expr_holder_t dispatch(typename Traits::add_type const &rhs) {
    auto expr{make_expression<typename Traits::add_type>()};
    auto &add{expr.template get<typename Traits::add_type>()};
    add.set_coeff(lhs.coeff() + rhs.coeff());
    std::set<expr_holder_t> used_expr;
    for (auto &child : lhs.symbol_map() | std::views::values) {
      auto pos{rhs.symbol_map().find(child)};
      if (pos != rhs.symbol_map().end()) {
        used_expr.insert(pos->second);
        add.push_back(child + pos->second);
      } else {
        add.push_back(child);
      }
    }
    if (used_expr.size() != rhs.size()) {
      for (auto &child : rhs.symbol_map() | std::views::values) {
        if (!used_expr.count(child)) {
          add.push_back(child);
        }
      }
    }
    return expr;
  }

  // x+y+z - x --> y+z  (symbol domains only)
  template <typename SymbolType = typename Traits::symbol_type>
  requires(!std::is_void_v<SymbolType>)
  expr_holder_t dispatch(SymbolType const &) {
    auto expr_add{make_expression<typename Traits::add_type>(lhs)};
    auto &add{expr_add.template get<typename Traits::add_type>()};
    auto pos{add.symbol_map().find(base::m_rhs)};
    if (pos != add.symbol_map().end()) {
      auto expr{pos->second - base::m_rhs};
      add.symbol_map().erase(pos);
      add.push_back(expr);
      return expr_add;
    }
    add.push_back(-base::m_rhs);
    return expr_add;
  }

protected:
  using base::m_lhs;
  using base::m_rhs;
  typename Traits::add_type const &lhs;
};

//==============================================================================
// n_ary_mul_sub_dispatch<Traits> — LHS is mul
//==============================================================================
template <typename Traits>
requires arithmetic_expression_domain<typename Traits::expression_type>
class n_ary_mul_sub_dispatch
    : public sub_dispatch<Traits, n_ary_mul_sub_dispatch<Traits>> {
  using base = sub_dispatch<Traits, n_ary_mul_sub_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;

  using base::get_default;

  n_ary_mul_sub_dispatch(expr_holder_t lhs_in, expr_holder_t rhs)
      : base(std::move(lhs_in), std::move(rhs)),
        lhs{base::m_lhs.template get<typename Traits::mul_type>()} {}

  // 2*x - x --> x  (symbol domains only)
  template <typename SymbolType = typename Traits::symbol_type>
  requires(!std::is_void_v<SymbolType>)
  expr_holder_t dispatch(SymbolType const &) {
    if (lhs.symbol_map().size() == 1) {
      auto pos{lhs.symbol_map().find(base::m_rhs)};
      if (lhs.symbol_map().end() != pos) {
        const auto value{get_coefficient<Traits>(lhs, 0) - 1};
        if (value == 0) {
          return Traits::zero();
        }

        if (value == 1) {
          return base::m_rhs;
        }

        return Traits::make_constant(value) * base::m_rhs;
      }
    }

    return get_default();
  }

  /// c1*expr - c2*expr --> (c1-c2)*expr
  expr_holder_t dispatch(typename Traits::mul_type const &rhs) {
    const auto &hash_rhs{rhs.hash_value()};
    const auto &hash_lhs{lhs.hash_value()};
    if (hash_rhs == hash_lhs) {
      const auto fac_lhs{get_coefficient<Traits>(lhs, 1.0)};
      const auto fac_rhs{get_coefficient<Traits>(rhs, 1.0)};
      const auto result{fac_lhs - fac_rhs};
      if (result == scalar_number{0})
        return Traits::zero();
      const auto abs_result{result.abs()};
      if (abs_result == scalar_number{1} && lhs.size() == 1) {
        auto child = lhs.symbol_map().begin()->second;
        if (result < 0)
          return make_expression<typename Traits::negative_type>(
              std::move(child));
        return child;
      }
      auto expr{make_expression<typename Traits::mul_type>(lhs)};
      auto &mul{expr.template get<typename Traits::mul_type>()};
      mul.set_coeff(Traits::make_constant(abs_result));
      if (result < 0) {
        return make_expression<typename Traits::negative_type>(std::move(expr));
      }
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
// symbol_sub_dispatch<Traits> — LHS is a symbol (only for non-void symbol_type)
//==============================================================================
template <typename Traits>
requires arithmetic_expression_domain<typename Traits::expression_type>
class symbol_sub_dispatch
    : public sub_dispatch<Traits, symbol_sub_dispatch<Traits>> {
  using base = sub_dispatch<Traits, symbol_sub_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;

  using base::get_default;

  symbol_sub_dispatch(expr_holder_t lhs_in, expr_holder_t rhs)
      : base(std::move(lhs_in), std::move(rhs)),
        lhs{base::m_lhs.template get<typename Traits::symbol_type>()} {}

  /// x-x --> 0
  expr_holder_t dispatch(typename Traits::symbol_type const &rhs) {
    if (&lhs == &rhs) {
      return Traits::zero();
    }
    return get_default();
  }

  // x - 3*x --> -(2*x)
  expr_holder_t dispatch(typename Traits::mul_type const &rhs) {
    const auto &hash_rhs{rhs.hash_value()};
    const auto &hash_lhs{lhs.hash_value()};
    if (hash_rhs == hash_lhs) {
      auto expr{make_expression<typename Traits::mul_type>(rhs)};
      auto &mul{expr.template get<typename Traits::mul_type>()};
      const auto value{1.0 - get_coefficient<Traits>(rhs, 1.0)};
      mul.set_coeff(Traits::make_constant(value.abs()));
      if (value < 0) {
        return make_expression<typename Traits::negative_type>(std::move(expr));
      } else {
        return expr;
      }
    }
    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  typename Traits::symbol_type const &lhs;
};

//==============================================================================
// one_sub_dispatch<Traits> — LHS is one
//==============================================================================
template <typename Traits>
requires arithmetic_expression_domain<typename Traits::expression_type>
class one_sub_dispatch : public sub_dispatch<Traits, one_sub_dispatch<Traits>> {
  using base = sub_dispatch<Traits, one_sub_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;

  using base::get_default;

  one_sub_dispatch(expr_holder_t lhs, expr_holder_t rhs)
      : base(std::move(lhs), std::move(rhs)) {}

  // 1 - constant
  expr_holder_t dispatch(typename Traits::constant_type const &) {
    auto rhs_val = Traits::try_numeric(base::m_rhs);
    if (rhs_val) {
      auto value = scalar_number{1} - *rhs_val;
      if (value == scalar_number{0})
        return Traits::zero();
      return Traits::make_constant(value);
    }
    return get_default();
  }
};

} // namespace detail
} // namespace numsim::cas

#endif // SIMPLIFIER_SUB_H
