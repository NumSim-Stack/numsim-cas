#ifndef N_ARY_TREE_H
#define N_ARY_TREE_H

#include <algorithm>
#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/core/hash_functions.h>
#include <numsim_cas/expression_crtp.h>
#include <numsim_cas/is_symbol.h>
#include <numsim_cas/numsim_cas_forward.h>
#include <numsim_cas/numsim_cas_type_traits.h>
#include <ranges>
#include <vector>

namespace numsim::cas {

template <typename Base> class n_ary_tree : public Base {
public:
  using base_t = Base;
  using expr_t = typename Base::expr_t;
  using hash_t = typename expr_t::hash_type;
  using expr_holder_t = expression_holder<expr_t>;
  using iterator = typename expr_ordered_map<expr_t>::iterator;
  using const_iterator =
      typename expr_ordered_map<expr_holder_t>::const_iterator;

  n_ary_tree() noexcept { this->reserve(2); }

  template <typename... Args>
  n_ary_tree(Args &&...args) noexcept : base_t(std::forward<Args>(args)...) {}

  template <typename... Args>
  n_ary_tree(n_ary_tree &&data, Args &&...args) noexcept
      : base_t(std::forward<Args>(args)...), m_coeff(std::move(data.m_coeff)),
        m_symbol_map(std::move(data.m_symbol_map)) {
    this->m_hash_value = data.m_hash_value;
  }

  template <typename... Args>
  n_ary_tree(n_ary_tree const &data, Args &&...args) noexcept
      : base_t(std::forward<Args>(args)...), m_coeff(data.m_coeff),
        m_symbol_map(data.m_symbol_map) {
    this->m_hash_value = data.m_hash_value;
  }

  ~n_ary_tree() override = default;

  inline void push_back(expression_holder<expr_t> const &expr) {
    insert_hash(expr);
  }

  inline void push_back(expression_holder<expr_t> &&expr) {
    insert_hash(std::move(expr));
  }

  inline void reserve([[maybe_unused]] std::size_t size) noexcept {
    // m_symbol_map.reserve(size);
  }

  [[nodiscard]] inline auto size() const noexcept {
    return m_symbol_map.size();
  }

  [[nodiscard]] inline auto const &symbol_map() const noexcept {
    return m_symbol_map;
  }

  [[nodiscard]] inline auto &symbol_map() noexcept { return m_symbol_map; }
  [[nodiscard]] inline auto symbol_map_values() noexcept {
    return m_symbol_map | std::views::values;
  }
  [[nodiscard]] inline auto symbol_map_values() const noexcept {
    return m_symbol_map | std::views::values;
  }

  inline auto set_coeff(expr_holder_t const &expr) noexcept { m_coeff = expr; }
<<<<<<< HEAD
  inline auto set_coeff(expr_holder_t &&expr) noexcept {
    m_coeff = std::move(expr);
  }
=======
  inline auto set_coeff(expr_holder_t &&expr) noexcept { m_coeff = std::move(expr); }
>>>>>>> origin/move_to_virtual

  [[nodiscard]] inline auto const &coeff() const noexcept { return m_coeff; }
  [[nodiscard]] inline auto &coeff() noexcept { return m_coeff; }

  //  template <typename _Base, typename _DerivedLHS, typename _DerivedRHS>
  //  friend bool operator<(n_ary_tree<_Base, _DerivedLHS> const &lhs,
  //                        n_ary_tree<_Base, _DerivedRHS> const &rhs);
  template <typename BaseLHS, typename BaseRHS>
  friend bool operator<(n_ary_tree<BaseLHS> const &lhs,
                        symbol_base<BaseRHS> const &rhs);
  template <typename BaseLHS, typename BaseRHS>
  friend bool operator<(symbol_base<BaseLHS> const &lhs,
                        n_ary_tree<BaseRHS> const &rhs);
  template <typename BaseLHS, typename BaseRHS>
  friend bool operator<(n_ary_tree<BaseLHS> const &lhs,
                        n_ary_tree<BaseRHS> const &rhs);
  template <typename BaseLHS, typename BaseRHS>
  friend bool operator>(n_ary_tree<BaseLHS> const &lhs,
                        n_ary_tree<BaseRHS> const &rhs);
  template <typename BaseLHS, typename BaseRHS>
  friend bool operator==(n_ary_tree<BaseLHS> const &lhs,
                         n_ary_tree<BaseRHS> const &rhs);
  template <typename BaseLHS, typename BaseRHS>
  friend bool operator!=(n_ary_tree<BaseLHS> const &lhs,
                         n_ary_tree<BaseRHS> const &rhs);

protected:
  void update_hash_value() const noexcept override {
    this->m_hash_value = 0;

    // otherwise we can not provide the order of the symbols
    hash_combine(this->m_hash_value, base_t::get_id());

    std::vector<std::size_t> hashes(m_symbol_map.size());
    std::size_t i = 0;
    for (const auto &child : m_symbol_map | std::views::values) {
      hashes[i++] = child->hash_value();
    }

    // Sort for commutative operations like addition
    std::sort(hashes.begin(), hashes.end());

    // Combine all child hashes
    for (auto h : hashes) {
      hash_combine(this->m_hash_value, h);
    }
  }

  expr_holder_t m_coeff;
  // Derived const &m_derived;

private:
  // x+y+(4*z)+(4*x*y)
  //-> {hash_value(x), x}
  //-> {hash_value(y), y}
  //-> {hash_value(z), 4z}
  //-> {hash_value(x*y), 4*x*y}
  expr_ordered_map<expr_holder_t> m_symbol_map;

private:
  void insert_hash(expression_holder<expr_t> const &expr) {
    if (m_symbol_map.contains(expr)) {
      throw internal_error(
          "n_ary_tree::insert_hash: duplicate child insertion");
    }
    m_symbol_map[expr] = expr;
  }

  void insert_hash(expression_holder<expr_t> &&expr) {
    if (m_symbol_map.contains(expr)) {
      throw internal_error(
          "n_ary_tree::insert_hash: duplicate child insertion");
    }
    m_symbol_map[expr] = std::move(expr);
  }
};

template <typename BaseSymbol, typename BaseTree>
bool operator<(symbol_base<BaseSymbol> const &lhs,
               n_ary_tree<BaseTree> const &rhs) {
  if (rhs.size() == 1) {
    return lhs.hash_value() <
           rhs.symbol_map().begin()->second.get().hash_value();
  }
  return lhs.hash_value() < rhs.hash_value();
}

template <typename BaseSymbol, typename BaseTree>
bool operator<(n_ary_tree<BaseTree> const &lhs,
               symbol_base<BaseSymbol> const &rhs) {
  if (lhs.size() == 1) {
    return lhs.symbol_map().begin()->second.get().hash_value() <
           rhs.hash_value();
  }
  return lhs.hash_value() < rhs.hash_value();
}

template <typename BaseLHS, typename BaseRHS>
bool operator<(n_ary_tree<BaseLHS> const &lhs, n_ary_tree<BaseRHS> const &rhs) {
  if (lhs.hash_value() != rhs.hash_value())
    return lhs.hash_value() < rhs.hash_value();
  if (lhs.size() != rhs.size())
    return lhs.size() < rhs.size();
  auto lit = lhs.symbol_map().begin();
  auto rit = rhs.symbol_map().begin();
  for (; lit != lhs.symbol_map().end(); ++lit, ++rit) {
    if (lit->second < rit->second)
      return true;
    if (rit->second < lit->second)
      return false;
  }
  return false;
}

template <typename BaseLHS, typename BaseRHS>
bool operator>(n_ary_tree<BaseLHS> const &lhs, n_ary_tree<BaseRHS> const &rhs) {
  return rhs < lhs;
}

template <typename BaseLHS, typename BaseRHS>
bool operator==(n_ary_tree<BaseLHS> const &lhs,
                n_ary_tree<BaseRHS> const &rhs) {
  if (lhs.hash_value() != rhs.hash_value())
    return false;
  if (lhs.id() != rhs.id())
    return false;
  if (lhs.size() != rhs.size())
    return false;
  auto it_l = lhs.symbol_map().begin();
  auto it_r = rhs.symbol_map().begin();
  for (; it_l != lhs.symbol_map().end(); ++it_l, ++it_r) {
    if (it_l->second != it_r->second)
      return false;
  }
  return true;
}

template <typename BaseLHS, typename BaseRHS>
bool operator!=(n_ary_tree<BaseLHS> const &lhs,
                n_ary_tree<BaseRHS> const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // N_ARY_TREE_H
