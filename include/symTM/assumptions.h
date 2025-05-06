#ifndef ASSUMPTIONS_H
#define ASSUMPTIONS_H

#include <set>
#include <typeindex>
#include <typeinfo>
#include <variant>

namespace symTM {
namespace assumptions {

struct positive {};      // symbol > 0
struct negative {};      // symbol < 0
struct nonzero {};       // symbol /= 0
struct nonnegative {};   // symbol >= 0
struct nonpositive {};   // symbol <= 0
struct integer {};       // symbol is integer
struct even {};          // symbol is even integer
struct odd {};           // symbol is odd integer
struct rational {};      // symbol is rational
struct irrational {};    // symbol is irrational
struct real {};          // symbol is real
struct complex {};       // symbol is complex
struct prime {};         // symbol is prime
struct symmetric {};     // tensor = tensor + (tensor)^T 2th order tensors
struct skewsymmetric {}; // tensor = tensor - (tensor)^T 2th order tensors

// === Comparator for Assumption Set ===
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
                 even, odd, rational, irrational, real, complex, prime,
                 symmetric, skewsymmetric>;

// === Define Assumption Set ===
using assumption_set = std::set<assumption_variant, assumption_comparator>;

} // namespace assumptions

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
  LHS lhs;
  RHS rhs;
  relation_variant type;
};

template <typename LHS, typename RHS>
using relation_set = std::set<relation<LHS, RHS>>;
} // namespace relation

// === Generalized Assumption Manager Class ===
template <typename SymbolType> class assumption_manager {
private:
  assumptions::assumption_set m_assumption_set;
  relation::relation_set<SymbolType, SymbolType>
      relation_set; // Store symbolic relations

public:
  // Add an assumption
  template <typename Assumption> void add_assumption(Assumption &&a) {
    m_assumption_set.insert(a);
  }

  // Remove an assumption
  template <typename Assumption> void remove_assumption(Assumption &&a) {
    m_assumption_set.erase(a);
  }

  // Check if an assumption exists
  template <typename Assumption> bool has_assumption(Assumption &&a) const {
    return m_assumption_set.contains(a);
  }

  // Add a relation
  template <typename Relation>
  void add_relation(SymbolType lhs, SymbolType rhs, Relation type) {
    relation_set.insert(
        relation::relation<SymbolType, SymbolType>(lhs, rhs, type));
  }

  // Check if a relation holds
  template <typename Relation>
  bool check_relation(SymbolType lhs, SymbolType rhs, Relation type) const {
    return relation_set.find(relation::relation<SymbolType, SymbolType>(
               lhs, rhs, type)) != relation_set.end();
  }
};

} // namespace symTM

#endif // ASSUMPTIONS_H
