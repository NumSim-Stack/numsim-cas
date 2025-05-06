#ifndef TENSOR_EVALUATOR_H
#define TENSOR_EVALUATOR_H

#include "../symTM_type_traits.h"
#include "tensor_data.h"
#include "tensor_data_add.h"
#include "tensor_data_sub.h"
#include "tensor_data_basis_change.h"
#include "tensor_data_inner_product.h"
#include "tensor_data_outer_product.h"
#include "../functions.h"

namespace numsim::cas {

template<typename ValueType>
class tensor_evaluator{
public:
  tensor_evaluator() : dim(0), rank(0), m_data() {}
  tensor_evaluator(tensor_evaluator const &) = delete;
  tensor_evaluator(tensor_evaluator &&) = delete;
  const tensor_evaluator &operator=(tensor_evaluator const &) = delete;

  std::unique_ptr<tensor_data_base<ValueType>> apply(expression &expr) {
//    auto &expr_ = static_cast<VisitableTensor_t<ValueType> &>(expr);
//    auto &tensor_expr = static_cast<tensor_expression<ValueType> &>(expr);
//    dim = tensor_expr.dim();
//    rank = tensor_expr.rank();
//    m_data = make_tensor_data<ValueType>(dim, rank);
//    expr_.accept(*this);
    return std::move(m_data);
  }

  std::unique_ptr<tensor_data_base<ValueType>> apply(expression_holder<tensor_expression<ValueType>> expr) {
//    auto &expr_ = static_cast<VisitableTensor_t<ValueType> &>(expr.get());
//    auto &tensor_expr = expr.get();
//    dim = tensor_expr.dim();
//    rank = tensor_expr.rank();
//    m_data = make_tensor_data<ValueType>(dim, rank);
//    expr_.accept(*this);
    return std::move(m_data);
  }

  void operator()(tensor<ValueType> &visitable) {
    if (visitable) {
      tensor_evaluator ev;
      auto& expr = visitable.expr();
      auto temp = ev.apply(expr);
      tensor_data_add<ValueType> add(*m_data.get(), *temp.get());
      add.evaluate(dim, rank);
    } else {
      tensor_data_add<ValueType> add(*m_data.get(), visitable.data());
      add.evaluate(dim, rank);
    }
  }

  void operator()(tensor_add<ValueType> &visitable) {
    for (auto& child : visitable) {
      tensor_evaluator ev;
      auto temp = ev.apply(*child);
      tensor_data_add<ValueType> add(*m_data.get(), *temp.get());
      add.evaluate(dim, rank);
    }
  }

  void operator()(tensor_negative<ValueType> &visitable) {
    tensor_evaluator ev;
    auto temp = ev.apply(visitable.expr());
    tensor_data_sub<ValueType> sub(*m_data.get(), *temp.get());
    sub.evaluate(dim, rank);
  }

  void operator()(inner_product_wrapper<ValueType> &visitable) {
    tensor_evaluator ev;
    auto result_lhs{ev.apply(visitable.expr_lhs())};
    auto result_rhs{ev.apply(visitable.expr_rhs())};
    tensor_data_inner_product<ValueType> ip(*m_data.get(), *result_lhs.get(),
                                 *result_rhs.get(), visitable.sequence_lhs(),
                                 visitable.sequence_rhs());
    //reverse the input parameter due to template call
    //TODO reverse arguments in tensor_data
    ip.evaluate(dim, result_rhs->rank(), result_lhs->rank());
  }

  void operator()(basis_change_imp<ValueType> &visitable) {
    tensor_evaluator ev;
    auto temp = ev.apply(visitable.expr());
    tensor_data_basis_change<ValueType> imp(*m_data.get(), *temp.get(),
                                 visitable.indices());
    imp.evaluate(dim, rank);
  }

  void operator()(outer_product_wrapper<ValueType> &visitable) {
    tensor_evaluator ev;
    auto result_lhs{ev.apply((visitable.expr_lhs()))};
    auto result_rhs{ev.apply((visitable.expr_rhs()))};
    tensor_data_outer_product<ValueType> op(*m_data.get(), *result_lhs.get(),
                                 *result_rhs.get(), visitable.sequence_lhs(),
                                 visitable.sequence_rhs());
    //reverse the input parameter due to template call
    //TODO reverse arguments in tensor_data
    op.evaluate(dim, result_rhs->rank(), result_lhs->rank());
  }

  void operator()(kronecker_delta<ValueType> &visitable){

  }

//  void operator()(simple_outer_product<ValueType> &visitable){

//  }

  void operator()(tensor_scalar_mul<ValueType>& visitable){
    auto& rhs = visitable.expr_rhs();
    auto& lhs = visitable.expr_lhs();

    //  scalar_evaluator ev_scalar;
    //  const auto scalar = ev_scalar.apply(*lhs);
    //  evaluator_tensor ev_tensor;
    //  const auto tensor = ev_tensor.apply(*rhs);
    //  scalar_tensor_mul_op op(*m_data, scalar, *tensor);
    //  op.evaluate(dim, rank);
  }

  void operator()(tensor_scalar_div<ValueType>& visitable){
    auto& rhs = visitable.expr_rhs();
    auto& lhs = visitable.expr_lhs();

    //  scalar_evaluator ev_scalar;
    //  const auto scalar = ev_scalar.apply(*lhs);
    //  evaluator_tensor ev_tensor;
    //  const auto tensor = ev_tensor.apply(*rhs);
    //  scalar_tensor_mul_op op(*m_data, scalar, *tensor);
    //  op.evaluate(dim, rank);
  }

  virtual void operator()(tensor_symmetry<ValueType> &) {}


protected:
  std::size_t dim;
  std::size_t rank;
  std::unique_ptr<tensor_data_base<ValueType>> m_data;
};

} // NAMESPACE symTM

#endif // TENSOR_EVALUATOR_H
