#ifndef N_ARY_VECTOR_H
#define N_ARY_VECTOR_H

#include "expression_crtp.h"
#include "get_hash_scalar.h"
#include "is_symbol.h"
#include "numsim_cas_forward.h"
#include "numsim_cas_type_traits.h"
#include <memory>
#include <vector>

namespace numsim::cas {

template <typename Base, typename Derived>
class n_ary_vector : public expression_crtp<Derived, Base> {
public:
  using base = expression_crtp<Derived, Base>;
  using expr_type = typename Base::expr_type;
  using hash_type = typename expr_type::hash_type;
  using expr_holder = expression_holder<expr_type>;
  using value_type = typename expr_type::value_type;
  using iterator = typename umap<expr_type>::iterator;
  using const_iterator = typename umap<expr_holder>::const_iterator;

  n_ary_vector() noexcept { this->reserve(2); }

  template <typename... Args>
  n_ary_vector(Args &&...args) noexcept : base(std::forward<Args>(args)...) {}

  template <typename... Args>
  n_ary_vector(n_ary_vector &&data, Args &&...args) noexcept
      : base(std::forward<Args>(args)...), m_coeff(std::move(data.m_coeff)),
        m_data(std::move(data.m_data)) {
    this->m_hash_value = data.m_hash_value;
  }

  template <typename... Args>
  n_ary_vector(n_ary_vector const &data, Args &&...args) noexcept
      : base(std::forward<Args>(args)...), m_coeff(data.m_coeff),
        m_data(data.m_data) {
    this->m_hash_value = data.m_hash_value;
  }

  inline void push_back(expression_holder<expr_type> const &expr) noexcept {
    insert_hash(expr);
  }

  inline void push_back(expression_holder<expr_type> &&expr) noexcept {
    insert_hash(expr);
  }

  inline void reserve([[maybe_unused]] std::size_t size) noexcept {
    m_data.reserve(size);
  }

  [[nodiscard]] inline auto size() const noexcept { return m_data.size(); }

  [[nodiscard]] inline auto const &data() const noexcept { return m_data; }

  [[nodiscard]] inline auto &data() noexcept { return m_data; }

  inline auto set_coeff(expr_holder const &expr) noexcept { m_coeff = expr; }

  [[nodiscard]] inline auto &coeff() const noexcept { return m_coeff; }
  [[nodiscard]] inline auto &coeff() noexcept { return m_coeff; }

  void update_hash_value() {
    std::vector<std::size_t> child_hashes;
    child_hashes.reserve(m_data.size());
    this->m_hash_value = 0;

    // otherwise we can not provide the order of the symbols
    hash_combine(this->m_hash_value, base::get_id());

    for (const auto &child : m_data) {
      child_hashes.push_back(get_hash_value(child));
    }

    // Sort for commutative operations like addition
    std::stable_sort(child_hashes.begin(), child_hashes.end());

    // Combine all child hashes
    for (const auto &child_hash : child_hashes) {
      hash_combine(this->m_hash_value, child_hash);
    }
  }

  //  template <typename _Base, typename _DerivedLHS, typename _DerivedRHS>
  //  friend bool operator<(n_ary_vector<_Base, _DerivedLHS> const &lhs,
  //                        n_ary_vector<_Base, _DerivedRHS> const &rhs);
  template <typename _Base, typename _DerivedLHS, typename _DerivedRHS>
  friend bool operator<(n_ary_vector<_Base, _DerivedRHS> const &lhs,
                        symbol_base<_Base, _DerivedLHS> const &rhs);
  template <typename _Base, typename _DerivedLHS, typename _DerivedRHS>
  friend bool operator<(symbol_base<_Base, _DerivedLHS> const &lhs,
                        n_ary_vector<_Base, _DerivedRHS> const &rhs);
  template <typename _Base, typename _Derived>
  friend bool operator<(n_ary_vector<_Base, _Derived> const &lhs,
                        n_ary_vector<_Base, _Derived> const &rhs);
  template <typename _Derived, typename _Base, typename _BaseLHS,
            typename _BaseRHS>
  friend bool operator>(n_ary_vector<_Base, _Derived> const &lhs,
                        n_ary_vector<_Base, _Derived> const &rhs);
  template <typename _Derived, typename _Base, typename _BaseLHS,
            typename _BaseRHS>
  friend bool operator==(n_ary_vector<_Base, _Derived> const &lhs,
                         n_ary_vector<_Base, _Derived> const &rhs);
  template <typename _Derived, typename _Base, typename _BaseLHS,
            typename _BaseRHS>
  friend bool operator!=(n_ary_vector<_Base, _Derived> const &lhs,
                         n_ary_vector<_Base, _Derived> const &rhs);

protected:
  expr_holder m_coeff;

private:
  expr_vector<expr_holder> m_data;

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
    m_data.emplace_back(expr);
    update_hash_value();
  }
};

template <typename _Base, typename _DerivedSymbol, typename _DerivedTree>
bool operator<(symbol_base<_Base, _DerivedSymbol> const &lhs,
               n_ary_vector<_Base, _DerivedTree> const &rhs) {
  if (rhs.size() == 1) {
    return lhs.hash_value() < rhs.data().front().get().hash_value();
  }
  return lhs.hash_value() < rhs.hash_value();
}

template <typename _Base, typename _DerivedSymbol, typename _DerivedTree>
bool operator<(n_ary_vector<_Base, _DerivedTree> const &lhs,
               symbol_base<_Base, _DerivedSymbol> const &rhs) {
  if (lhs.size() == 1) {
    return lhs.data().front().get().hash_value() < rhs.hash_value();
  }
  return lhs.hash_value() < rhs.hash_value();
}

template <typename _Base, typename _Derived>
bool operator<(n_ary_vector<_Base, _Derived> const &lhs,
               n_ary_vector<_Base, _Derived> const &rhs) {
  if (lhs.size() == 1 && rhs.size() == 1) {
    return lhs.data().front() < rhs.data().front();
  }
  return lhs.hash_value() < rhs.hash_value();
}

template <typename _Base, typename _Derived>
bool operator>(n_ary_vector<_Base, _Derived> const &lhs,
               n_ary_vector<_Base, _Derived> const &rhs) {
  return !(lhs < rhs);
}

template <typename _Base, typename _Derived>
bool operator==(n_ary_vector<_Base, _Derived> const &lhs,
                n_ary_vector<_Base, _Derived> const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

template <typename _Base, typename _Derived>
bool operator!=(n_ary_vector<_Base, _Derived> const &lhs,
                n_ary_vector<_Base, _Derived> const &rhs) {
  if (lhs.size() == 1 && rhs.size() == 1) {
    return lhs.hash_map().begin()->second == rhs.hash_map().begin()->second;
  }
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // N_ARY_VECTOR_H
