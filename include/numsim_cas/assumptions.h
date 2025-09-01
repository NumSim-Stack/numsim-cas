#ifndef ASSUMPTIONS_H
#define ASSUMPTIONS_H

#include "expression_holder.h"
#include <set>
#include <typeindex>
#include <typeinfo>
#include <variant>

namespace numsim::cas {

template <typename Container> class assumption {
public:
  using value_type = Container;
  assumption() = default;
  assumption(assumption const &data) : m_data(data.m_data) {}
  assumption(assumption &&data) : m_data(std::move(data.m_data)) {}
  template <typename T> assumption(T &&data) : m_data(std::forward<T>(data)) {}

  template <typename T> constexpr inline auto &operator=(T &&data) {
    m_data = std::forward<T>(data);
    return m_data;
  }

  template <typename T> constexpr inline auto &operator=(T const &data) {
    m_data = data;
    return m_data;
  }

  constexpr inline auto &operator=(assumption &&data) {
    m_data = std::move(data.m_data);
    return m_data;
  }

  constexpr const auto &get() const { return m_data; }

private:
  value_type m_data;
};

} // Namespace numsim::cas

namespace std {
template <> struct less<std::any> {
  inline auto operator()(std::any const &lhs,
                         std::any const &rhs) const noexcept {
    return lhs.type().hash_code() < rhs.type().hash_code();
  }
};

template <typename Container> struct less<numsim::cas::assumption<Container>> {
  using assumption = numsim::cas::assumption<Container>;
  constexpr inline auto operator()(assumption const &lhs,
                                   assumption const &rhs) const noexcept {
    return std::less<typename assumption::value_type>{}(lhs.get(), rhs.get());
  }
};
} // Namespace std

namespace numsim::cas {

template <typename Container = std::any> class assumption_manager {
public:
  // Add an assumption
  template <typename Assumption> void insert(Assumption &&a) {
    m_assumption_set.insert(a);
  }

  // Remove an assumption
  template <typename Assumption> void erase(Assumption &&a) {
    m_assumption_set.erase(a);
  }

  // Check if an assumption exists
  template <typename Assumption> bool contains(Assumption &&a) const {
    return m_assumption_set.contains(a);
  }

private:
  std::set<assumption<Container>> m_assumption_set;
};

struct positive {};    // symbol > 0
struct negative {};    // symbol < 0
struct nonzero {};     // symbol /= 0
struct nonnegative {}; // symbol >= 0
struct nonpositive {}; // symbol <= 0
struct integer {};     // symbol is integer
struct even {};        // symbol is even integer
struct odd {};         // symbol is odd integer
struct rational {};    // symbol is rational
struct irrational {};  // symbol is irrational
struct real {};        // symbol is real
struct complex {};     // symbol is complex
struct prime {};       // symbol is prime

namespace detail {

struct assumption_comparator {
  template <typename LHS, typename RHS>
  bool operator()(const LHS &lhs, const RHS &rhs) const {
    return std::visit(
        [](const auto &left, const auto &right) {
          return std::type_index(typeid(left)) < std::type_index(typeid(right));
        },
        lhs, rhs);
  }
};

// === Define the Assumption Type ===
using assumption_variant =
    std::variant<positive, negative, nonzero, nonnegative, nonpositive, integer,
                 even, odd, rational, irrational, real, complex, prime
                 //,                  symmetric, skewsymmetric
                 >;

using assumption_set = std::set<assumption_variant, assumption_comparator>;

} // namespace detail

// tensor = 1/size(seq) sum_seq basis_change<seq_i>(tensor)
struct symmetric {
  std::vector<sequence> m_sequences;
};

struct skewsymmetric {}; // tensor = tensor - (tensor)^T 2th order tensors
struct volumetric {};    // tensor = vol(tensor) + dev(tensor) 2th order tensors
struct deviatoric {};    // tensor = vol(tensor) + dev(tensor) 2th order tensors
using tensor_space =
    std::variant<symmetric, skewsymmetric, volumetric, deviatoric>;

using tensor_space_set = std::set<tensor_space, detail::assumption_comparator>;

namespace relation {
struct equal {};         // a == b
struct not_equal {};     // a != b
struct greater {};       // a >  b
struct greater_equal {}; // a >= b
struct less {};          // a <  b
struct less_equal {};    // a <= b

using relation_variant =
    std::variant<equal, not_equal, greater, greater_equal, less, less_equal>;

template <typename LHS, typename RHS> class relation {
  template <typename Relation>
  relation(LHS lhs, RHS rhs, Relation type) : lhs(lhs), rhs(rhs), type(type) {}

  // Operator < to allow storage in ordered sets
  bool operator<(const relation &other) const {
    return std::tie(lhs, rhs, type) <
           std::tie(other.lhs, other.rhs, other.type);
  }

private:
  expression_holder<LHS> lhs;
  expression_holder<RHS> rhs;
  relation_variant type;
};

template <typename LHS, typename RHS>
using relation_set = std::set<relation<LHS, RHS>>;
} // namespace relation

// template <typename SymbolType> class assumption_manager {
// private:
//   detail::assumption_set m_assumption_set;
//   relation::relation_set<SymbolType, SymbolType>
//       relation_set; // Store symbolic relations

// public:
//   // Add an assumption
//   template <typename Assumption> void add_assumption(Assumption &&a) {
//     m_assumption_set.insert(a);
//   }

//         // Remove an assumption
//  template <typename Assumption> void remove_assumption(Assumption &&a) {
//    m_assumption_set.erase(a);
//  }

//         // Check if an assumption exists
//  template <typename Assumption> bool has_assumption(Assumption &&a) const {
//    return m_assumption_set.contains(a);
//  }

//         // Add a relation
//  template <typename Relation>
//  void add_relation(SymbolType lhs, SymbolType rhs, Relation type) {
//    relation_set.insert(
//        relation::relation<SymbolType, SymbolType>(lhs, rhs, type));
//  }

//         // Check if a relation holds
//  template <typename Relation>
//  bool check_relation(SymbolType lhs, SymbolType rhs, Relation type) const {
//    return relation_set.find(relation::relation<SymbolType, SymbolType>(
//               lhs, rhs, type)) != relation_set.end();
//  }
//};
} // namespace numsim::cas

#endif // ASSUMPTIONS_H
