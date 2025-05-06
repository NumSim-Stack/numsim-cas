#ifndef TENSOR_DIFFERENTIATION_H
#define TENSOR_DIFFERENTIATION_H

#include <algorithm>

#include "../symTM_type_traits.h"
#include "tensor_expression.h"
#include "tensor_functions.h"
#include "kronecker_delta.h"


namespace numsim::cas {

template<typename ValueType>
class tensor_differentiation
{
public:
  using expr_type = expression_holder<tensor_expression<ValueType>>;

  tensor_differentiation(expr_type const&arg)
      : m_dim(0), m_rank(0), m_data(), m_arg(arg) {
    auto &tensor_expr = m_arg.get();
    m_rank_arg = tensor_expr.rank();
  }

  tensor_differentiation(tensor_differentiation const &) = delete;
  tensor_differentiation(tensor_differentiation &&) = delete;
  const tensor_differentiation &
  operator=(tensor_differentiation const &) = delete;

  expr_type apply(expr_type &expr) {
//    auto &expr_ = expr.template get<VisitableTensor_t<ValueType>>();
//    const auto &tensor_expr = expr.get();
//    m_dim = tensor_expr.dim();
//    m_rank = tensor_expr.rank();
//    m_rank_result = tensor_expr.rank() + m_rank_arg;
//    expr_.accept(*this);
    return m_data;
  }

  expr_type apply(tensor_expression<ValueType> &expr) {
//    auto &expr_ = static_cast<VisitableTensor_t<ValueType> &>(expr);
//    m_dim = expr.dim();
//    m_rank = expr.rank() + m_rank_arg;
//    expr_.accept(*this);
    return std::move(m_data);
  }

  virtual void operator()(tensor<ValueType> &visitable) {
//    if (visitable) {
//      tensor_differentiation diff(m_arg);
//      m_data = diff.apply(visitable);
//    } else {
//      if (&visitable == &m_arg.get()) {
//        if (m_rank_result == 2) {
//          m_data = make_expression<kronecker_delta<ValueType>>(m_dim);
//        } else {
//          // \frac{\partial A_{ijkl...}}{\partial A_{mnop...}} = I_{im} I_{jn} I_{ko} I_{lp}
//          // 1,rank(A)+1,2,rank(A)+2,3,rank(A)+3,4,rank(A)+4,...
//          auto temp{make_expression<simple_outer_product<ValueType>>(m_dim, m_rank_result)};
//          for (std::size_t i{0}; i < m_rank_result; i += 2) {
//            temp.template get<simple_outer_product<ValueType>>().add_child(make_expression<kronecker_delta<ValueType>>(m_dim));
//          }
//          std::size_t iter{1};
//          std::vector<std::size_t> indices(m_rank_result);
//          for(auto& i : nth_range(indices.begin(),indices.end(),2)){
//            i = iter++;
//          }
//          for(auto& i : nth_range(++indices.begin(),indices.end(),2)){
//            i = iter++;
//          }
//          m_data = basis_change(std::move(temp), std::move(indices));
//        }
//      }
//    }
  }

  virtual void operator()(tensor_add<ValueType> &visitable) { /*loop_result(visitable);*/ }

  virtual void operator()(tensor_negative<ValueType> &visitable) {
//    tensor_differentiation diff(m_arg);
//    auto diff_lhs{diff.apply(visitable.expr())};
//    m_data = std::move(make_expression<tensor_negative<ValueType>>(std::move(diff_lhs)));
  }

