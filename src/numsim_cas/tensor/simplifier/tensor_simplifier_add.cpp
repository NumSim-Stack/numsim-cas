#include <numsim_cas/functions.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/tensor/simplifier/tensor_simplifier_add.h>
#include <numsim_cas/tensor/tensor_operators.h>

namespace numsim::cas {
namespace simplifier {
namespace tensor_detail {

n_ary_add::n_ary_add(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      lhs(m_lhs.template get<tensor_add>()) {}

//  inline expr_holder_t
//  dispatch([[maybe_unused]]tensor_constant const& rhs){
//    auto add_expr{make_expression<tensor_add>(lhs)};
//    auto &add{add_expr.template get<tensor_add>()};
//    auto coeff{add.coeff() + m_rhs};
//    add.set_coeff(std::move(coeff));
//    return std::move(add_expr);
//  }

//  inline expr_holder_t
//  dispatch([[maybe_unused]]tensor_one const& ){
//    auto add_expr{make_expression<tensor_add>(lhs)};
//    auto &add{add_expr.template get<tensor_add>()};
//    auto
//    coeff{make_expression<scalar_constant>(get_coefficient(add,
//    0.0) + static_cast(1))};
//    //auto coeff{add.coeff() + m_rhs};
//    add.set_coeff(std::move(coeff));
//    return add_expr;
//  }

template <typename Expr>
[[nodiscard]] auto n_ary_add::dispatch([[maybe_unused]] Expr const &rhs) {
  /// do a deep copy of data
  auto expr_add{make_expression<tensor_add>(lhs)};
  auto &add{expr_add.template get<tensor_add>()};
  /// check if sub_exp == expr_rhs for sub_exp \in expr_lhs
  auto pos{lhs.hash_map().find(m_rhs)};
  if (pos != lhs.hash_map().end()) {
    add.hash_map().erase(m_rhs);
    add.push_back(pos->second + m_rhs);
    return expr_add;
  }
  /// no equal expr or sub_expr
  add.push_back(m_rhs);
  return expr_add;
}

// merge two expression
[[nodiscard]] auto n_ary_add::dispatch(tensor_add const &rhs) {
  auto expr{make_expression<tensor_add>(rhs.dim(), rhs.rank())};
  auto &add{expr.template get<tensor_add>()};
  merge_add(lhs, rhs, add);
  return expr;
}

[[nodiscard]] auto n_ary_add::dispatch(tensor_negative const &rhs) {
  const auto &expr_rhs{rhs.expr()};
  const auto pos{lhs.hash_map().find(expr_rhs)};
  if (pos != lhs.hash_map().end()) {
    auto expr{make_expression<tensor_add>(lhs)};
    auto &add{expr.template get<tensor_add>()};
    add.hash_map().erase(expr_rhs);
    return expr;
  }
  return get_default();
}

// template<typename T>
// class n_ary_mul_add final : public add_default<T>{
// public:
//   using value_type = T;
//   using expr_holder_t = expression_holder<scalar_expression>;
//   using base = add_default<T>;
//   using base::dispatch;
//   using base::get_default;
//   using base::get_coefficient;

//  n_ary_mul_add(expr_holder_t lhs, expr_holder_t
//  rhs):base(lhs,rhs),lhs{base::m_lhs.template get<scalar_mul>()}
//  {}

//  auto dispatch(scalar const&rhs) {
//    const auto &hash_rhs{rhs.hash_value()};
//    const auto &hash_lhs{lhs.hash_value()};
//    if (hash_rhs == hash_lhs) {
//      auto expr{make_expression<scalar_mul>(lhs)};
//      auto &mul{expr.template get<scalar_mul>()};
//      mul.set_coeff(make_expression<scalar_constant>(
//          get_coefficient(lhs, 1.0) + 1.0));
//      return std::move(expr);
//    }
//    return get_default();
//  }

//         /// expr + expr --> 2*expr
//  auto dispatch(scalar_mul const&rhs) {
//    const auto &hash_rhs{rhs.hash_value()};
//    const auto &hash_lhs{lhs.hash_value()};
//    if (hash_rhs == hash_lhs) {
//      const auto fac_lhs{get_coefficient(lhs, 1.0)};
//      const auto fac_rhs{get_coefficient(rhs, 1.0)};
//      auto expr{make_expression<scalar_mul>(lhs)};
//      auto &mul{expr.template get<scalar_mul>()};
//      mul.set_coeff(
//          make_expression<scalar_constant>(fac_lhs + fac_rhs));
//      return std::move(expr);
//    }
//    return get_default();
//  }

// private:
//   using base::m_lhs;
//   using base::m_rhs;
//   scalar_mul const& lhs;
// };

symbol_add::symbol_add(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      lhs{base::m_lhs.template get<tensor>()} {}

/// x+x --> 2*x
[[nodiscard]] inline symbol_add::expr_holder_t
symbol_add::dispatch(tensor const &rhs) {
  if (&lhs == &rhs) {
    auto new_expr{make_expression<tensor_scalar_mul>(
        make_expression<scalar_constant>(2), m_rhs)};
    return new_expr;
  }
  return get_default();
}

//  inline expr_holder_t  dispatch(scalar_mul const&rhs) {
//    const auto &hash_rhs{rhs.hash_value()};
//    const auto &hash_lhs{lhs.hash_value()};
//    if (hash_rhs == hash_lhs) {
//      auto expr{make_expression<scalar_mul>(rhs)};
//      auto &mul{expr.template get<scalar_mul>()};
//      mul.set_coeff(make_expression<scalar_constant>(
//          get_coefficient(rhs, 1.0) + 1.0));
//      return std::move(expr);
//    }
//    return get_default();
//  }
//  inline expr_holder_t dispatch(scalar_constant
//  const&rhs) {
//  }

// tensor_scalar_mul_add

tensor_scalar_mul_add::tensor_scalar_mul_add(expr_holder_t lhs,
                                             expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      lhs{base::m_lhs.template get<tensor_scalar_mul>()} {}

// scalar_expr * lhs + rhs
template <typename Expr>
[[nodiscard]] inline tensor_scalar_mul_add::expr_holder_t
tensor_scalar_mul_add::dispatch(Expr const &rhs) {
  if (lhs.expr_rhs() == m_rhs) {
    return (lhs.expr_lhs() + 1) * m_rhs;
  }
  return get_default();
}

// scalar_expr_lhs * lhs + scalar_expr_rhs * rhs
[[nodiscard]] inline tensor_scalar_mul_add::expr_holder_t
tensor_scalar_mul_add::dispatch(tensor_scalar_mul const &rhs) {
  if (lhs.expr_rhs().get().hash_value() == rhs.expr_rhs().get().hash_value()) {
    return (lhs.expr_lhs() + rhs.expr_lhs()) * rhs.expr_rhs();
  }
  return get_default();
}

// add_negative
add_negative::add_negative(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      lhs{base::m_lhs.template get<tensor_negative>()} {}

// (-lhs) + (-rhs) --> -(lhs+rhs)
[[nodiscard]] inline add_negative::expr_holder_t
add_negative::dispatch(tensor_negative const &rhs) {
  return -(lhs.expr() + rhs.expr());
}

// add_base
[[nodiscard]] inline add_base::expr_holder_t
add_base::dispatch(tensor_zero const &) {
  return std::move(m_rhs);
}

[[nodiscard]] inline add_base::expr_holder_t
add_base::dispatch(tensor_add const &) {
  auto &_rhs{m_rhs.template get<tensor_visitable_t>()};
  n_ary_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

[[nodiscard]] inline add_base::expr_holder_t
add_base::dispatch(tensor_scalar_mul const &) {
  auto &_rhs{m_rhs.template get<tensor_visitable_t>()};
  tensor_scalar_mul_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

[[nodiscard]] inline add_base::expr_holder_t
add_base::dispatch(tensor_negative const &) {
  auto &_rhs{m_rhs.template get<tensor_visitable_t>()};
  add_negative visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

[[nodiscard]] inline add_base::expr_holder_t
add_base::dispatch(tensor const &) {
  auto &_rhs{m_rhs.template get<tensor_visitable_t>()};
  symbol_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

template <typename Type>
[[nodiscard]] inline add_base::expr_holder_t add_base::dispatch(Type const &) {
  auto &_rhs{m_rhs.template get<tensor_visitable_t>()};
  add_default<void> visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

} // namespace tensor_detail
} // namespace simplifier
} // namespace numsim::cas
