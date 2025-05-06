#ifndef TENSOR_H
#define TENSOR_H

#include <ostream>
#include "tensor_expression.h"
#include "../symbol_base.h"
#include "tensor_data.h"
#include "../symTM_type_traits.h"
#include "../functions.h"
#include "tensor_functions.h"
#include "tensor_negative.h"

namespace numsim::cas {

template<typename ValueType>
class tensor final : public symbol_base<expression_crtp<tensor<ValueType>, tensor_expression<ValueType>>> {
public:
  using base = symbol_base<expression_crtp<tensor<ValueType>, tensor_expression<ValueType>>>;
  using base::operator=;

  tensor() = delete;
  tensor(std::string const &name, std::size_t dim, std::size_t rank)
      : base(name, dim, rank),
        m_data(make_tensor_data<ValueType>(dim, rank))
  {}

  tensor(tensor && data)
      : base(data.m_name, data.m_dim, data.m_rank),
        m_data(std::move(data.m_data))
  {}

  const tensor &operator=(expression_holder<tensor_expression<ValueType>> &&data) {
    this->m_expr = std::move(data);
    return *this;
  }

  const tensor &operator+=(expression_holder<tensor_expression<ValueType>> &&data) {
    *this = std::move(data) + *this;
    return *this;
  }

  const tensor &operator-=(expression_holder<tensor_expression<ValueType>> &&data) {
    *this = std::move(data) - *this;
    return *this;
  }

  template <typename Dervied>
  const tensor &operator=(tmech::tensor_base<Dervied> const &data) {
    constexpr auto Dim{Dervied::dimension()};
    constexpr auto Rank{Dervied::rank()};
    assert(this->m_dim == Dim);
    assert(this->m_rank == Rank);
    static_cast<tensor_data<ValueType, Dim, Rank> &>(*m_data.get())
        .data() = data.convert();
    return *this;
  }


  inline auto operator-(){
    return make_expression<tensor_expression<ValueType>, tensor_negative<ValueType>>(this);
  }

  template<typename T>
  friend std::ostream& operator<<(std::ostream& os, const tensor<T>& dt);

  inline tensor_data_base<ValueType>& data(){
    return *m_data.get();
  }

private:
  std::unique_ptr<tensor_data_base<ValueType>> m_data;
};

template<typename ValueType>
std::ostream& operator<<(std::ostream& os, const tensor<ValueType>& t){
  os<<"tensor";
  return os;
}

//ast::unique_ptr<expression> operator+(tensor &lhs, tensor &rhs);

//ast::unique_ptr<expression> operator+(ast::unique_ptr<expression> &&lhs,
//                                      tensor &rhs);

//ast::unique_ptr<expression> operator+(ast::unique_ptr<expression> &&lhs,
//                                      ast::unique_ptr<expression> &&rhs);

//ast::unique_ptr<expression> operator+(ast::unique_ptr<tensor_add> &&lhs,
//                                      ast::unique_ptr<tensor_expression> &&rhs);

//ast::unique_ptr<expression> operator-(tensor &lhs, tensor &rhs);

//ast::unique_ptr<expression> operator-(ast::unique_ptr<expression> &&lhs,
//                                      tensor &rhs);

//ast::unique_ptr<expression> operator-(ast::unique_ptr<expression> &&lhs,
//                                      ast::unique_ptr<expression> &&rhs);

//ast::unique_ptr<expression> operator-(ast::unique_ptr<tensor_add> &&lhs,
//                                      ast::unique_ptr<tensor_expression> &&rhs);

//ast::unique_ptr<expression> operator+(tensor &lhs, tensor &rhs) {
//  auto op{ast::make_unique<tensor_add>(lhs.dim(), rhs.rank())};
//  op.get()->add_child(&lhs);
//  op.get()->add_child(&rhs);
//  return std::move(op);
//}

//ast::unique_ptr<expression> operator+(ast::unique_ptr<expression> &&lhs,
//                                      tensor &rhs) {
//  if (auto ptr = dynamic_cast<tensor_add *>(lhs.get()))
//    ptr->add_child(&rhs);
//  return std::move(lhs);
//}

//ast::unique_ptr<expression> operator+(ast::unique_ptr<expression> &&lhs,
//                                      ast::unique_ptr<expression> &&rhs) {
//  if (auto ptr = dynamic_cast<n_ary_tree *>(lhs.get())) {
//    // ptr->add_child(std::move(lhs));
//    ptr->add_child(std::move(rhs));
//    return std::move(lhs);
//  } else {
//    auto tensor = static_cast<tensor_expression *>(lhs.get());
//    auto op{ast::make_unique<tensor_add>(tensor->dim(), tensor->rank())};
//    op.get()->add_child(std::move(lhs));
//    op.get()->add_child(std::move(rhs));
//    return std::move(op);
//  }
//}

//ast::unique_ptr<expression>
//operator+(ast::unique_ptr<tensor_add> &&lhs,
//          ast::unique_ptr<tensor_expression> &&rhs) {
//  lhs.get()->add_child(std::move(rhs));
//  return std::move(lhs);
//}

//// ---------------------------------------

//ast::unique_ptr<expression> operator-(tensor &lhs, tensor &rhs) {
//  auto op{ast::make_unique<tensor_add>(lhs.dim(), rhs.rank())};
//  op.get()->add_child(&lhs);
//  op.get()->add_child(std::make_unique<tensor_negative>(&rhs));
//  return std::move(op);
//}

//ast::unique_ptr<expression> operator-(ast::unique_ptr<expression> &&lhs,
//                                      tensor &rhs) {
//  if (auto ptr = dynamic_cast<tensor_add *>(lhs.get()))
//    ptr->add_child(std::make_unique<tensor_negative>(&rhs));
//  return std::move(lhs);
//}

//ast::unique_ptr<expression> operator-(ast::unique_ptr<expression> &&lhs,
//                                      ast::unique_ptr<expression> &&rhs) {
//  if (auto ptr = dynamic_cast<n_ary_tree *>(lhs.get())) {
//    // ptr->add_child(std::move(lhs));
//    auto expr{std::make_unique<tensor_negative>(std::move(rhs))};
//    ptr->add_child(std::move(expr));
//    return std::move(lhs);
//  } else {
//    auto tensor = static_cast<tensor_expression *>(lhs.get());
//    auto op{ast::make_unique<tensor_add>(tensor->dim(), tensor->rank())};
//    op.get()->add_child(std::move(lhs));
//    auto expr{std::make_unique<tensor_negative>(std::move(rhs))};
//    op.get()->add_child(std::move(expr));
//    return std::move(op);
//  }
//}

//ast::unique_ptr<expression>
//operator-(ast::unique_ptr<tensor_add> &&lhs,
//          ast::unique_ptr<tensor_expression> &&rhs) {
//  auto expr{std::make_unique<tensor_negative>(std::move(rhs))};
//  lhs.get()->add_child(std::move(expr));
//  return std::move(lhs);
//}
// ---------------------------------------

} // NAMESPACE symTM

#endif // TENSOR_H
