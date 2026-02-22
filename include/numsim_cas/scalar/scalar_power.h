#ifndef SCALAR_POWER_H
#define SCALAR_POWER_H

#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_pow final : public binary_op<scalar_node_base_t<scalar_pow>> {
public:
  using base = binary_op<scalar_node_base_t<scalar_pow>>;

  using base::base;
  scalar_pow(scalar_pow const &expr) : base(static_cast<base const &>(expr)) {}
<<<<<<< HEAD
  scalar_pow(scalar_pow &&expr) noexcept
      : base(std::move(static_cast<base &&>(expr))) {}
=======
  scalar_pow(scalar_pow &&expr) noexcept : base(std::move(static_cast<base &&>(expr))) {}
>>>>>>> origin/move_to_virtual
  scalar_pow() = delete;
  ~scalar_pow() override = default;
  const scalar_pow &operator=(scalar_pow &&) = delete;
};

// template <typename T, typename... Args>
// struct update_hash<
//     numsim::cas::binary_op<numsim::cas::scalar_pow<T>, Args...>> {
//   using type_t = numsim::cas::binary_op<numsim::cas::scalar_pow<T>, Args...>;
//   std::size_t operator()(const type_t &expr) const noexcept {
//     std::size_t seed{0};
//     numsim::cas::hash_combine(seed, type_t::get_id());
////    if (!numsim::cas::is_same<numsim::cas::scalar_constant<T>>(
////            expr.expr_rhs())) {
//      numsim::cas::hash_combine(seed, expr.expr_lhs().get().hash_value());
//      numsim::cas::hash_combine(seed, expr.expr_rhs().get().hash_value());
////    } else {
////      numsim::cas::hash_combine(seed, expr.expr_lhs().get().hash_value());
////    }
//    return seed;
//  }
//};
} // namespace numsim::cas

#endif // SCALAR_POWER_H
