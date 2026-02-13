#ifndef SIMPLIFIER_SUB_H
#define SIMPLIFIER_SUB_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/scalar_number.h>
#include <ranges>
#include <set>

namespace numsim::cas {
namespace detail {

//==============================================================================
// sub_dispatch<Traits, Derived> — Base algorithm for A - B
//==============================================================================
template <typename Traits, typename Derived = void> class sub_dispatch {
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

  template <typename _Expr, typename _ValueType>
  scalar_number get_coefficient(_Expr const &expr, _ValueType const &value) {
    if constexpr (is_detected_v<has_coefficient, _Expr>) {
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
class negative_sub_dispatch
    : public sub_dispatch<Traits, negative_sub_dispatch<Traits>> {
  using base = sub_dispatch<Traits, negative_sub_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;
  using base::get_coefficient;

  negative_sub_dispatch(expr_holder_t lhs, expr_holder_t rhs)
      : base(std::move(lhs), std::move(rhs)),
        lhs{base::m_lhs.template get<typename Traits::negative_type>()} {}

  //-expr - (coeff + x) --> -(expr + coeff + x)
  expr_holder_t dispatch(typename Traits::add_type const &rhs) {
    auto add_expr{make_expression<typename Traits::add_type>(rhs)};
    auto &add{add_expr.template get<typename Traits::add_type>()};
    auto coeff{base::m_lhs + add.coeff()};
    add.set_coeff(std::move(coeff));
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
class constant_sub_dispatch
    : public sub_dispatch<Traits, constant_sub_dispatch<Traits>> {
  using base = sub_dispatch<Traits, constant_sub_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;
  using base::get_coefficient;

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

  // constant - (coeff + x)
  expr_holder_t
  dispatch([[maybe_unused]] typename Traits::add_type const &rhs) {
    auto add_expr{make_expression<typename Traits::add_type>(rhs)};
    auto &add{add_expr.template get<typename Traits::add_type>()};
    auto coeff{base::m_lhs - add.coeff()};
    add.set_coeff(std::move(coeff));
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
class n_ary_sub_dispatch
    : public sub_dispatch<Traits, n_ary_sub_dispatch<Traits>> {
  using base = sub_dispatch<Traits, n_ary_sub_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;
  using base::get_coefficient;

  n_ary_sub_dispatch(expr_holder_t lhs, expr_holder_t rhs)
      : base(std::move(lhs), std::move(rhs)),
        lhs{base::m_lhs.template get<typename Traits::add_type>()} {}

  // (coeff + terms) - constant
  expr_holder_t
  dispatch([[maybe_unused]] typename Traits::constant_type const &) {
    auto rhs_val = Traits::try_numeric(base::m_rhs);
    if (rhs_val) {
      auto add_expr{make_expression<typename Traits::add_type>(lhs)};
      auto &add{add_expr.template get<typename Traits::add_type>()};
      const auto value{base::get_coefficient(lhs, 0) - *rhs_val};
      add.coeff().free();
      if (value != 0) {
        add.set_coeff(Traits::make_constant(value));
      }
      return add_expr;
    }
    return base::get_default();
  }

  // (coeff + terms) - 1
  expr_holder_t
  dispatch([[maybe_unused]] typename Traits::one_type const &) {
    auto add_expr{make_expression<typename Traits::add_type>(lhs)};
    auto &add{add_expr.template get<typename Traits::add_type>()};
    const auto value{base::get_coefficient(add, 0) - 1};
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
    for (auto &child : lhs.hash_map() | std::views::values) {
      auto pos{rhs.hash_map().find(child)};
      if (pos != rhs.hash_map().end()) {
        used_expr.insert(pos->second);
        add.push_back(child + pos->second);
      } else {
        add.push_back(child);
      }
    }
    if (used_expr.size() != rhs.size()) {
      for (auto &child : rhs.hash_map() | std::views::values) {
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
    auto pos{add.hash_map().find(base::m_rhs)};
    if (pos != add.hash_map().end()) {
      auto expr{pos->second - base::m_rhs};
      add.hash_map().erase(pos);
      add.push_back(expr);
      return expr_add;
    }
    add.push_back(-base::m_rhs);
    return expr_add;
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  typename Traits::add_type const &lhs;
};

//==============================================================================
// n_ary_mul_sub_dispatch<Traits> — LHS is mul
//==============================================================================
template <typename Traits>
class n_ary_mul_sub_dispatch
    : public sub_dispatch<Traits, n_ary_mul_sub_dispatch<Traits>> {
  using base = sub_dispatch<Traits, n_ary_mul_sub_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  n_ary_mul_sub_dispatch(expr_holder_t lhs, expr_holder_t rhs)
      : base(std::move(lhs), std::move(rhs)),
        lhs{base::m_lhs.template get<typename Traits::mul_type>()} {}

  // 2*x - x --> x  (symbol domains only)
  template <typename SymbolType = typename Traits::symbol_type>
  requires(!std::is_void_v<SymbolType>)
  expr_holder_t dispatch(SymbolType const &) {
    if (lhs.hash_map().size() == 1) {
      auto pos{lhs.hash_map().find(base::m_rhs)};
      if (lhs.hash_map().end() != pos) {
        const auto value{base::get_coefficient(lhs, 0) - 1};
        if (value == 0) {
          return Traits::zero();
        }

        if (value == 1) {
          return base::m_rhs;
        }

        return make_expression<typename Traits::constant_type>(value) *
               base::m_rhs;
      }
    }

    return get_default();
  }

  /// expr + expr --> 2*expr
  expr_holder_t dispatch(typename Traits::mul_type const &rhs) {
    const auto &hash_rhs{rhs.hash_value()};
    const auto &hash_lhs{lhs.hash_value()};
    if (hash_rhs == hash_lhs) {
      const auto fac_lhs{base::get_coefficient(lhs, 1.0)};
      const auto fac_rhs{base::get_coefficient(rhs, 1.0)};
      auto expr{make_expression<typename Traits::mul_type>(lhs)};
      auto &mul{expr.template get<typename Traits::mul_type>()};
      mul.set_coeff(
          make_expression<typename Traits::constant_type>(fac_lhs + fac_rhs));
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
class symbol_sub_dispatch
    : public sub_dispatch<Traits, symbol_sub_dispatch<Traits>> {
  using base = sub_dispatch<Traits, symbol_sub_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  symbol_sub_dispatch(expr_holder_t lhs, expr_holder_t rhs)
      : base(std::move(lhs), std::move(rhs)),
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
      const auto value{1.0 - base::get_coefficient(rhs, 1.0)};
      mul.set_coeff(
          make_expression<typename Traits::constant_type>(value.abs()));
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
class one_sub_dispatch
    : public sub_dispatch<Traits, one_sub_dispatch<Traits>> {
  using base = sub_dispatch<Traits, one_sub_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;
  using base::get_coefficient;
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