  virtual void operator()(inner_product_wrapper<ValueType> &visitable) {
//    auto& expr_lhs{visitable.expr_lhs()};
//    auto& expr_rhs{visitable.expr_rhs()};
//    auto seq_lhs{visitable.sequence_lhs()};
//    auto seq_rhs{visitable.sequence_rhs()};
//    std::for_each(seq_lhs.begin(), seq_lhs.end(), [](auto &i) { i += 1; });
//    std::for_each(seq_rhs.begin(), seq_rhs.end(), [](auto &i) { i += 1; });
//    // get derivative lhs
//    tensor_differentiation diff(m_arg);
//    auto diff_lhs{diff.apply(expr_lhs)};
//    auto diff_rhs{diff.apply(expr_rhs)};

//    const auto size_inner_lhs{seq_lhs.size()};
//    const auto size_inner_rhs{seq_rhs.size()};
//    const auto rank_lhs{expr_lhs->rank()};
//    const auto rank_rhs{expr_rhs->rank()};
//    const auto rank_new{m_rank + m_rank_arg};
//    const auto free_lhs{rank_lhs - size_inner_lhs};
//    const auto free_rhs{rank_rhs - size_inner_rhs};

//    if (diff_lhs && diff_rhs) {
//      expr_type lhs, rhs;
//      if (free_rhs) {
//        // setup new basis
//        std::vector<std::size_t> indices(rank_new);
//        // left side
//        std::iota(&indices[0], &indices[free_lhs], 1);
//        // result
//        std::iota(&indices[free_lhs], &indices[free_lhs + m_rank_arg],
//                  free_lhs + free_rhs + 1);
//        // right side
//        std::iota(&indices[free_lhs + m_rank_arg], &indices[rank_new],
//                  free_lhs + 1);
//        // setup lhs and rhs
//        auto inner_lhs{
//            inner_product(std::move(diff_lhs), seq_lhs, expr_rhs, seq_rhs)};
//        auto inner_rhs{
//            inner_product(expr_lhs, seq_lhs, std::move(diff_rhs), seq_rhs)};
//        auto basis_lhs{basis_change(std::move(inner_lhs), indices)};
//        lhs = std::move(basis_lhs);
//        rhs = std::move(inner_rhs);
//      } else {
//        // setup lhs and rhs
//        auto inner_lhs{
//            inner_product(std::move(diff_lhs), seq_lhs, expr_rhs, seq_rhs)};
//        auto inner_rhs{
//            inner_product(expr_lhs, seq_lhs, std::move(diff_rhs), seq_rhs)};
//        lhs = std::move(inner_lhs);
//        rhs = std::move(inner_rhs);
//      }

//      auto result{make_expression<tensor_add<ValueType>>(m_dim, rank_new)};
//      result.template get<tensor_add<ValueType>>().add_child(std::move(lhs));
//      result.template get<tensor_add<ValueType>>().add_child(std::move(rhs));
//      m_data = std::move(result);
//      return;
//    }

//    if (diff_lhs) {
//      if (free_rhs) {
//        // setup new basis
//        std::vector<std::size_t> indices(rank_new);
//        // left side
//        std::iota(&indices[0], &indices[free_lhs], 1);
//        // result
//        std::iota(&indices[free_lhs], &indices[free_lhs + m_rank_arg],
//                  free_lhs + free_rhs + 1);
//        // right side
//        std::iota(&indices[free_lhs + m_rank_arg], &indices[rank_new],
//                  free_lhs + 1);
//        auto inner_lhs{
//            inner_product(std::move(diff_lhs), seq_lhs, expr_rhs, seq_rhs)};
//        m_data = basis_change(std::move(inner_lhs), indices);
//      } else {
//        m_data =
//            inner_product(std::move(diff_lhs), seq_lhs, expr_rhs, seq_rhs);
//      }
//      return;
//    }

//    if (diff_rhs) {
//      m_data = inner_product(expr_lhs, seq_lhs, std::move(diff_rhs), seq_rhs);
//      return;
//    }
  }

  virtual void operator()(basis_change_imp<ValueType> &visitable) {
//    auto expr{visitable.expr()};
//    const auto &seq{visitable.indices()};
//    // get derivative
//    tensor_differentiation diff(m_arg);
//    auto temp{diff.apply(expr)};
//    if (temp) {
//      // new indices
//      std::vector<std::size_t> indices(m_rank_result);
//      for (std::size_t i{0}; i < seq.size(); ++i) {
//        indices[i] = seq[i] + 1;
//      }
//      std::iota(&indices[m_rank], &indices[m_rank_result], m_rank + 1);
//      m_data = basis_change(std::move(temp), indices);
//    }
  }

