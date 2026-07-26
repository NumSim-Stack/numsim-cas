#ifndef SIMPLIFIER_ADD_H
#define SIMPLIFIER_ADD_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/domain_traits.h>
#include <numsim_cas/core/scalar_number.h>
#include <numsim_cas/core/simplifier/simplifier_common.h>
#include <numsim_cas/functions.h>
#include <ranges>
#include <set>

namespace numsim::cas {
namespace detail {

//==============================================================================
// add_dispatch<Traits, Derived> — Base algorithm for A + B
//==============================================================================
template <typename Traits, typename Derived = void>
requires basic_expression_domain<typename Traits::expression_type>
class add_dispatch {
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

      // Like terms first: (c1*T)+(c2*T) → (c1+c2)*T, T+(c*T) → (1+c)*T
      // (also covers c*T + c*T without nesting muls)
      if (auto merged = try_merge_like(m_lhs, m_rhs); merged.is_valid()) {
        return merged;
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
      // Domains without n_ary_tree mul_type (e.g. tensor) override
      // get_default()
      return m_lhs;
    }
  }

  static bool is_numeric_expr(expr_holder_t const &expr) {
    return Traits::try_numeric(expr).has_value();
  }

  // (c1*T)+(c2*T) with equal children → (c1+c2)*T; also T+(c*T) → (1+c)*T.
  // Coefficients combine as expressions so symbolic (t2s) coeffs survive.
  // Returns an invalid holder when the operands are not like terms.
  expr_holder_t try_merge_like(expr_holder_t const &a, expr_holder_t const &b) {
    if constexpr (std::is_void_v<typename Traits::mul_type>) {
      return expr_holder_t{};
    } else {
      using mul_type = typename Traits::mul_type;
      const bool a_mul{is_same<mul_type>(a)};
      const bool b_mul{is_same<mul_type>(b)};
      if (!a_mul && !b_mul)
        return expr_holder_t{};
      auto coeff_or_one = [](mul_type const &m) {
        return m.coeff().is_valid() ? m.coeff()
                                    : Traits::make_constant(scalar_number{1});
      };
      if (a_mul && b_mul) {
        auto const &am{a.template get<mul_type>()};
        auto const &bm{b.template get<mul_type>()};
        if (!am.like_term_of(bm))
          return expr_holder_t{};
        return scaled_copy(am, coeff_or_one(am) + coeff_or_one(bm));
      }
      auto const &m{(a_mul ? a : b).template get<mul_type>()};
      auto const &other{a_mul ? b : a};
      if (!(m.size() == 1 && m.symbol_map().begin()->second == other))
        return expr_holder_t{};
      return scaled_copy(m, coeff_or_one(m) +
                                Traits::make_constant(scalar_number{1}));
    }
  }

  template <typename MulT>
  expr_holder_t scaled_copy(MulT const &m, expr_holder_t coeff) {
    using mul_type = MulT;
    auto val = Traits::try_numeric(coeff);
    if (val && *val == scalar_number{0})
      return Traits::zero(m_lhs);
    auto expr{make_expression<mul_type>(m)};
    auto &mul{expr.template get<mul_type>()};
    if (val && *val == scalar_number{1}) {
      mul.coeff().free();
      if (mul.size() == 1)
        return mul.symbol_map().begin()->second;
      return expr;
    }
    mul.set_coeff(std::move(coeff));
    return expr;
  }

  template <typename Expr> expr_holder_t dispatch(Expr const &) {
    return get_default();
  }

  // expr + 0 --> expr
  expr_holder_t dispatch(typename Traits::zero_type const &) { return m_lhs; }

  // expr + (-expr) --> 0
  expr_holder_t dispatch(typename Traits::negative_type const &neg_node) {
    if (m_lhs == neg_node.expr()) {
      return Traits::zero(m_lhs);
    }
    return get_default();
  }

