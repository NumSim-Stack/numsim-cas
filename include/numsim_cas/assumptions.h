#ifndef ASSUMPTIONS_H
#define ASSUMPTIONS_H

#include "expression_holder.h"
#include "numsim_cas_type_traits.h"
#include <set>
#include <variant>
#include <vector>

namespace numsim::cas {

// ---------- Assumption tags (numeric) ----------
struct positive {};
struct negative {};
struct nonzero {};
struct nonnegative {};
struct nonpositive {};
struct integer {};
struct even {};
struct odd {};
struct rational {};
struct irrational {};
struct real_tag {}; // rename to avoid clash with std::real
struct complex_tag {};
struct prime {};

// ---------- Tensor-space assumptions & payloads ----------
struct symmetric {
  std::vector<sequence>
      permutations; // e.g., {{2,1}} for rank-2, {{3,4,1,2}} for 4th major, …
};
struct skewsymmetric {};
struct volumetric {};
struct deviatoric {};
struct unkown {};

// A unified “assumption” sum type for numbers
using numeric_assumption =
    std::variant<positive, negative, nonzero, nonnegative, nonpositive, integer,
                 even, odd, rational, irrational, real_tag, complex_tag, prime>;

// // A unified “assumption” sum type for tensor space
// using tensor_space =
//     std::variant<symmetric, skewsymmetric, volumetric, deviatoric, unkown>;

// ---------- Lexicographic helpers ----------
inline int cmp_vec(const std::vector<std::size_t> &a,
                   const std::vector<std::size_t> &b) {
  if (a == b)
    return 0;
  return a < b ? -1 : 1;
}
inline int cmp_vecseq(const std::vector<sequence> &A,
                      const std::vector<sequence> &B) {
  const auto n = std::min(A.size(), B.size());
  for (std::size_t i = 0; i < n; ++i) {
    if (A[i] == B[i])
      continue;
    return A[i] < B[i] ? -1 : 1;
  }
  if (A.size() == B.size())
    return 0;
  return A.size() < B.size() ? -1 : 1;
}

// ---------- Variant comparators that respect payloads ----------
struct numeric_assumption_less {
  bool operator()(numeric_assumption const &a,
                  numeric_assumption const &b) const noexcept {
    if (a.index() != b.index())
      return a.index() < b.index();
    // all alts are empty tags → equal here
    return false;
  }
};

// struct tensor_space_less {
//   bool operator()(tensor_space const& a,
//                   tensor_space const& b) const noexcept {
//     if (a.index() != b.index()) return a.index() < b.index();
//     // same alternative → compare payload (only symmetric has payload)
//     if (std::holds_alternative<symmetric>(a)) {
//       auto const& sa = std::get<symmetric>(a).permutations;
//       auto const& sb = std::get<symmetric>(b).permutations;
//       const int c = cmp_vecseq(sa, sb);
//       return c < 0;
//     }
//     // skewsymmetric/volumetric/deviatoric are empty tags
//     return false;
//   }
// };

// ---------- Managers ----------
class numeric_assumption_manager {
public:
  void insert(numeric_assumption a) { set_.insert(std::move(a)); }
  void erase(numeric_assumption const &a) { set_.erase(a); }
  bool contains(numeric_assumption const &a) const {
    return set_.find(a) != set_.end();
  }
  auto const &data() const { return set_; }

private:
  std::set<numeric_assumption, numeric_assumption_less> set_;
};

// class tensor_space_manager {
// public:
//   void insert(tensor_space a)   { set_.insert(std::move(a)); }
//   void erase(tensor_space const& a) { set_.erase(a); }
//   bool contains(tensor_space const& a) const {
//     return set_.find(a) != set_.end();
//   }
//   auto const& data() const { return set_; }

// private:
//   std::set<tensor_space, tensor_space_less> set_;
// };

// ---------- Relations ----------
namespace relation {
struct equal {};
struct not_equal {};
struct greater {};
struct greater_equal {};
struct less {};
struct less_equal {};

using kind =
    std::variant<equal, not_equal, greater, greater_equal, less, less_equal>;

template <typename LHSExpr, typename RHSExpr> struct relation {
  using LHS = expression_holder<LHSExpr>;
  using RHS = expression_holder<RHSExpr>;
  LHS lhs;
  RHS rhs;
  kind k;

  // order by (kind_index, lhs_hash, rhs_hash)
  friend bool operator<(relation const &a, relation const &b) {
    auto ai = a.k.index(), bi = b.k.index();
    if (ai != bi)
      return ai < bi;
    auto al = a.lhs.get().hash_value();
    auto bl = b.lhs.get().hash_value();
    if (al != bl)
      return al < bl;
    auto ar = a.rhs.get().hash_value();
    auto br = b.rhs.get().hash_value();
    return ar < br;
  }
};

template <typename L, typename R> using set = std::set<relation<L, R>>;

} // namespace relation
} // namespace numsim::cas

