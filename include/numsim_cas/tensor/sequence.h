#ifndef SEQUENCE_H
#define SEQUENCE_H

#include <cstddef>
#include <initializer_list>
#include <numsim_cas/core/hash_functions.h>
#include <ostream>
#include <span>
#include <stdexcept>
#include <vector>

namespace numsim::cas {

class sequence {
public:
  using index_t = std::size_t;

  sequence() = default;

  // Construct with size (like vector)
  explicit sequence(std::size_t n) : m_data(n) {}

  // 1-based input -> store 0-based
  sequence(std::initializer_list<index_t> one_based) : m_data(one_based) {
    for (auto &i : m_data) {
      if (i == 0)
        throw std::out_of_range("sequence: 1-based index cannot be 0");
      --i;
    }
  }

  // container surface
  std::size_t size() const noexcept { return m_data.size(); }
  bool empty() const noexcept { return m_data.empty(); }

  void reserve(std::size_t n) { m_data.reserve(n); }
  void resize(std::size_t n) { m_data.resize(n); }

  auto begin() noexcept { return m_data.begin(); }
  auto end() noexcept { return m_data.end(); }
  auto begin() const noexcept { return m_data.begin(); }
  auto end() const noexcept { return m_data.end(); }

  index_t &operator[](std::size_t i) { return m_data[i]; }
  index_t operator[](std::size_t i) const { return m_data[i]; }

  template <class InputIt>
  void insert(typename std::vector<index_t>::iterator pos, InputIt first,
              InputIt last) {
    m_data.insert(pos, first, last);
  }

  const std::vector<index_t> &indices() const noexcept { return m_data; }

  // comparisons (C++20)
  friend bool operator==(sequence const &, sequence const &) = default;
  friend auto operator<=>(sequence const &a, sequence const &b) {
    return a.m_data <=> b.m_data;
  }

  // print as 1-based for mathematical readability
  friend std::ostream &operator<<(std::ostream &os, sequence const &s) {
    os << '{';
    for (std::size_t i = 0; i < s.m_data.size(); ++i) {
      if (i)
        os << ", ";
      os << (s.m_data[i] + 1);
    }
    return os << '}';
  }

private:
  std::vector<index_t> m_data;
};

inline void hash_combine(std::size_t &seed, const sequence &value) {
  for (const auto &c : value.indices()) {
    hash_combine(seed, c);
  }
}

// ---- concat ----
// Concatenate a + b
inline sequence concat(sequence const &a, sequence const &b) {
  sequence out;
  out.reserve(a.size() + b.size());
  out.insert(out.end(), a.begin(), a.end());
  out.insert(out.end(), b.begin(), b.end());
  return out;
}

// Concatenate many sequences: concat_all({a,b,c,...})
inline sequence concat_all(std::span<const sequence> parts) {
  std::size_t total = 0;
  for (auto const &p : parts)
    total += p.size();

  sequence out;
  out.reserve(total);
  for (auto const &p : parts)
    out.insert(out.end(), p.begin(), p.end());
  return out;
}

// ---- split ----
// Split s into (lhs_size, rest)
inline std::pair<sequence, sequence> split(sequence const &s,
                                           std::size_t lhs_size) {
  if (lhs_size > s.size())
    throw std::out_of_range("split: lhs_size > size");

  sequence lhs, rhs;
  lhs.reserve(lhs_size);
  rhs.reserve(s.size() - lhs_size);

  auto split_at = s.begin() + static_cast<std::ptrdiff_t>(lhs_size);
  lhs.insert(lhs.end(), s.begin(), split_at);
  rhs.insert(rhs.end(), split_at, s.end());
  return {std::move(lhs), std::move(rhs)};
}

// Split into N parts given sizes. sizes must sum to s.size()
inline std::vector<sequence> split_many(sequence const &s,
                                        std::span<const std::size_t> sizes) {
  std::size_t sum = 0;
  for (auto n : sizes)
    sum += n;
  if (sum != s.size())
    throw std::invalid_argument(
        "split_many: sizes do not sum to sequence size");

  std::vector<sequence> out;
  out.reserve(sizes.size());

  auto it = s.begin();
  for (auto n : sizes) {
    sequence part;
    part.reserve(n);
    auto step = static_cast<std::ptrdiff_t>(n);
    part.insert(part.end(), it, it + step);
    out.push_back(std::move(part));
    it += step;
  }
  return out;
}

// ---- permute ----
// Permute by 0-based positions: perm[i] is in [0, n).
// out[i] = in[perm[i]]
inline sequence permute(sequence const &in,
                        std::span<const sequence::index_t> perm) {
  if (perm.size() != in.size())
    throw std::invalid_argument("permute: size mismatch");

  sequence out(in.size());
  for (std::size_t i = 0; i < perm.size(); ++i) {
    const auto p = perm[i];
    if (p >= in.size())
      throw std::out_of_range("permute: entry out of range");
    out[i] = in[p];
  }
  return out;
}

inline sequence invert_perm(std::span<const std::size_t> perm) {
  const std::size_t n = perm.size();
  sequence inv(n);

  std::vector<bool> seen(n, false);
  for (std::size_t i = 0; i < n; ++i) {
    const auto p = perm[i];
    if (p >= n)
      throw std::out_of_range("invert_perm: out of range");
    if (seen[p])
      throw std::invalid_argument("invert_perm: duplicate entry");
    seen[p] = true;
    inv[p] = i;
  }
  return inv;
}

} // namespace numsim::cas

#endif // SEQUENCE_H