  // non-add + add --> swap so add is LHS (triggers the n-ary dispatcher).
  // Unconditional: the void-mul guard sent tensor A+(B+C) to get_default,
  // which nested the rhs add as a single child (round-11 review).
  expr_holder_t dispatch(typename Traits::add_type const &) {
    return m_rhs + m_lhs;
  }

protected:
  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

//==============================================================================
// constant_add_dispatch<Traits> — LHS is constant
//==============================================================================
template <typename Traits>
requires arithmetic_expression_domain<typename Traits::expression_type>
class constant_add_dispatch
    : public add_dispatch<Traits, constant_add_dispatch<Traits>> {
  using base = add_dispatch<Traits, constant_add_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;

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
  expr_holder_t dispatch([[maybe_unused]]
                         typename Traits::add_type const &rhs) {
    auto lhs_val = Traits::try_numeric(base::m_lhs);
    if (lhs_val) {
      return fold_constant_into_add_coeff<Traits>(
          make_expression<typename Traits::add_type>(rhs), *lhs_val);
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
requires arithmetic_expression_domain<typename Traits::expression_type>
class one_add_dispatch : public add_dispatch<Traits, one_add_dispatch<Traits>> {
  using base = add_dispatch<Traits, one_add_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;

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
  expr_holder_t dispatch([[maybe_unused]]
                         typename Traits::add_type const &rhs) {
    return fold_constant_into_add_coeff<Traits>(
        make_expression<typename Traits::add_type>(rhs), scalar_number{1});
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
requires arithmetic_expression_domain<typename Traits::expression_type>
class n_ary_add_dispatch
    : public add_dispatch<Traits, n_ary_add_dispatch<Traits>> {
  using base = add_dispatch<Traits, n_ary_add_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;

  using base::get_default;

  n_ary_add_dispatch(expr_holder_t lhs_in, expr_holder_t rhs)
      : base(std::move(lhs_in), std::move(rhs)),
        lhs{base::m_lhs.template get<typename Traits::add_type>()} {}

  // (coeff + terms) + constant
  expr_holder_t dispatch([[maybe_unused]]
                         typename Traits::constant_type const &) {
    auto rhs_val = Traits::try_numeric(base::m_rhs);
    if (rhs_val) {
      return fold_constant_into_add_coeff<Traits>(
          make_expression<typename Traits::add_type>(lhs), *rhs_val);
    }
    return base::get_default();
  }

  // (coeff + terms) + 1
  expr_holder_t dispatch([[maybe_unused]] typename Traits::one_type const &) {
    return fold_constant_into_add_coeff<Traits>(
        make_expression<typename Traits::add_type>(lhs), scalar_number{1});
  }

  // Zero-filter + degenerate collapse after in-place mutation: a
  // cancellation must not leave a literal zero child, a single-child add,
  // or a stale cached hash (review findings on #339/#340).
  expr_holder_t merge_and_finish(expr_holder_t expr_add,
                                 typename Traits::add_type &add,
                                 expr_holder_t combined) {
    if (!is_same<typename Traits::zero_type>(combined)) {
      // signed insert: the combined term may exactly negate an existing
      // child; a plain insert would leave a {t,-t} pair (round-8 review)
      add_insert_signed(add, std::move(combined), [](expr_holder_t const &e) {
        return is_same<typename Traits::zero_type>(e);
      });
    }
    add.invalidate_hash();
    if (add.size() == 0) {
      return add.coeff().is_valid() ? add.coeff() : Traits::zero();
    }
    if (add.size() == 1 && !add.coeff().is_valid()) {
      return add.symbol_map().begin()->second;
    }
    return expr_add;
  }

  // x+y+z + x --> 2*x+y+z; x+y+... + c*x --> (1+c)*x+y+...
  // (symbol domains only; this template deduces every non-specialized rhs)
  template <typename SymbolType = typename Traits::symbol_type>
  requires(!std::is_void_v<SymbolType>)
  expr_holder_t dispatch(SymbolType const &) {
    using mul_type = typename Traits::mul_type;
    auto expr_add{make_expression<typename Traits::add_type>(lhs)};
    auto &add{expr_add.template get<typename Traits::add_type>()};
    auto pos{add.find_like(base::m_rhs)};
    if (pos == add.symbol_map().end()) {
      if (is_same<mul_type>(base::m_rhs)) {
        // stored bare x vs incoming c*x: probe the bare child (review #339)
        auto const &rm{base::m_rhs.template get<mul_type>()};
        if (rm.size() == 1) {
          pos = add.find_like(rm.symbol_map().begin()->second);
        }
      } else {
        // stored c*x vs incoming bare x: retry with a bare mul{x}
        auto probe{make_expression<mul_type>()};
        probe.template get<mul_type>().push_back(base::m_rhs);
        pos = add.find_like(probe);
      }
    }
    if (pos == add.symbol_map().end()) {
      // negation pairs share no hash: (3-x)+x needs a -x probe; only an
      // exact -x cancels — like-terms of -x would nest adds (round-7)
      auto neg_probe{-base::m_rhs};
      pos = add.find_like(neg_probe);
      if (pos != add.symbol_map().end() && !(pos->second == neg_probe)) {
        pos = add.symbol_map().end();
      }
    }
    if (pos != add.symbol_map().end()) {
      auto expr{pos->second + base::m_rhs};
      add.symbol_map().erase(pos);
      return merge_and_finish(std::move(expr_add), add, std::move(expr));
    }
    add.push_back(base::m_rhs);
    return expr_add;
  }

  // merge two add expressions; zero-filter + collapse since childwise
  // combines may fully cancel ((3+x)+(1-x) -> 4), round-7 review
  expr_holder_t dispatch(typename Traits::add_type const &rhs) {
    auto expr{make_expression<typename Traits::add_type>()};
    auto &add{expr.template get<typename Traits::add_type>()};
    merge_add(lhs, rhs, add, [](expr_holder_t const &e) {
      return is_same<typename Traits::zero_type>(e);
    });
    return finalize_add<Traits>(std::move(expr));
  }

  // (coeff + terms) + (-expr)
  expr_holder_t dispatch(typename Traits::negative_type const &rhs) {
    // map keys compare by hash; confirm deep equality before cancelling
    const auto pos{lhs.symbol_map().find(rhs.expr())};
    if (pos != lhs.symbol_map().end() && pos->second == rhs.expr()) {
      auto expr{make_expression<typename Traits::add_type>(lhs)};
      auto &add{expr.template get<typename Traits::add_type>()};
      add.symbol_map().erase(rhs.expr());
      add.invalidate_hash();
      if (add.size() == 0) {
        return add.coeff().is_valid() ? add.coeff() : Traits::zero();
      }
      if (add.size() == 1 && !add.coeff().is_valid()) {
        return add.symbol_map().begin()->second;
      }
      return expr;
    }

    auto inner_val = Traits::try_numeric(rhs.expr());
    if (inner_val) {
      return fold_constant_into_add_coeff<Traits>(
          make_expression<typename Traits::add_type>(lhs), -(*inner_val));
    }

    // add + (-add): route through subtraction, which cancels childwise
    // with deep comparison (map keys alone alias e.g. a against 1+a)
    if (is_same<typename Traits::add_type>(rhs.expr())) {
      return base::m_lhs - rhs.expr();
    }

    // non-cancelling -t: insert into a copy — get_default would nest the
    // whole lhs add as a single child (round-9 review)
    auto add_expr{make_expression<typename Traits::add_type>(lhs)};
    auto &add{add_expr.template get<typename Traits::add_type>()};
    insert_signed<Traits>(add, base::m_rhs);
    return detail::finalize_add<Traits>(std::move(add_expr));
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
requires arithmetic_expression_domain<typename Traits::expression_type>
class n_ary_mul_add_dispatch
    : public add_dispatch<Traits, n_ary_mul_add_dispatch<Traits>> {
  using base = add_dispatch<Traits, n_ary_mul_add_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;

  using base::get_default;

  n_ary_mul_add_dispatch(expr_holder_t lhs_in, expr_holder_t rhs)
      : base(std::move(lhs_in), std::move(rhs)),
        lhs{base::m_lhs.template get<typename Traits::mul_type>()} {}

  // constant*expr + expr --> (constant+1)*expr  (symbol domains only)
  template <typename SymbolType = typename Traits::symbol_type>
  requires(!std::is_void_v<SymbolType>)
  expr_holder_t dispatch(SymbolType const &) {
    // deep single-child check: a map find would alias child x+2 against x
    if (lhs.size() == 1 && lhs.symbol_map().begin()->second == base::m_rhs) {
      return base::scaled_copy(
          lhs, Traits::make_constant(get_coefficient<Traits>(lhs, 1) + 1));
    }
    return get_default();
  }

  /// c1*T + c2*T --> (c1+c2)*T
  expr_holder_t dispatch(typename Traits::mul_type const &rhs) {
    if (lhs.like_term_of(rhs)) {
      return base::try_merge_like(base::m_lhs, base::m_rhs);
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
requires arithmetic_expression_domain<typename Traits::expression_type>
class symbol_add_dispatch
    : public add_dispatch<Traits, symbol_add_dispatch<Traits>> {
  using base = add_dispatch<Traits, symbol_add_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;

  using base::get_default;

  symbol_add_dispatch(expr_holder_t lhs_in, expr_holder_t rhs)
      : base(std::move(lhs_in), std::move(rhs)),
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
    // deep single-child check: a map find would alias child x+2 against x
    if (rhs.size() == 1 && rhs.symbol_map().begin()->second == base::m_lhs) {
      return base::scaled_copy(
          rhs, Traits::make_constant(get_coefficient<Traits>(rhs, 1) + 1));
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
requires arithmetic_expression_domain<typename Traits::expression_type>
class negative_add_dispatch
    : public add_dispatch<Traits, negative_add_dispatch<Traits>> {
  using base = add_dispatch<Traits, negative_add_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;

  using base::get_default;

  negative_add_dispatch(expr_holder_t lhs_in, expr_holder_t rhs)
      : base(std::move(lhs_in), std::move(rhs)),
        lhs{base::m_lhs.template get<typename Traits::negative_type>()} {}

  // (-lhs) + (-rhs) --> -(lhs+rhs)
  expr_holder_t dispatch(typename Traits::negative_type const &rhs) {
    return -(lhs.expr() + rhs.expr());
  }

  // -expr + (coeff + terms)
  expr_holder_t dispatch([[maybe_unused]]
                         typename Traits::add_type const &rhs) {
    auto inner_val = Traits::try_numeric(lhs.expr());
    if (inner_val) {
      return fold_constant_into_add_coeff<Traits>(
          make_expression<typename Traits::add_type>(rhs), -(*inner_val));
    }
    auto add_expr{make_expression<typename Traits::add_type>(rhs)};
    auto &add{add_expr.template get<typename Traits::add_type>()};
    // (-x) + (y - x): the map may already hold -x, so merge instead of a
    // raw push_back (which asserts on duplicates)
    insert_signed<Traits>(add, base::m_lhs);
    return detail::finalize_add<Traits>(std::move(add_expr));
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

  // -expr + expr --> 0 (generic fallback)
  template <typename Expr> expr_holder_t dispatch(Expr const &) {
    if (lhs.expr() == base::m_rhs) {
      return Traits::zero();
    }
    return base::get_default();
  }

private:
  typename Traits::negative_type const &lhs;
};

} // namespace detail
} // namespace numsim::cas

#endif // SIMPLIFIER_ADD_H