// namespace numsim::cas {

// struct add_tensor_space
// {
//   static constexpr inline tensor_space operator()(symmetric const&, symmetric
//   const&){
//     return symmetric{};
//   }

//   static constexpr inline tensor_space operator()(symmetric const&,
//   skewsymmetric const&){
//     return unkown{};
//   }

//   static constexpr inline tensor_space operator()(skewsymmetric const&,
//   symmetric const&){
//     return unkown{};
//   }

//   template<typename LHS, typename RHS>
//   static constexpr inline tensor_space operator()(LHS const&, RHS const&){
//     return unkown{};
//   }
// };
// }

// constexpr inline auto operator+(numsim::cas::tensor_space const& lhs,
// numsim::cas::tensor_space const& rhs){
//   return std::visit(numsim::cas::add_tensor_space{}, lhs, rhs);
// }

// namespace numsim::cas {

// template <typename Container> class assumption {
// public:
//   using value_type = Container;
//   assumption() = default;
//   assumption(assumption const &data) : m_data(data.m_data) {}
//   assumption(assumption &&data) : m_data(std::move(data.m_data)) {}
//   template <typename T> assumption(T &&data) : m_data(std::forward<T>(data))
//   {}

//   template <typename T> constexpr inline auto &operator=(T &&data) {
//     m_data = std::forward<T>(data);
//     return m_data;
//   }

//   template <typename T> constexpr inline auto &operator=(T const &data) {
//     m_data = data;
//     return m_data;
//   }

//   constexpr inline auto &operator=(assumption &&data) {
//     m_data = std::move(data.m_data);
//     return m_data;
//   }

//   constexpr const auto &get() const { return m_data; }

// private:
//   value_type m_data;
// };

// } // Namespace numsim::cas

// namespace std {
// template <> struct less<std::any> {
//   inline auto operator()(std::any const &lhs,
//                          std::any const &rhs) const noexcept {
//     return lhs.type().hash_code() < rhs.type().hash_code();
//   }
// };

// template <typename Container> struct less<numsim::cas::assumption<Container>>
// {
//   using assumption = numsim::cas::assumption<Container>;
//   constexpr inline auto operator()(assumption const &lhs,
//                                    assumption const &rhs) const noexcept {
//     return std::less<typename assumption::value_type>{}(lhs.get(),
//     rhs.get());
//   }
// };
// } // Namespace std

// namespace numsim::cas {

// template <typename Container = std::any> class assumption_manager {
// public:
//   // Add an assumption
//   template <typename Assumption> void insert(Assumption &&a) {
//     m_assumption_set.insert(a);
//   }

//   // Remove an assumption
//   template <typename Assumption> void erase(Assumption &&a) {
//     m_assumption_set.erase(a);
//   }

//   // Check if an assumption exists
//   template <typename Assumption> bool contains(Assumption &&a) const {
//     return m_assumption_set.contains(a);
//   }

// private:
//   std::set<assumption<Container>> m_assumption_set;
// };