  virtual void operator()(outer_product_wrapper<ValueType> &visitable) {
//    auto expr_lhs{visitable.expr_lhs()};
//    auto expr_rhs{visitable.expr_rhs()};
//    auto seq_lhs{visitable.sequence_lhs()};
//    auto seq_rhs{visitable.sequence_rhs()};
//    std::for_each(seq_lhs.begin(), seq_lhs.end(), [](auto &i) { i += 1; });
//    std::for_each(seq_rhs.begin(), seq_rhs.end(), [](auto &i) { i += 1; });

//    // get derivative lhs
//    tensor_differentiation diff(m_arg);
//    expr_type diff_lhs{diff.apply(expr_lhs)};
//    expr_type diff_rhs{diff.apply(expr_rhs)};
//    if (diff_lhs && diff_rhs) {
//      const auto rank_lhs{expr_lhs.get().rank()};
//      const auto rank_rhs{expr_rhs.get().rank()};
//      const auto new_rank{m_rank + m_rank_arg};
//      std::vector<std::size_t> indices_lhs(new_rank);
//      std::vector<std::size_t> indices_rhs(new_rank);
//      for (std::size_t i{0}; i < seq_lhs.size(); ++i) {
//        indices_lhs[i] = seq_lhs[i];
//      }
//      std::iota(&indices_lhs[rank_lhs], &indices_lhs[new_rank], rank_lhs + 1);
//      for (std::size_t i{0}; i < seq_rhs.size(); ++i) {
//        indices_rhs[i] = seq_rhs[i];
//      }
//      std::iota(&indices_rhs[rank_rhs], &indices_rhs[new_rank], rank_rhs + 1);
//      auto outer_lhs{
//          outer_product(std::move(diff_lhs), indices_lhs, expr_rhs, seq_rhs)};
//      auto outer_rhs{outer_product(expr_lhs, indices_lhs, std::move(diff_rhs),
//                                   indices_rhs)};
//      auto result{make_expression<tensor_add<ValueType>>(m_dim, new_rank)};
//      result.template get<tensor_add<ValueType>>().add_child(std::move(outer_lhs));
//      result.template get<tensor_add<ValueType>>().add_child(std::move(outer_rhs));
//      m_data = std::move(result);
//      return;
//    }

//    if (diff_lhs) {
//      const auto rank_lhs{expr_lhs->rank()};
//      const auto new_rank{m_rank + m_rank_arg};
//      std::vector<std::size_t> indices_lhs(new_rank);
//      for (std::size_t i{0}; i < seq_lhs.size(); ++i) {
//        indices_lhs[i] = seq_lhs[i];
//      }
//      std::iota(&indices_lhs[rank_lhs], &indices_lhs[new_rank], rank_lhs + 1);
//      auto outer_lhs{
//          outer_product(std::move(diff_lhs), indices_lhs, expr_rhs, seq_rhs)};
//      m_data = std::move(outer_lhs);
//      return;
//    }

//    if (diff_rhs) {
//      const auto rank_rhs{expr_rhs->rank()};
//      const auto new_rank{m_rank + m_rank_arg};
//      std::vector<std::size_t> indices_rhs(new_rank);
//      for (std::size_t i{0}; i < seq_rhs.size(); ++i) {
//        indices_rhs[i] = seq_rhs[i];
//      }
//      std::iota(&indices_rhs[rank_rhs], &indices_rhs[new_rank], rank_rhs + 1);
//      auto outer_rhs{
//          outer_product(expr_lhs, seq_lhs, std::move(diff_rhs), indices_rhs)};
//      m_data = std::move(outer_rhs);
//      return;
//    }
  }

  virtual void operator()(kronecker_delta<ValueType> &visitable) {
    // do nothing
  }

  //virtual void operator()(simple_outer_product<ValueType> &visitable) {}

  virtual void operator()(tensor_scalar_mul<ValueType> &) {}

  virtual void operator()(tensor_scalar_div<ValueType> &) {}

  virtual void operator()(tensor_symmetry<ValueType> &visitable) {
//    auto divisor{make_expression<scalar_constant<ValueType>>(visitable.symmetries().size()+1)};
//    auto add{make_expression<tensor_add<ValueType>>(visitable.dim(), visitable.rank())};
//    tensor_differentiation<ValueType> diff(m_arg);
//    add.template get<tensor_add<ValueType>>().add_child(diff.apply(visitable.expr()));
//    for(const auto& sym : visitable.symmetries()){
//      tensor_differentiation<ValueType> diff(m_arg);
//      auto basis{make_expression<basis_change_imp<ValueType>>(visitable.expr(), sym)};
//      add.template get<tensor_add<ValueType>>().add_child(diff.apply(basis));
//    }
//    auto result{make_expression<tensor_scalar_div<ValueType>>(std::move(divisor), std::move(add))};
//    m_data = std::move(result);
  }

private:
  template <typename Type>
  std::vector<expr_type> loop(Type &type) {
    std::vector<expr_type> diff_vec;
    diff_vec.reserve(type.size());

    for (auto& child : type) {
      tensor_differentiation diff(m_arg);
      auto temp{diff.apply(child)};
      if (temp) {
        diff_vec.emplace_back(std::move(temp));
      }
    }
    return diff_vec;
  }

  template <typename Type> void loop_result(Type &type) {
    auto diff_vec{loop(type)};
    if (diff_vec.empty()) {
      return;
    }

    if (diff_vec.size() == 1) {
      m_data = std::move(diff_vec.back());
      return;
    }

    auto temp{make_expression<Type>(m_dim, m_rank + m_rank_arg)};
    for (auto &child : diff_vec) {
      temp.template get<Type>().add_child(std::move(child));
    }
    m_data = std::move(temp);
  }

  std::size_t m_dim;
  std::size_t m_rank;
  std::size_t m_rank_result;
  std::size_t m_rank_arg;
  expr_type m_data;
  expr_type const&m_arg;
};

}
#endif // TENSOR_DIFFERENTIATION_H
