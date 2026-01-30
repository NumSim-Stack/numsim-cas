#ifndef TENSOR_DIFFERENTIATION_H
#define TENSOR_DIFFERENTIATION_H

#include "../../numsim_cas_type_traits.h"
#include "../../operators.h"
#include "../../tensor_to_scalar/tensor_to_scalar_std.h"
#include "../kronecker_delta.h"
#include "../tensor_expression.h"
#include "../tensor_functions_fwd.h"
#include "../tensor_std.h"
#include <algorithm>
#include <ranges>

namespace numsim::cas {

class tensor_differentiation {
public:
  using expr_type = expression_holder<tensor_expression>;

  tensor_differentiation(expr_type const &arg) : m_arg(arg) {
    I = make_expression<kronecker_delta>(m_arg.get().dim());
  }
  tensor_differentiation(tensor_differentiation const &) = delete;
  tensor_differentiation(tensor_differentiation &&) = delete;
  const tensor_differentiation &
  operator=(tensor_differentiation const &) = delete;

  auto apply(expr_type const &expr) {
    if (expr.is_valid()) {
      m_expr = expr;
      m_dim = expr.get().dim();
      m_rank_result = expr.get().rank() + m_arg.get().rank();
      m_rank_arg = m_arg.get().rank();
      // std::visit([this](auto &&arg) { (*this)(arg); }, *expr);
    }
    if (!m_result.is_valid()) {
      return make_expression<tensor_zero>(m_dim, m_rank_result);
    }
    return m_result;
  }

  void operator()([[maybe_unused]] tensor const &visitable) {
    if (&visitable == &m_arg.get()) {
      if (m_rank_result == 2) {
        m_result = make_expression<kronecker_delta>(m_dim);
        return;
      }

      // \frac{\partial A_{ijkl...}}{\partial A_{mnop...}} = I_{im} I_{jn}
      // I_{ko} I_{lp} ...
      m_result = make_expression<identity_tensor>(m_dim, m_rank_result);
    }
  }

  void operator()([[maybe_unused]] identity_tensor const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) {
    m_result = make_expression<tensor_zero>(m_dim, m_rank_result);
  }

  void operator()([[maybe_unused]] tensor_add const &visitable) {
    loop(visitable.hash_map() | std::views::values,
         [&](auto dexpr) { m_result += dexpr; });
  }

  void operator()([[maybe_unused]] tensor_negative const &visitable) {
    m_result = -diff(visitable.expr(), m_arg);
  }

  void operator()([[maybe_unused]] inner_product_wrapper const &visitable) {
    //    auto& expr_lhs{visitable.expr_lhs()};
    //    auto& expr_rhs{visitable.expr_rhs()};
    //    auto seq_lhs{visitable.sequence_lhs()};
    //    auto seq_rhs{visitable.sequence_rhs()};
    //    std::for_each(seq_lhs.begin(), seq_lhs.end(), [](auto &i) { i += 1;
    //    }); std::for_each(seq_rhs.begin(), seq_rhs.end(), [](auto &i) { i +=
    //    1; });
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
    //            inner_product(std::move(diff_lhs), seq_lhs, expr_rhs,
    //            seq_rhs)};
    //        auto inner_rhs{
    //            inner_product(expr_lhs, seq_lhs, std::move(diff_rhs),
    //            seq_rhs)};
    //        auto basis_lhs{basis_change(std::move(inner_lhs), indices)};
    //        lhs = std::move(basis_lhs);
    //        rhs = std::move(inner_rhs);
    //      } else {
    //        // setup lhs and rhs
    //        auto inner_lhs{
    //            inner_product(std::move(diff_lhs), seq_lhs, expr_rhs,
    //            seq_rhs)};
    //        auto inner_rhs{
    //            inner_product(expr_lhs, seq_lhs, std::move(diff_rhs),
    //            seq_rhs)};
    //        lhs = std::move(inner_lhs);
    //        rhs = std::move(inner_rhs);
    //      }

    //      auto result{make_expression<tensor_add>(m_dim,
    //      rank_new)}; result.template
    //      get<tensor_add>().add_child(std::move(lhs));
    //      result.template
    //      get<tensor_add>().add_child(std::move(rhs)); m_data =
    //      std::move(result); return;
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
    //            inner_product(std::move(diff_lhs), seq_lhs, expr_rhs,
    //            seq_rhs)};
    //        m_data = basis_change(std::move(inner_lhs), indices);
    //      } else {
    //        m_data =
    //            inner_product(std::move(diff_lhs), seq_lhs, expr_rhs,
    //            seq_rhs);
    //      }
    //      return;
    //    }

    //    if (diff_rhs) {
    //      m_data = inner_product(expr_lhs, seq_lhs, std::move(diff_rhs),
    //      seq_rhs); return;
    //    }
  }

