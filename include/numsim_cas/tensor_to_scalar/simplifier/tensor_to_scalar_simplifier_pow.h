#ifndef TENSOR_TO_SCALAR_SIMPLIFIER_POW_H
#define TENSOR_TO_SCALAR_SIMPLIFIER_POW_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/simplifier/simplifier_pow.h>
#include <numsim_cas/tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_mul.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_domain_traits.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

using tensor_to_scalar_traits = domain_traits<tensor_to_scalar_expression>;

namespace tensor_to_scalar_detail {
namespace simplifier {

// Thin wrapper: fallback dispatch (no LHS specialization)
class pow_default_visitor final
    : public tensor_to_scalar_visitor_return_expr_t,
      public detail::pow_dispatch<tensor_to_scalar_traits, void> {
  using algo = detail::pow_dispatch<tensor_to_scalar_traits, void>;

public:
  using expr_holder_t = expression_holder<tensor_to_scalar_expression>;
  using algo::algo;

#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override { return this->dispatch(n); }
  NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

// Thin wrapper: LHS is pow — pow(pow(x,a),b) → pow(x,a*b)
// Virtual function bodies defined in .cpp to keep pow() ADL in that TU.
class pow_pow_visitor final
    : public tensor_to_scalar_visitor_return_expr_t,
      public detail::pow_pow_dispatch<tensor_to_scalar_traits> {
  using algo = detail::pow_pow_dispatch<tensor_to_scalar_traits>;

public:
  using expr_holder_t = expression_holder<tensor_to_scalar_expression>;
  using algo::algo;

#define NUMSIM_LOOP_OVER(T) expr_holder_t operator()(T const &n) override;
  NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

// Thin wrapper: LHS is mul — extract nested pow factors
// Virtual function bodies defined in .cpp to keep pow() ADL in that TU.
class mul_pow_visitor final
    : public tensor_to_scalar_visitor_return_expr_t,
      public detail::mul_pow_dispatch<tensor_to_scalar_traits> {
  using algo = detail::mul_pow_dispatch<tensor_to_scalar_traits>;

public:
  using expr_holder_t = expression_holder<tensor_to_scalar_expression>;
  using algo::algo;

#define NUMSIM_LOOP_OVER(T) expr_holder_t operator()(T const &n) override;
  NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

// Dispatcher: analyzes LHS type and creates appropriate specialized visitor
struct pow_base final : public tensor_to_scalar_visitor_return_expr_t {
  using expr_holder_t = expression_holder<tensor_to_scalar_expression>;

  pow_base(expr_holder_t lhs, expr_holder_t rhs);

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST,
                                        NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  expr_holder_t dispatch(tensor_to_scalar_pow const &);

  expr_holder_t dispatch(tensor_to_scalar_mul const &);

  template <typename Type> expr_holder_t dispatch(Type const &) {
    auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
    pow_default_visitor visitor(std::move(m_lhs), std::move(m_rhs));
    return _rhs.accept(visitor);
  }

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

} // namespace simplifier
} // namespace tensor_to_scalar_detail
} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_SIMPLIFIER_POW_H
