#ifndef SCALAR_EVALUATOR_H
#define SCALAR_EVALUATOR_H

#include <cmath>
#include <complex>
#include <numsim_cas/core/evaluator_base.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <ranges>
#include <variant>

namespace numsim::cas {

template <typename ValueType>
class scalar_evaluator final : public scalar_visitor_const_t,
                               public evaluator_base<ValueType> {
  using base = evaluator_base<ValueType>;
  using base::m_result;

public:
  using expr_holder_t = expression_holder<scalar_expression>;

  scalar_evaluator() = default;
  scalar_evaluator(scalar_evaluator const &) = delete;
  scalar_evaluator(scalar_evaluator &&) = delete;
  scalar_evaluator &operator=(scalar_evaluator const &) = delete;

  ValueType apply(expr_holder_t const &expr) {
    if (expr.is_valid()) {
      base::m_current_expr = base::to_base_holder(expr);
      expr.template get<scalar_visitable_t>().accept(*this);
      return m_result;
    }
    return ValueType{0};
  }

  void operator()(scalar const &) override { base::dispatch(); }

  void operator()([[maybe_unused]] scalar_zero const &) override {
    m_result = ValueType{0};
  }

  void operator()([[maybe_unused]] scalar_one const &) override {
    m_result = ValueType{1};
  }

  void operator()(scalar_constant const &visitable) override {
    m_result = std::visit(
        [](auto const &v) -> ValueType {
          using V = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<V, std::complex<double>>) {
            return static_cast<ValueType>(v.real());
          } else {
            return static_cast<ValueType>(v);
          }
        },
        visitable.value().raw());
  }

  void operator()(scalar_add const &visitable) override {
    ValueType result{0};
    if (visitable.coeff().is_valid()) {
      result += apply(visitable.coeff());
    }
    for (auto const &child : visitable.hash_map() | std::views::values) {
      result += apply(child);
    }
    m_result = result;
  }

  void operator()(scalar_mul const &visitable) override {
    ValueType result{1};
    if (visitable.coeff().is_valid()) {
      result = apply(visitable.coeff());
    }
    for (auto const &child : visitable.hash_map() | std::views::values) {
      result *= apply(child);
    }
    m_result = result;
  }

  void operator()(scalar_negative const &visitable) override {
    m_result = -apply(visitable.expr());
  }

  void operator()(scalar_pow const &visitable) override {
    m_result =
        std::pow(apply(visitable.expr_lhs()), apply(visitable.expr_rhs()));
  }

  void operator()(scalar_rational const &visitable) override {
    m_result = apply(visitable.expr_lhs()) / apply(visitable.expr_rhs());
  }

  void operator()(scalar_sin const &visitable) override {
    m_result = std::sin(apply(visitable.expr()));
  }

  void operator()(scalar_cos const &visitable) override {
    m_result = std::cos(apply(visitable.expr()));
  }

  void operator()(scalar_tan const &visitable) override {
    m_result = std::tan(apply(visitable.expr()));
  }

  void operator()(scalar_asin const &visitable) override {
    m_result = std::asin(apply(visitable.expr()));
  }

  void operator()(scalar_acos const &visitable) override {
    m_result = std::acos(apply(visitable.expr()));
  }

  void operator()(scalar_atan const &visitable) override {
    m_result = std::atan(apply(visitable.expr()));
  }

  void operator()(scalar_sqrt const &visitable) override {
    m_result = std::sqrt(apply(visitable.expr()));
  }

  void operator()(scalar_log const &visitable) override {
    m_result = std::log(apply(visitable.expr()));
  }

  void operator()(scalar_exp const &visitable) override {
    m_result = std::exp(apply(visitable.expr()));
  }

  void operator()(scalar_sign const &visitable) override {
    const ValueType u{apply(visitable.expr())};
    if (u > ValueType{0})
      m_result = ValueType{1};
    else if (u < ValueType{0})
      m_result = ValueType{-1};
    else
      m_result = ValueType{0};
  }

  void operator()(scalar_abs const &visitable) override {
    m_result = std::abs(apply(visitable.expr()));
  }

  void operator()(scalar_named_expression const &visitable) override {
    m_result = apply(visitable.expr());
  }

  template <class T> void operator()([[maybe_unused]] T const &) noexcept {
    static_assert(sizeof(T) == 0,
                  "scalar_evaluator: missing overload for this node type");
  }
};

} // namespace numsim::cas

#endif // SCALAR_EVALUATOR_H