// struct positive {};    // symbol > 0
// struct negative {};    // symbol < 0
// struct nonzero {};     // symbol /= 0
// struct nonnegative {}; // symbol >= 0
// struct nonpositive {}; // symbol <= 0
// struct integer {};     // symbol is integer
// struct even {};        // symbol is even integer
// struct odd {};         // symbol is odd integer
// struct rational {};    // symbol is rational
// struct irrational {};  // symbol is irrational
// struct real {};        // symbol is real
// struct complex {};     // symbol is complex
// struct prime {};       // symbol is prime

// namespace detail {

// struct assumption_comparator {
//   template <typename LHS, typename RHS>
//   bool operator()(const LHS &lhs, const RHS &rhs) const {
//     return std::visit(
//         [](const auto &left, const auto &right) {
//           return std::type_index(typeid(left)) <
//           std::type_index(typeid(right));
//         },
//         lhs, rhs);
//   }
// };

// // === Define the Assumption Type ===
// using assumption_variant =
//     std::variant<positive, negative, nonzero, nonnegative, nonpositive,
//     integer,
//                  even, odd, rational, irrational, real, complex, prime
//                  //,                  symmetric, skewsymmetric
//                  >;

// using assumption_set = std::set<assumption_variant, assumption_comparator>;

// } // namespace detail

// // tensor = 1/size(seq) sum_seq basis_change<seq_i>(tensor)
// struct symmetric {
//   std::vector<sequence> m_sequences;
// };

// struct skewsymmetric {}; // tensor = tensor - (tensor)^T 2th order tensors
// struct volumetric {};    // tensor = vol(tensor) + dev(tensor) 2th order
// tensors struct deviatoric {};    // tensor = vol(tensor) + dev(tensor) 2th
// order tensors using tensor_space =
//     std::variant<symmetric, skewsymmetric, volumetric, deviatoric>;

// using tensor_space_set = std::set<tensor_space,
// detail::assumption_comparator>;

// namespace relation {
// struct equal {};         // a == b
// struct not_equal {};     // a != b
// struct greater {};       // a >  b
// struct greater_equal {}; // a >= b
// struct less {};          // a <  b
// struct less_equal {};    // a <= b

// using relation_variant =
//     std::variant<equal, not_equal, greater, greater_equal, less, less_equal>;

// template <typename LHS, typename RHS> class relation {
//   template <typename Relation>
//   relation(LHS lhs, RHS rhs, Relation type) : lhs(lhs), rhs(rhs), type(type)
//   {}

//   // Operator < to allow storage in ordered sets
//   bool operator<(const relation &other) const {
//     return std::tie(lhs, rhs, type) <
//            std::tie(other.lhs, other.rhs, other.type);
//   }

// private:
//   expression_holder<LHS> lhs;
//   expression_holder<RHS> rhs;
//   relation_variant type;
// };

// template <typename LHS, typename RHS>
// using relation_set = std::set<relation<LHS, RHS>>;
// } // namespace relation

// // template <typename SymbolType> class assumption_manager {
// // private:
// //   detail::assumption_set m_assumption_set;
// //   relation::relation_set<SymbolType, SymbolType>
// //       relation_set; // Store symbolic relations

// // public:
// //   // Add an assumption
// //   template <typename Assumption> void add_assumption(Assumption &&a) {
// //     m_assumption_set.insert(a);
// //   }

// //         // Remove an assumption
// //  template <typename Assumption> void remove_assumption(Assumption &&a) {
// //    m_assumption_set.erase(a);
// //  }

// //         // Check if an assumption exists
// //  template <typename Assumption> bool has_assumption(Assumption &&a) const
// {
// //    return m_assumption_set.contains(a);
// //  }

// //         // Add a relation
// //  template <typename Relation>
// //  void add_relation(SymbolType lhs, SymbolType rhs, Relation type) {
// //    relation_set.insert(
// //        relation::relation<SymbolType, SymbolType>(lhs, rhs, type));
// //  }

// //         // Check if a relation holds
// //  template <typename Relation>
// //  bool check_relation(SymbolType lhs, SymbolType rhs, Relation type) const
// {
// //    return relation_set.find(relation::relation<SymbolType, SymbolType>(
// //               lhs, rhs, type)) != relation_set.end();
// //  }
// //};
// //} // namespace numsim::cas

#endif // ASSUMPTIONS_H
