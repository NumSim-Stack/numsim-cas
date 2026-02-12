#ifndef TENSOR_H
#define TENSOR_H

#include <numsim_cas/core/symbol_base.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <ostream>

namespace numsim::cas {

class tensor final : public symbol_base<tensor_node_base_t<tensor>> {
public:
  using base = symbol_base<tensor_node_base_t<tensor>>;
  using base::operator=;

  tensor() = delete;
  tensor(std::string const &name, std::size_t dim, std::size_t rank)
      : base(name, dim, rank) {}

  tensor(tensor &&data) : base(data.m_name, data.m_dim, data.m_rank) {}

  // const tensor &operator=(expression_holder<tensor_expression> &&data) {
  //   this->m_expr = std::move(data);
  //   return *this;
  // }

  // const tensor &operator+=(expression_holder<tensor_expression> &&data) {
  //   *this = std::move(data) + *this;
  //   return *this;
  // }

  // const tensor &operator-=(expression_holder<tensor_expression> &&data) {
  //   *this = std::move(data) - *this;
  //   return *this;
  // }

  // template <typename Dervied>
  // const tensor &operator=(tmech::tensor_base<Dervied> const &data) {
  //   constexpr auto Dim{Dervied::dimension()};
  //   constexpr auto Rank{Dervied::rank()};
  //   assert(this->m_dim == Dim);
  //   assert(this->m_rank == Rank);
  //   return *this;
  // }

  // friend std::ostream &operator<<(std::ostream &os, const tensor &dt);
};

// std::ostream &operator<<(std::ostream &os, [[maybe_unused]] const tensor &t)
// {
//   os << "tensor";
//   return os;
// }

struct call_tensor {
  template <typename Tensor>
  static constexpr inline auto dim(Tensor const &tensor) {
    return tensor.dim();
  }

  template <typename Tensor>
  static constexpr inline auto rank(Tensor const &tensor) {
    return tensor.rank();
  }

  static constexpr inline auto
  dim(expression_holder<tensor_expression> const &tensor) {
    return tensor.get().dim();
  }

  static constexpr inline auto
  rank(expression_holder<tensor_expression> const &tensor) {
    return tensor.get().rank();
  }
};

// ast::unique_ptr<expression> operator+(tensor &lhs, tensor &rhs);

// ast::unique_ptr<expression> operator+(ast::unique_ptr<expression> &&lhs,
//                                       tensor &rhs);

// ast::unique_ptr<expression> operator+(ast::unique_ptr<expression> &&lhs,
//                                       ast::unique_ptr<expression> &&rhs);

// ast::unique_ptr<expression> operator+(ast::unique_ptr<tensor_add> &&lhs,
//                                       ast::unique_ptr<tensor_expression>
//                                       &&rhs);

// ast::unique_ptr<expression> operator-(tensor &lhs, tensor &rhs);

// ast::unique_ptr<expression> operator-(ast::unique_ptr<expression> &&lhs,
//                                       tensor &rhs);

// ast::unique_ptr<expression> operator-(ast::unique_ptr<expression> &&lhs,
//                                       ast::unique_ptr<expression> &&rhs);

// ast::unique_ptr<expression> operator-(ast::unique_ptr<tensor_add> &&lhs,
//                                       ast::unique_ptr<tensor_expression>
//                                       &&rhs);

// ast::unique_ptr<expression> operator+(tensor &lhs, tensor &rhs) {
//   auto op{ast::make_unique<tensor_add>(lhs.dim(), rhs.rank())};
//   op.get()->add_child(&lhs);
//   op.get()->add_child(&rhs);
//   return std::move(op);
// }

// ast::unique_ptr<expression> operator+(ast::unique_ptr<expression> &&lhs,
//                                       tensor &rhs) {
//   if (auto ptr = dynamic_cast<tensor_add *>(lhs.get()))
//     ptr->add_child(&rhs);
//   return std::move(lhs);
// }

// ast::unique_ptr<expression> operator+(ast::unique_ptr<expression> &&lhs,
//                                       ast::unique_ptr<expression> &&rhs) {
//   if (auto ptr = dynamic_cast<n_ary_tree *>(lhs.get())) {
//     // ptr->add_child(std::move(lhs));
//     ptr->add_child(std::move(rhs));
//     return std::move(lhs);
//   } else {
//     auto tensor = static_cast<tensor_expression *>(lhs.get());
//     auto op{ast::make_unique<tensor_add>(tensor->dim(), tensor->rank())};
//     op.get()->add_child(std::move(lhs));
//     op.get()->add_child(std::move(rhs));
//     return std::move(op);
//   }
// }

// ast::unique_ptr<expression>
// operator+(ast::unique_ptr<tensor_add> &&lhs,
//           ast::unique_ptr<tensor_expression> &&rhs) {
//   lhs.get()->add_child(std::move(rhs));
//   return std::move(lhs);
// }

//// ---------------------------------------

// ast::unique_ptr<expression> operator-(tensor &lhs, tensor &rhs) {
//   auto op{ast::make_unique<tensor_add>(lhs.dim(), rhs.rank())};
//   op.get()->add_child(&lhs);
//   op.get()->add_child(std::make_unique<tensor_negative>(&rhs));
//   return std::move(op);
// }

// ast::unique_ptr<expression> operator-(ast::unique_ptr<expression> &&lhs,
//                                       tensor &rhs) {
//   if (auto ptr = dynamic_cast<tensor_add *>(lhs.get()))
//     ptr->add_child(std::make_unique<tensor_negative>(&rhs));
//   return std::move(lhs);
// }

// ast::unique_ptr<expression> operator-(ast::unique_ptr<expression> &&lhs,
//                                       ast::unique_ptr<expression> &&rhs) {
//   if (auto ptr = dynamic_cast<n_ary_tree *>(lhs.get())) {
//     // ptr->add_child(std::move(lhs));
//     auto expr{std::make_unique<tensor_negative>(std::move(rhs))};
//     ptr->add_child(std::move(expr));
//     return std::move(lhs);
//   } else {
//     auto tensor = static_cast<tensor_expression *>(lhs.get());
//     auto op{ast::make_unique<tensor_add>(tensor->dim(), tensor->rank())};
//     op.get()->add_child(std::move(lhs));
//     auto expr{std::make_unique<tensor_negative>(std::move(rhs))};
//     op.get()->add_child(std::move(expr));
//     return std::move(op);
//   }
// }

// ast::unique_ptr<expression>
// operator-(ast::unique_ptr<tensor_add> &&lhs,
//           ast::unique_ptr<tensor_expression> &&rhs) {
//   auto expr{std::make_unique<tensor_negative>(std::move(rhs))};
//   lhs.get()->add_child(std::move(expr));
//   return std::move(lhs);
// }
//  ---------------------------------------

} // namespace numsim::cas

#endif // TENSOR_H
