#ifndef TENSOR_SIMPLIFIER_SUB_H
#define TENSOR_SIMPLIFIER_SUB_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/tensor_std.h>

namespace numsim::cas {
namespace tensor_detail {
namespace simplifier {

template <typename Derived>
class sub_default : public tensor_visitor_return_expr_t {
public:
  using expr_holder_t = expression_holder<tensor_expression>;

  sub_default(expr_holder_t lhs, expr_holder_t rhs)
      : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

protected:
#define NUMSIM_ADD_OVR(T)                                                      \
  expr_holder_t operator()(T const &n) override {                              \
    if constexpr (std::is_void_v<Derived>) {                                   \
      return dispatch(n);                                                      \
    } else {                                                                   \
      return static_cast<Derived *>(this)->dispatch(n);                        \
    }                                                                          \
  }
  NUMSIM_CAS_TENSOR_NODE_LIST(NUMSIM_ADD_OVR, NUMSIM_ADD_OVR)
#undef NUMSIM_ADD_OVR

  // rhs is negative
  auto get_default() {
    auto add_new{
        make_expression<tensor_add>(m_lhs.get().dim(), m_lhs.get().rank())};
    auto &add{add_new.template get<tensor_add>()};
    add.push_back(m_lhs);
    add.push_back(-m_rhs);
    return std::move(add_new);
  }

  template <typename Expr> expr_holder_t dispatch(Expr const &) {
    return get_default();
  }

protected:
  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

class negative_sub final : public sub_default<negative_sub> {
public:
  using expr_holder_t = expression_holder<tensor_expression>;
  using base = sub_default<negative_sub>;
  using base::dispatch;

  negative_sub(expr_holder_t lhs, expr_holder_t rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_negative const &lhs;
};

class n_ary_sub final : public sub_default<n_ary_sub> {
public:
  using expr_holder_t = expression_holder<tensor_expression>;
  using base = sub_default<n_ary_sub>;
  using base::dispatch;

  n_ary_sub(expr_holder_t lhs, expr_holder_t rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_add const &lhs;
};

class symbol_sub final : public sub_default<symbol_sub> {
public:
  using expr_holder_t = expression_holder<tensor_expression>;
  using base = sub_default<symbol_sub>;
  using base::dispatch;
  using base::get_default;

  symbol_sub(expr_holder_t lhs, expr_holder_t rhs);

  /// x-x --> 0
  expr_holder_t dispatch(tensor const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor const &lhs;
};

class sub_base final : public tensor_visitor_return_expr_t {
public:
  using expr_holder_t = expression_holder<tensor_expression>;

  sub_base(expr_holder_t lhs, expr_holder_t rhs);

protected:
#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_TENSOR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  expr_holder_t dispatch(tensor_add const &);

  expr_holder_t dispatch(tensor const &);

  // 0 - expr
  expr_holder_t dispatch(tensor_zero const &);

  // - expr_lhs - expr_rhs --> -(expr_lhs+expr_rhs)
  expr_holder_t dispatch(tensor_negative const &lhs);

  template <typename Type> expr_holder_t dispatch(Type const &) {
    auto &_rhs{m_rhs.template get<tensor_visitable_t>()};
    sub_default<void> visitor(std::move(m_lhs), std::move(m_rhs));
    return _rhs.accept(visitor);
  }

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

} // namespace simplifier
} // namespace tensor_detail
} // namespace numsim::cas

#endif // TENSOR_SIMPLIFIER_SUB_H
