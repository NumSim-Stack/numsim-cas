#ifndef SCALAR_SIMPLIFIER_SUB_H
#define SCALAR_SIMPLIFIER_SUB_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_globals.h>

namespace numsim::cas {
namespace simplifier {

template <typename Derived>
class sub_default : public scalar_visitor_return_expr_t {
public:
  using expr_holder_t = expression_holder<scalar_expression>;

  sub_default(expr_holder_t lhs, expr_holder_t rhs)
      : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override {                              \
    if constexpr (std::is_void_v<Derived>) {                                   \
      return dispatch(n);                                                      \
    } else {                                                                   \
      return static_cast<Derived *>(this)->dispatch(n);                        \
    }                                                                          \
  }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER

  // rhs is negative
  expr_holder_t get_default() {
    // std::cout<<m_lhs<<" "<<m_rhs<<std::endl;
    // std::cout<<"lhs id "<<m_lhs.get().id()<<" "<<"lhs id
    // "<<m_rhs.get().id()<<std::endl; std::cout<<m_lhs.get().hash_value()<<"
    // "<<m_rhs.get().hash_value()<<std::endl;

    if (m_lhs.get().hash_value() == m_rhs.get().hash_value()) {
      return get_scalar_zero();
    }

    const auto lhs_constant{is_same<scalar_constant>(m_lhs)};
    const auto rhs_constant{is_same<scalar_constant>(m_rhs)};
    auto add_new{make_expression<scalar_add>()};
    auto &add{add_new.template get<scalar_add>()};
    if (lhs_constant) {
      add.set_coeff(m_lhs);
    } else {
      add_new.template get<scalar_add>().push_back(m_lhs);
    }

    if (rhs_constant) {
      add.set_coeff(make_expression<scalar_constant>(
          -m_rhs.template get<scalar_constant>().value()));
    } else {
      add_new.template get<scalar_add>().push_back(-m_rhs);
    }
    return add_new;
  }

  template <typename Expr> expr_holder_t dispatch(Expr const &) {
    return get_default();
  }

  // expr - 0 --> expr
  expr_holder_t dispatch(scalar_zero const &) { return m_lhs; }

  template <typename _Expr, typename _ValueType>
  scalar_number get_coefficient(_Expr const &expr, _ValueType const &value) {
    if (is_detected_v<has_coefficient, _Expr>) {
      auto func{[&](auto const &coeff) -> scalar_number {
        if (coeff.is_valid()) {
          if (is_same<scalar_negative>(coeff)) {
            const auto &neg_expr{coeff.template get<scalar_negative>().expr()};
            if (is_same<scalar_one>(neg_expr))
              return {-1};
            if (is_same<scalar_constant>(neg_expr))
              return -neg_expr.template get<scalar_constant>().value();
          } else {
            if (is_same<scalar_one>(coeff))
              return {1};
            if (is_same<scalar_constant>(coeff))
              return coeff.template get<scalar_constant>().value();
          }
        }

        return value;
      }};
      return func(expr.coeff());
    }
    return value;
  }

protected:
  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

class negative_sub final : public sub_default<negative_sub> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = sub_default<negative_sub>;
  using base::dispatch;
  using base::get_coefficient;

  negative_sub(expr_holder_t lhs, expr_holder_t rhs);

  //-expr - (constant + x)
  //-expr - constant - x
  //-(expr + constant + x)
  expr_holder_t dispatch([[maybe_unused]] scalar_add const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_negative const &lhs;
};

class constant_sub final : public sub_default<constant_sub> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = sub_default<constant_sub>;
  using base::dispatch;
  using base::get_coefficient;

  constant_sub(expr_holder_t lhs, expr_holder_t rhs);

  // lhs - rhs
  expr_holder_t dispatch(scalar_constant const &rhs);

  // constant_lhs - (constant + x)
  // constant_lhs - constant - x
  expr_holder_t dispatch([[maybe_unused]] scalar_add const &rhs);

  expr_holder_t dispatch(scalar_one const &);

private:
  scalar_constant const &lhs;
};

class n_ary_sub final : public sub_default<n_ary_sub> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = sub_default<n_ary_sub>;
  using base::dispatch;
  using base::get_coefficient;

  n_ary_sub(expr_holder_t lhs, expr_holder_t rhs);

  expr_holder_t dispatch([[maybe_unused]] scalar_constant const &rhs);

  expr_holder_t dispatch([[maybe_unused]] scalar_one const &);

  // x+y+z - x --> y+z
  expr_holder_t dispatch([[maybe_unused]] scalar const &rhs);

  // merge two expression
  expr_holder_t dispatch(scalar_add const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_add const &lhs;
};

class n_ary_mul_sub final : public sub_default<n_ary_mul_sub> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = sub_default<n_ary_mul_sub>;
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  n_ary_mul_sub(expr_holder_t lhs, expr_holder_t rhs);

  // 2*x*y*z - x --> 2*x*y*z
  // 2*x - x --> x
  expr_holder_t dispatch([[maybe_unused]] scalar const &rhs);

  /// expr + expr --> 2*expr
  expr_holder_t dispatch(scalar_mul const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_mul const &lhs;
};

class symbol_sub final : public sub_default<symbol_sub> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = sub_default<symbol_sub>;
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  symbol_sub(expr_holder_t lhs, expr_holder_t rhs);

  /// x-x --> 0
  expr_holder_t dispatch(scalar const &rhs);

  // x - 3*x --> -(2*x)
  expr_holder_t dispatch(scalar_mul const &rhs);

  //  expr_holder_t operator()(scalar_constant
  //  const&rhs) {
  //  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar const &lhs;
};

class scalar_one_sub final : public sub_default<scalar_one_sub> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = sub_default<scalar_one_sub>;
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  scalar_one_sub(expr_holder_t lhs, expr_holder_t rhs);

  expr_holder_t dispatch(scalar_constant const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_one const &lhs;
};

struct sub_base final : public scalar_visitor_return_expr_t {
  using expr_holder_t = expression_holder<scalar_expression>;

  sub_base(expr_holder_t lhs, expr_holder_t rhs);

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  expr_holder_t dispatch(scalar_constant const &);

  expr_holder_t dispatch(scalar_add const &);

  expr_holder_t dispatch(scalar_mul const &);

  expr_holder_t dispatch(scalar const &);

  expr_holder_t dispatch(scalar_one const &);

  template <typename Type>
  expr_holder_t dispatch([[maybe_unused]] Type const &rhs);

  // 0 - expr
  expr_holder_t dispatch(scalar_zero const &);

  // - expr_lhs - expr_rhs --> -(expr_lhs+expr_rhs)
  expr_holder_t dispatch(scalar_negative const &lhs);

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

} // namespace simplifier
} // namespace numsim::cas

#endif // SCALAR_SIMPLIFIER_SUB_H
