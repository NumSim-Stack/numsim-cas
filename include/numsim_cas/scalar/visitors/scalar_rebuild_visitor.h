#ifndef SCALAR_REBUILD_VISITOR_H
#define SCALAR_REBUILD_VISITOR_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <ranges>

namespace numsim::cas {

class scalar_rebuild_visitor : public scalar_visitor_const_t {
public:
  using expr_holder_t = expression_holder<scalar_expression>;

  virtual ~scalar_rebuild_visitor() = default;

  virtual expr_holder_t apply(expr_holder_t const &expr) noexcept {
    if (expr.is_valid()) {
      m_current = expr;
      expr.get<scalar_visitable_t>().accept(*this);
      return std::move(m_result);
    }
    return expr;
  }

  void operator()(scalar const &) noexcept override { m_result = m_current; }

  void operator()(scalar_zero const &) noexcept override {
    m_result = m_current;
  }

  void operator()(scalar_one const &) noexcept override {
    m_result = m_current;
  }

  void operator()(scalar_constant const &) noexcept override {
    m_result = m_current;
  }

  void operator()(scalar_rational const &) noexcept override {
    m_result = m_current;
  }

  void operator()(scalar_add const &v) noexcept override {
    expr_holder_t result;
    if (v.coeff().is_valid())
      result += apply(v.coeff());
    for (auto &child : v.hash_map() | std::views::values)
      result += apply(child);
    m_result = std::move(result);
  }

  void operator()(scalar_mul const &v) noexcept override {
    expr_holder_t result;
    if (v.coeff().is_valid())
      result *= apply(v.coeff());
    for (auto &child : v.hash_map() | std::views::values)
      result *= apply(child);
    m_result = std::move(result);
  }

  void operator()(scalar_negative const &v) noexcept override {
    m_result = -apply(v.expr());
  }

  void operator()(scalar_function const &v) noexcept override {
    m_result =
        make_expression<scalar_function>(std::string(v.name()), apply(v.expr()));
  }

  void operator()(scalar_sin const &v) noexcept override {
    m_result = sin(apply(v.expr()));
  }

  void operator()(scalar_cos const &v) noexcept override {
    m_result = cos(apply(v.expr()));
  }

  void operator()(scalar_tan const &v) noexcept override {
    m_result = tan(apply(v.expr()));
  }

  void operator()(scalar_asin const &v) noexcept override {
    m_result = asin(apply(v.expr()));
  }

  void operator()(scalar_acos const &v) noexcept override {
    m_result = acos(apply(v.expr()));
  }

  void operator()(scalar_atan const &v) noexcept override {
    m_result = atan(apply(v.expr()));
  }

  void operator()(scalar_pow const &v) noexcept override {
    m_result = pow(apply(v.expr_lhs()), apply(v.expr_rhs()));
  }

  void operator()(scalar_sqrt const &v) noexcept override {
    m_result = sqrt(apply(v.expr()));
  }

  void operator()(scalar_log const &v) noexcept override {
    m_result = log(apply(v.expr()));
  }

  void operator()(scalar_exp const &v) noexcept override {
    m_result = exp(apply(v.expr()));
  }

  void operator()(scalar_sign const &v) noexcept override {
    m_result = sign(apply(v.expr()));
  }

  void operator()(scalar_abs const &v) noexcept override {
    m_result = abs(apply(v.expr()));
  }

protected:
  expr_holder_t m_current;
  expr_holder_t m_result;
};

} // namespace numsim::cas

#endif // SCALAR_REBUILD_VISITOR_H