  void operator()([[maybe_unused]] basis_change_imp const &visitable) {
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

  void operator()([[maybe_unused]] outer_product_wrapper const &visitable) {
    //    auto expr_lhs{visitable.expr_lhs()};
    //    auto expr_rhs{visitable.expr_rhs()};
    //    auto seq_lhs{visitable.sequence_lhs()};
    //    auto seq_rhs{visitable.sequence_rhs()};
    //    std::for_each(seq_lhs.begin(), seq_lhs.end(), [](auto &i) { i += 1;
    //    }); std::for_each(seq_rhs.begin(), seq_rhs.end(), [](auto &i) { i +=
    //    1; });

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
    //      std::iota(&indices_lhs[rank_lhs], &indices_lhs[new_rank], rank_lhs +
    //      1); for (std::size_t i{0}; i < seq_rhs.size(); ++i) {
    //        indices_rhs[i] = seq_rhs[i];
    //      }
    //      std::iota(&indices_rhs[rank_rhs], &indices_rhs[new_rank], rank_rhs +
    //      1); auto outer_lhs{
    //          outer_product(std::move(diff_lhs), indices_lhs, expr_rhs,
    //          seq_rhs)};
    //      auto outer_rhs{outer_product(expr_lhs, indices_lhs,
    //      std::move(diff_rhs),
    //                                   indices_rhs)};
    //      auto result{make_expression<tensor_add>(m_dim,
    //      new_rank)}; result.template
    //      get<tensor_add>().add_child(std::move(outer_lhs));
    //      result.template
    //      get<tensor_add>().add_child(std::move(outer_rhs)); m_data
    //      = std::move(result); return;
    //    }

    //    if (diff_lhs) {
    //      const auto rank_lhs{expr_lhs->rank()};
    //      const auto new_rank{m_rank + m_rank_arg};
    //      std::vector<std::size_t> indices_lhs(new_rank);
    //      for (std::size_t i{0}; i < seq_lhs.size(); ++i) {
    //        indices_lhs[i] = seq_lhs[i];
    //      }
    //      std::iota(&indices_lhs[rank_lhs], &indices_lhs[new_rank], rank_lhs +
    //      1); auto outer_lhs{
    //          outer_product(std::move(diff_lhs), indices_lhs, expr_rhs,
    //          seq_rhs)};
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
    //      std::iota(&indices_rhs[rank_rhs], &indices_rhs[new_rank], rank_rhs +
    //      1); auto outer_rhs{
    //          outer_product(expr_lhs, seq_lhs, std::move(diff_rhs),
    //          indices_rhs)};
    //      m_data = std::move(outer_rhs);
    //      return;
    //    }
  }

  void operator()([[maybe_unused]] kronecker_delta const &visitable) {
    m_result = make_expression<tensor_zero>(m_dim, m_rank_result);
  }

  // void operator()(simple_outer_product &visitable) {}

  // (scalar*tensor)' = scalar*tensor'
  void operator()(tensor_scalar_mul const &visitable) {
    m_result = visitable.expr_lhs() * diff(visitable.expr_rhs(), m_arg);
  }

  // (tensor/scalar)' = tensor'/scalar
  void operator()(tensor_scalar_div const &visitable) {
    m_result = diff(visitable.expr_lhs(), m_arg) / visitable.expr_rhs();
  }

  // (tensor_to_scalar*tensor)' = tensor_to_scalar*tensor' +
  // otimes(tensor,tensor_to_scalar')
  void operator()(tensor_to_scalar_with_tensor_mul const &visitable) {
    m_result = visitable.expr_lhs() * diff(visitable.expr_rhs(), m_arg) +
               otimes(visitable.expr_rhs(), diff(visitable.expr_lhs(), m_arg));
  }

  // (tensor/tensor_to_scalar)' = tensor'/tensor_to_scalar -
  // otimes(tensor,tensor_to_scalar') / pow(tensor_to_scalar,2)
  void operator()(tensor_to_scalar_with_tensor_div const &visitable) {
    m_result = diff(visitable.expr_lhs(), m_arg) / visitable.expr_rhs() +
               otimes(visitable.expr_lhs(), diff(visitable.expr_rhs(), m_arg)) /
                   std::pow(visitable.expr_rhs(), 2);
  }

  // dev(A[X])' = (A[X] - vol(A[X]))'
  //            = (A[X] - trace(A[X])/dim(A)*I)'
  //            =  dAdX - otimes(I,I):dAdX/dim(A)
  void operator()([[maybe_unused]] tensor_deviatoric const &visitable) {
    auto result{diff(visitable.expr(), m_arg)};
    m_result = result - inner_product(otimes(I, I), sequence{3, 4}, result,
                                      sequence{1, 2}) /
                            static_cast(m_dim);
  }

  // (vol(A[X]))' = (trace(A[X])*I/dim(A))' = otimes(I,I):dAdX/dim(A)
  void operator()(tensor_volumetric const &visitable) {
    auto result{diff(visitable.expr(), m_arg)};
    // 0.5 * I_ij * I_kl * (I_km * I_ln + I_kn * I_lm)
    // 0.5 * I_ij * (I_lm * I_ln + I_ln * I_lm)
    // 0.5 * I_ij * (I_mn + I_mn)
    // I_ij * I_mn
    m_result =
        inner_product(otimes(I, I), sequence{3, 4}, result, sequence{1, 2});
  }

  // rank() == 2
  // rank() == 4
  void operator()(tensor_inv const &) {}

  void operator()(tensor_symmetry const &visitable) {
    auto result{diff(visitable.expr(), m_arg)};
    if (result.is_valid()) {
      // ijkl    --> ikjl    + iljk
      // 1,2,3,4 --> 1,3,2,4 + 1,4,2,3
      m_result = 0.5 * (result + permute_indices(result, sequence{1, 4, 3, 2}));
    }
    //    auto
    //    divisor{make_expression<scalar_constant>(visitable.symmetries().size()+1)};
    //    auto add{make_expression<tensor_add>(visitable.dim(),
    //    visitable.rank())}; tensor_differentiation diff(m_arg);
    //    add.template
    //    get<tensor_add>().add_child(diff.apply(visitable.expr()));
    //    for(const auto& sym : visitable.symmetries()){
    //      tensor_differentiation diff(m_arg);
    //      auto
    //      basis{make_expression<basis_change_imp>(visitable.expr(),
    //      sym)}; add.template
    //      get<tensor_add>().add_child(diff.apply(basis));
    //    }
    //    auto
    //    result{make_expression<tensor_scalar_div>(std::move(divisor),
    //    std::move(add))}; m_data = std::move(result);
  }

  // pow(expr, scalar) scalar \in N
  // pow(expr, 0) = I
  // A^2 = A_im*A_mj
  // d(A_im*A_mj)/dA_op = dA_im/dA_op*A_mj + A_im*dA_mj/dA_op  (i,j,o,p)
  //                    = I_io * I_mp * A_mj + A_im * I_mo * I_jp
  //                    = I_io * I_pm * A_mj + A_im * I_mo * I_jp
  //                    = I_io * A_pj + A_io * I_jp
  void operator()([[maybe_unused]] tensor_pow const &visitable) {
    m_result = make_expression<tensor_power_diff>(visitable.expr_lhs(),
                                                  visitable.expr_rhs());
  }

  // A^n = A*A*A...
  // sum_r^n otimesu(pow(expr, r), pow(expr, (n-1)-r)), n := scalar expr \in N
  void operator()([[maybe_unused]] tensor_power_diff const &visitable) {
    assert(0);
  }

  template <typename T> void operator()([[maybe_unused]] T const &visitable) {
    assert(0);
  }

private:
  template <typename Type> std::vector<expr_type> loop(Type const &type) {
    std::vector<expr_type> diff_vec;
    diff_vec.reserve(type.size());

    for (auto &child : type) {
      tensor_differentiation diff(m_arg);
      auto temp{diff.apply(child)};
      if (temp) {
        diff_vec.emplace_back(std::move(temp));
      }
    }
    return diff_vec;
  }

  template <typename Type, typename Func>
  void loop(Type const &type, Func func) {
    for (auto &child : type) {
      tensor_differentiation diff(m_arg);
      auto temp{diff.apply(child)};
      func(temp);
    }
  }

  template <typename Type> void loop_result(Type const &type) {
    auto diff_vec{loop(type)};
    if (diff_vec.empty()) {
      return;
    }

    if (diff_vec.size() == 1) {
      m_result = std::move(diff_vec.back());
      return;
    }

    auto temp{make_expression<Type>(m_dim, m_rank_result)};
    for (auto &child : diff_vec) {
      temp.template get<Type>().add_child(std::move(child));
    }
    m_result = std::move(temp);
  }

  auto apply_imp(expr_type const &expr) {
    if (expr.is_valid()) {
      const auto &tensor_expr = expr.get();
      m_dim = tensor_expr.dim();
      m_rank_arg = m_arg.get().rank();
      m_rank_result = tensor_expr.rank() + m_rank_arg;
      m_expr = expr;
      std::visit([this](auto &&arg) { (*this)(arg); }, *expr);
      return m_result;
    } else {
      return make_expression<tensor_zero>(m_dim, m_rank_result);
    }
  }

  expr_type const &m_arg;
  std::size_t m_dim{0};
  std::size_t m_rank_result{0};
  std::size_t m_rank_arg{0};
  expr_type m_result{nullptr};
  expr_type m_expr;
  expr_type I;
};

} // namespace numsim::cas
#endif // TENSOR_DIFFERENTIATION_H
