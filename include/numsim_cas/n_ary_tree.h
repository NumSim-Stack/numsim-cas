#ifndef N_ARY_TREE_H
#define N_ARY_TREE_H

#include "expression_crtp.h"
#include "get_hash_scalar.h"
#include "is_symbol.h"
#include "numsim_cas_forward.h"
#include "numsim_cas_type_traits.h"
#include <memory>
#include <vector>

namespace numsim::cas {

template <typename Base, typename Derived>
class n_ary_tree : public expression_crtp<Derived, Base> {
public:
  using derived_type = Derived;
  using base = expression_crtp<Derived, Base>;
  using expr_type = typename Base::expr_type;
  using hash_type = typename expr_type::hash_type;
  using expr_holder = expression_holder<expr_type>;
  using value_type = typename expr_type::value_type;
  using iterator = typename umap<expr_type>::iterator;
  using const_iterator = typename umap<expr_holder>::const_iterator;

  n_ary_tree() noexcept { this->reserve(2); }

  template <typename... Args>
  n_ary_tree(Args &&...args) noexcept : base(std::forward<Args>(args)...) {}

  template <typename... Args>
  n_ary_tree(n_ary_tree &&data, Args &&...args) noexcept
      : base(std::forward<Args>(args)...), m_coeff(std::move(data.m_coeff)),
        m_symbol_map(std::move(data.m_symbol_map)) {
    this->m_hash_value = data.m_hash_value;
  }

  template <typename... Args>
  n_ary_tree(n_ary_tree const &data, Args &&...args) noexcept
      : base(std::forward<Args>(args)...), m_coeff(data.m_coeff),
        m_symbol_map(data.m_symbol_map) {
    this->m_hash_value = data.m_hash_value;
  }

  virtual ~n_ary_tree() = default;

  inline void push_back(expression_holder<expr_type> const &expr) noexcept {
    insert_hash(expr);
  }

  inline void push_back(expression_holder<expr_type> &&expr) noexcept {
    insert_hash(expr);
  }

  inline void reserve([[maybe_unused]] std::size_t size) noexcept {
    // m_symbol_map.reserve(size);
  }

  [[nodiscard]] inline auto size() const noexcept {
    return m_symbol_map.size();
  }

  [[nodiscard]] inline auto const &hash_map() const noexcept {
    return m_symbol_map;
  }

  [[nodiscard]] inline auto &hash_map() noexcept { return m_symbol_map; }
  [[nodiscard]] inline auto hash_map_values() noexcept {
    return m_symbol_map | std::views::values;
  }
  [[nodiscard]] inline auto hash_map_values() const noexcept {
    return m_symbol_map | std::views::values;
  }

  inline auto set_coeff(expr_holder const &expr) noexcept { m_coeff = expr; }

  [[nodiscard]] inline auto &coeff() const noexcept { return m_coeff; }
  [[nodiscard]] inline auto &coeff() noexcept { return m_coeff; }

