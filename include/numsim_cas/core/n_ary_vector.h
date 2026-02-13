#ifndef N_ARY_VECTOR_H
#define N_ARY_VECTOR_H

#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/core/hash_functions.h>
#include <numsim_cas/is_symbol.h>
#include <numsim_cas/numsim_cas_forward.h>
#include <numsim_cas/numsim_cas_type_traits.h>
#include <vector>

namespace numsim::cas {

template <typename Base> class n_ary_vector : public Base {
public:
  using base_t = Base;
  using expr_t = typename base_t::expr_t;
  using expr_holder_t = expression_holder<expr_t>;
  using iterator = typename umap<expr_holder_t>::iterator;
  using const_iterator = typename umap<expr_holder_t>::const_iterator;

  n_ary_vector() noexcept { this->reserve(2); }

  template <typename... Args>
  n_ary_vector(Args &&...args) noexcept : base_t(std::forward<Args>(args)...) {}

  template <typename... Args>
  n_ary_vector(n_ary_vector &&data, Args &&...args) noexcept
      : base_t(std::forward<Args>(args)...), m_coeff(std::move(data.m_coeff)),
        m_data(std::move(data.m_data)) {
    this->m_hash_value = data.m_hash_value;
  }

  template <typename... Args>
  n_ary_vector(n_ary_vector const &data, Args &&...args) noexcept
      : base_t(std::forward<Args>(args)...), m_coeff(data.m_coeff),
        m_data(data.m_data) {
    this->m_hash_value = data.m_hash_value;
  }

  virtual ~n_ary_vector() = default;

  inline void push_back(expression_holder<expr_t> const &expr) noexcept {
    insert_hash(expr);
  }

  inline void push_back(expression_holder<expr_t> &&expr) noexcept {
    insert_hash(expr);
  }

  inline void reserve([[maybe_unused]] std::size_t size) noexcept {
    m_data.reserve(size);
  }

  [[nodiscard]] inline auto size() const noexcept { return m_data.size(); }

  [[nodiscard]] inline auto const &data() const noexcept { return m_data; }

  [[nodiscard]] inline auto &data() noexcept { return m_data; }

  inline auto set_coeff(expr_holder_t const &expr) noexcept { m_coeff = expr; }

  [[nodiscard]] inline auto &coeff() const noexcept { return m_coeff; }
  [[nodiscard]] inline auto &coeff() noexcept { return m_coeff; }

  //  template <typename _Base, typename _DerivedLHS, typename _DerivedRHS>
  //  friend bool operator<(n_ary_vector<_Base, _DerivedLHS> const &lhs,
  //                        n_ary_vector<_Base, _DerivedRHS> const &rhs);
  template <typename _BaseLHS, typename _BaseRHS>
  friend bool operator<(n_ary_vector<_BaseLHS> const &lhs,
                        symbol_base<_BaseRHS> const &rhs);
  template <typename _BaseLHS, typename _BaseRHS>
  friend bool operator<(symbol_base<_BaseLHS> const &lhs,
                        n_ary_vector<_BaseRHS> const &rhs);
  template <typename _BaseLHS, typename _BaseRHS>
  friend bool operator<(n_ary_vector<_BaseLHS> const &lhs,
                        n_ary_vector<_BaseRHS> const &rhs);
  template <typename _BaseLHS, typename _BaseRHS>
  friend bool operator>(n_ary_vector<_BaseLHS> const &lhs,
                        n_ary_vector<_BaseRHS> const &rhs);
  template <typename _BaseLHS, typename _BaseRHS>
  friend bool operator==(n_ary_vector<_BaseLHS> const &lhs,
                         n_ary_vector<_BaseRHS> const &rhs);
  template <typename _BaseLHS, typename _BaseRHS>
  friend bool operator!=(n_ary_vector<_BaseLHS> const &lhs,
                         n_ary_vector<_BaseRHS> const &rhs);

protected:
  virtual void update_hash_value() const override {
    std::vector<std::size_t> child_hashes;
    child_hashes.reserve(m_data.size());
    this->m_hash_value = 0;

    // otherwise we can not provide the order of the symbols
    hash_combine(this->m_hash_value, base_t::get_id());

    for (const auto &child : m_data) {
      child_hashes.push_back(child.get().hash_value());
    }

    // Sort for commutative operations like addition
    std::stable_sort(child_hashes.begin(), child_hashes.end());

    // Combine all child hashes
    for (const auto &child_hash : child_hashes) {
      hash_combine(this->m_hash_value, child_hash);
    }
  }

  expr_holder_t m_coeff;

private:
  expr_vector<expr_holder_t> m_data;

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

template <typename _BaseVector, typename _BaseSmbol>
bool operator<(symbol_base<_BaseSmbol> const &lhs,
               n_ary_vector<_BaseVector> const &rhs) {
  if (rhs.size() == 1) {
    return lhs.hash_value() < rhs.data().front().get().hash_value();
  }
  return lhs.hash_value() < rhs.hash_value();
}

template <typename _BaseVector, typename _BaseSmbol>
bool operator<(n_ary_vector<_BaseVector> const &lhs,
               symbol_base<_BaseSmbol> const &rhs) {
  if (lhs.size() == 1) {
    return lhs.data().front().get().hash_value() < rhs.hash_value();
  }
  return lhs.hash_value() < rhs.hash_value();
}

template <typename _BaseLHS, typename _BaseRHS>
bool operator<(n_ary_vector<_BaseLHS> const &lhs,
               n_ary_vector<_BaseRHS> const &rhs) {
  if (lhs.size() == 1 && rhs.size() == 1) {
    return lhs.data().front() < rhs.data().front();
  }
  return lhs.hash_value() < rhs.hash_value();
}

template <typename _BaseLHS, typename _BaseRHS>
bool operator>(n_ary_vector<_BaseLHS> const &lhs,
               n_ary_vector<_BaseRHS> const &rhs) {
  return rhs < lhs;
}

template <typename _BaseLHS, typename _BaseRHS>
bool operator==(n_ary_vector<_BaseLHS> const &lhs,
                n_ary_vector<_BaseRHS> const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

template <typename _BaseLHS, typename _BaseRHS>
bool operator!=(n_ary_vector<_BaseLHS> const &lhs,
                n_ary_vector<_BaseRHS> const &rhs) {
  if (lhs.size() == 1 && rhs.size() == 1) {
    return lhs.data().front() != rhs.data().front();
  }
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // N_ARY_VECTOR_H
