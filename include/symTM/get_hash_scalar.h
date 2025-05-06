#ifndef GET_HASH_SCALAR_H
#define GET_HASH_SCALAR_H

#include <vector>
#include <algorithm>
#include <utility>
#include <assert.h>
#include "scalar/scalar_negativ.h"

namespace symTM {

class get_hash_value_scalar
{
public:
  get_hash_value_scalar() {}
  template<typename T>
  auto operator()(T const& expr){
    return expr.hash_value();
  }
  template<typename T>
  auto operator()(scalar_negative<T> const& expr){
    return expr.expr().get().hash_value();
  }
};

template<typename T>
auto get_hash_value(expression_holder<scalar_expression<T>> const& expr){
  return visit(get_hash_value_scalar(), *expr);
}

class get_hash_value_tensor
{
public:
  get_hash_value_tensor() {}
  template<typename T>
  auto operator()(T const& expr){
    return expr.hash_value();
  }
  template<typename T>
  auto operator()(tensor_negative<T> const& expr){
    return expr.expr().get().hash_value();
  }
};

template<typename T>
auto get_hash_value(expression_holder<tensor_expression<T>> const& expr){
  return visit(get_hash_value_tensor(), *expr);
}


template<typename T>
auto get_hash_value(expression_holder<tensor_to_scalar_expression<T>> const& expr){
  return expr.get().hash_value();
}
}



//std::size_t orderedHash(const std::string& s) {
//  constexpr std::size_t base{53};
//  std::size_t hashValue{0};
//  for (const auto& ch : s) {
//    std::size_t charValue{0};
//    if (ch >= 'a' && ch <= 'z') {
//      charValue = ch - 'a' + 1; // 'a' = 1, 'b' = 2, ..., 'z' = 26
//    }
//    if (ch >= 'A' && ch <= 'Z') {
//      charValue = ch - 'A' + 27; // 'A' = 27, ..., 'Z' = 52
//    }
//    // Build a numerical representation based on base-53 encoding
//    hashValue += charValue*base;
//  }
//  return hashValue;
//}



//template<typename Base>
//class symbol_base;

//template <typename Type>
//class get_hash_imp;

//template <typename ValueType>
//class get_hash
//{
//public:
//  get_hash() {}
//  inline std::size_t operator()(expression_holder<scalar_expression<ValueType>> visitor){
//    return std::visit(*this, *visitor);
//  }

//  template<typename Base>
//  inline std::size_t operator()(symbol_base<Base> const& visitor){
//    std::size_t seed{0};
//    hash_combine(seed, visitor.name());
//    return seed;
//  }

//  inline std::size_t operator()(scalar_add<ValueType> const& visitor){
//    std::size_t seed{0};
//    if(visitor.size() > 1){
//      // Unique constant for scalar_add
//      constexpr std::size_t add_tag{0x12345678};
//      hash_combine(seed, add_tag);
//    }
//    return loop(visitor, seed);
//  }

//  inline std::size_t operator()(scalar_mul<ValueType> const& visitor){
//    std::size_t seed{0};
//    if(visitor.size() > 1){
//      // Unique constant for scalar_mul
//      constexpr std::size_t mul_tag{0x87654321};
//      hash_combine(seed, mul_tag);
//    }
//    return loop(visitor, seed);
//  }

//  inline std::size_t operator()(scalar_sub<ValueType> const& visitor){
//    std::size_t seed{0};
//    if(visitor.size() > 1){
//      // Unique constant for scalar_sub
//      constexpr std::size_t mul_tag{0x54321876};
//      hash_combine(seed, mul_tag);
//    }
//    return loop(visitor, seed);
//  }

//  inline std::size_t operator()(tensor_scalar_mul<ValueType> const& visitor){
//    std::size_t seed{0};
//    hash_combine(seed, visitor.expr_lhs().get().hash_value());
//    hash_combine(seed, visitor.expr_rhs().get().hash_value());
//    return seed;
//  }

//  inline std::size_t operator()(tensor_add<ValueType> const& visitor){
//    std::size_t seed{0};
//    if(visitor.size() > 1){
//      // Unique constant for tensor_add
//      constexpr std::size_t mul_tag{0x87654329};
//      hash_combine(seed, mul_tag);
//    }
//    return loop(visitor, seed);
//  }

//  inline std::size_t operator()(scalar<ValueType> const& visitor){
//    //using this_symbol_base = typename scalar<ValueType>::base_expr;
//    //return std::hash<const this_symbol_base*>()(static_cast<const this_symbol_base*>(&visitor));
//    return visitor.hash_value();
//  }

//  inline std::size_t operator()(tensor<ValueType> const& visitor){
//    return visitor.hash_value();
//  }

//  inline std::size_t operator()(scalar_constant<ValueType> const& visitor){
//    std::size_t seed{0};
//    hash_combine(seed, visitor());
//    return seed;
//  }

//  template<typename T>
//  inline std::size_t operator()([[maybe_unused]] T const& visitor){
//    assert(0);
//    return 0;
//  }

//private:
//  template<typename ExprType, typename Derived>
//  inline std::size_t loop(n_ary_tree<ExprType, Derived> const& visitor, std::size_t seed){
//    // Gather and hash elements in the base vector (n_ary_tree)
//    std::vector<std::size_t> child_hashes;
//    child_hashes.reserve(visitor.size());

//    for (const auto& child : visitor.hash_map() | std::views::values) {
//      // Assuming each child is wrapped in a variant and supports std::visit
//      child_hashes.push_back(std::visit(*this, *child));
//    }
//    //for a single entry return this
//    if(child_hashes.size() == 1){
//      return child_hashes.front();
//    }
//    // Sort for commutative operations like addition
//    std::stable_sort(child_hashes.begin(), child_hashes.end());
//    // Combine all child hashes
//    for (const auto& child_hash : child_hashes) {
//      hash_combine(seed, child_hash);
//    }
//    return seed;
//  }
//};
//}








//template <typename ValueType>
//struct std::hash<symTM::expression_holder<symTM::scalar_expression<ValueType>>> {
//  std::size_t operator()(const symTM::expression_holder<symTM::scalar_expression<ValueType>>& add_expr) const {
//    return std::visit(symTM::get_hash<ValueType>(), *add_expr);
//  }
//};


//template <typename ExprType>
//struct std::hash<symTM::n_ary_tree<ExprType>> {
//  std::size_t operator()(const symTM::n_ary_tree<ExprType>& add_expr) const {
//    return std::visit(symTM::get_hash<ValueType>(), *add_expr);
//  }
//};


#endif // GET_HASH_SCALAR_H
