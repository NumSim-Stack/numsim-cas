#ifndef SIMPLIFY_RULE_REGISTRY_H
#define SIMPLIFY_RULE_REGISTRY_H

#include <unordered_map>
#include <functional>
#include "expression_holder.h"

namespace symTM {

template<typename T, template<typename> class ExprType>
class simplify_rule_registry {
public:
  using expr_t = expression_holder<ExprType<T>>;
  using RuleFn = std::function<expr_t(expr_t const&, expr_t const&)>;

  using Key = type_id;

  static simplify_rule_registry& instance() {
    static simplify_rule_registry inst;
    return inst;
  }

  template<typename... Expr>
  void register_rule(RuleFn fn, bool commutative = true, int priority = 0) {
    auto key = hash_variadic(Expr::get_id()...);
    rule_table_[key] = {std::move(fn), commutative, priority};
  }

  template<typename... Numbers/*, std::enable_if_t<(std::is_integral_v<Numbers>,...),bool>*/>
  auto get_key(Numbers ...numbers){
    return hash_variadic(numbers...);
  }

//  template<typename... Expr>
//  auto get_key(Expr const& ...expr){
//    return hash_variadic(get_id(expr)...);
//  }

  template<typename... Expr>
  auto get_key(){
    return hash_variadic(Expr::get_id()...);
  }

//  template<typename... Expr>
//  auto run(Expr const& ...expr){
//    return rule_table_.find(get_key(expr...))->second.fn(expr...);
//  }

  template<typename... Expr>
  auto run(Key key, Expr const& ...expr){
    return rule_table_.find(key)->second.fn(expr...);
  }

  //  template<typename LHS, typename RHS>
  //  void register_rule(RuleFn fn, bool commutative = true, int priority = 0) {
  //    //auto key = make_key<LHS, RHS>();
  //    rule_table_[LHS::get_id()][RHS::get_id()] = {std::move(fn), commutative, priority};
  //  }

  template<typename... Indices>
  /*std::optional<RuleFn>*/ RuleFn lookup(Indices ... indices) {
    //    Key key{lhs_id, rhs_id};
    const auto key{hash_variadic(indices...)};

    return rule_table_.find(key)->second.fn;
    // Direct match
    //    auto it = rule_table_.find(key);
    //    if (it != rule_table_.end()) return it->second.fn;

    //    // Check flipped key if operation is commutative
    //    Key flipped{rhs_id, lhs_id};
    //    auto flip = rule_table_.find(flipped);
    //    if (flip != rule_table_.end() && flip->second.commutative) {
    //      return [=](expr_t const& lhs, expr_t const& rhs) {
    //        return flip->second.fn(rhs, lhs);
    //      };
    //    }

    //    return std::nullopt;
  }

  template<typename Stream>
  void print_rules(Stream & stream) const {
    for (const auto& [key, meta] : rule_table_) {
      stream << "Rule for (" << key.first << ", " << key.second
                << ") [commutative=" << meta.commutative
                << ", priority=" << meta.priority << "]\n";
    }
  }

private:
  simplify_rule_registry(){}

  template<typename LHS, typename RHS>
  Key make_key() const {
    return {LHS::get_id(), RHS::get_id()};
  }

  struct RuleMeta {
    RuleFn fn;
    bool commutative;
    int priority;
  };

  inline void hash_combine([[maybe_unused]]std::size_t& seed) {}

  template <typename _T, typename... Rest>
  inline void hash_combine(std::size_t& seed, const _T& v, const Rest&... rest) {
    std::hash<_T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    hash_combine(seed, rest...);
  }

  template <typename... Args>
  std::size_t hash_variadic(const Args&... args) {
    std::size_t seed = 0;
    hash_combine(seed, args...);
    return seed;
  }

  std::unordered_map<Key, RuleMeta> rule_table_;
};

}

#endif // SIMPLIFY_RULE_REGISTRY_H