  //  template <typename _Base, typename _DerivedLHS, typename _DerivedRHS>
  //  friend bool operator<(n_ary_tree<_Base, _DerivedLHS> const &lhs,
  //                        n_ary_tree<_Base, _DerivedRHS> const &rhs);
  template <typename _Base, typename _DerivedLHS, typename _DerivedRHS>
  friend bool operator<(n_ary_tree<_Base, _DerivedRHS> const &lhs,
                        symbol_base<_Base, _DerivedLHS> const &rhs);
  template <typename _Base, typename _DerivedLHS, typename _DerivedRHS>
  friend bool operator<(symbol_base<_Base, _DerivedLHS> const &lhs,
                        n_ary_tree<_Base, _DerivedRHS> const &rhs);
  template <typename _Base, typename _Derived>
  friend bool operator<(n_ary_tree<_Base, _Derived> const &lhs,
                        n_ary_tree<_Base, _Derived> const &rhs);
  template <typename _Derived, typename _Base, typename _BaseLHS,
            typename _BaseRHS>
  friend bool operator>(n_ary_tree<_Base, _Derived> const &lhs,
                        n_ary_tree<_Base, _Derived> const &rhs);
  template <typename _Derived, typename _Base, typename _BaseLHS,
            typename _BaseRHS>
  friend bool operator==(n_ary_tree<_Base, _Derived> const &lhs,
                         n_ary_tree<_Base, _Derived> const &rhs);
  template <typename _Derived, typename _Base, typename _BaseLHS,
            typename _BaseRHS>
  friend bool operator!=(n_ary_tree<_Base, _Derived> const &lhs,
                         n_ary_tree<_Base, _Derived> const &rhs);

protected:
  virtual void update_hash_value() const override {
    std::vector<std::size_t> child_hashes;
    child_hashes.reserve(m_symbol_map.size());
    this->m_hash_value = 0;

    // otherwise we can not provide the order of the symbols
    hash_combine(this->m_hash_value, base::get_id());

    for (const auto &child : m_symbol_map | std::views::values) {
      child_hashes.push_back(get_hash_value(child));
    }

    //    // for a single entry return this
    //    if (child_hashes.size() == 1) {
    //      this->m_hash_value = child_hashes.front();
    //      return;
    //    }

    // Sort for commutative operations like addition
    std::stable_sort(child_hashes.begin(), child_hashes.end());

    // Combine all child hashes
    for (const auto &child_hash : child_hashes) {
      hash_combine(this->m_hash_value, child_hash);
    }
  }

  expr_holder m_coeff;
  // Derived const &m_derived;

private:
  // x+y+(4*z)+(4*x*y)
  //-> {hash_value(x), x}
  //-> {hash_value(y), y}
  //-> {hash_value(z), 4z}
  //-> {hash_value(x*y), 4*x*y}
  umap<expr_holder> m_symbol_map;

private:
  template <typename T, typename... Args>
  void init_parameter_pack(T &&first, Args &&...args) noexcept {
    add_child(std::forward<T>(first));
    init_parameter_pack(std::forward<Args>(args)...);
  }

  template <typename T> void init_parameter_pack(T &&first) noexcept {
    add_child(std::forward<T>(first));
  }

  template <typename T> void insert_hash(T const &expr) noexcept {
    assert(m_symbol_map.find(expr) == m_symbol_map.end());
    m_symbol_map[expr] = expr;
    update_hash_value();
  }
};

template <typename _Base, typename _DerivedSymbol, typename _DerivedTree>
bool operator<(symbol_base<_Base, _DerivedSymbol> const &lhs,
               n_ary_tree<_Base, _DerivedTree> const &rhs) {
  if (rhs.size() == 1) {
    return lhs.hash_value() < rhs.hash_map().begin()->second.get().hash_value();
  }
  return lhs.hash_value() < rhs.hash_value();
}

template <typename _Base, typename _DerivedSymbol, typename _DerivedTree>
bool operator<(n_ary_tree<_Base, _DerivedTree> const &lhs,
               symbol_base<_Base, _DerivedSymbol> const &rhs) {
  if (lhs.size() == 1) {
    return lhs.hash_map().begin()->second.get().hash_value() < rhs.hash_value();
  }
  return lhs.hash_value() < rhs.hash_value();
}

template <typename _Base, typename _Derived>
bool operator<(n_ary_tree<_Base, _Derived> const &lhs,
               n_ary_tree<_Base, _Derived> const &rhs) {
  if (lhs.size() == 1 && rhs.size() == 1) {
    return lhs.hash_map().begin()->second < rhs.hash_map().begin()->second;
  }
  return lhs.hash_value() < rhs.hash_value();
}

template <typename _Base, typename _Derived>
bool operator>(n_ary_tree<_Base, _Derived> const &lhs,
               n_ary_tree<_Base, _Derived> const &rhs) {
  return !(lhs < rhs);
}

template <typename _Base, typename _Derived>
bool operator==(n_ary_tree<_Base, _Derived> const &lhs,
                n_ary_tree<_Base, _Derived> const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

template <typename _Base, typename _Derived>
bool operator!=(n_ary_tree<_Base, _Derived> const &lhs,
                n_ary_tree<_Base, _Derived> const &rhs) {
  if (lhs.size() == 1 && rhs.size() == 1) {
    return lhs.hash_map().begin()->second == rhs.hash_map().begin()->second;
  }
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // N_ARY_TREE_H
