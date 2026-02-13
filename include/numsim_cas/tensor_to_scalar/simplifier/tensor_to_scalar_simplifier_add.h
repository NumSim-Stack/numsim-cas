#ifndef TENSOR_TO_SCALAR_SIMPLIFIER_ADD_H
#define TENSOR_TO_SCALAR_SIMPLIFIER_ADD_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/simplifier/simplifier_add.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_domain_traits.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

using tensor_to_scalar_traits = domain_traits<tensor_to_scalar_expression>;

namespace tensor_to_scalar_detail {
namespace simplifier {

// Thin wrapper: fallback dispatch (no LHS specialization)
class add_default_visitor final
    : public tensor_to_scalar_visitor_return_expr_t,
      public detail::add_dispatch<tensor_to_scalar_traits, void> {
  using algo = detail::add_dispatch<tensor_to_scalar_traits, void>;

public:
  using expr_holder_t = expression_holder<tensor_to_scalar_expression>;
  using algo::algo;

#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override { return this->dispatch(n); }
  NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

// Thin wrapper: LHS is add — with domain-specific scalar_wrapper override
class n_ary_add final
    : public tensor_to_scalar_visitor_return_expr_t,
      public detail::n_ary_add_dispatch<tensor_to_scalar_traits> {
  using algo = detail::n_ary_add_dispatch<tensor_to_scalar_traits>;

public:
  using expr_holder_t = expression_holder<tensor_to_scalar_expression>;
  using algo::algo;
  using algo::dispatch;

  // domain-specific: extract and combine scalar wrappers
  expr_holder_t dispatch(tensor_to_scalar_scalar_wrapper const &rhs);

#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override { return this->dispatch(n); }
  NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

// Thin wrapper: LHS is negative
class negative_add final
    : public tensor_to_scalar_visitor_return_expr_t,
      public detail::negative_add_dispatch<tensor_to_scalar_traits> {
  using algo = detail::negative_add_dispatch<tensor_to_scalar_traits>;

public:
  using expr_holder_t = expression_holder<tensor_to_scalar_expression>;
  using algo::algo;

#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override { return this->dispatch(n); }
  NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

// Dispatcher: analyzes LHS type and creates appropriate specialized visitor
class add_base final : public tensor_to_scalar_visitor_return_expr_t {
public:
  using expr_holder_t = expression_holder<tensor_to_scalar_expression>;

  add_base(expr_holder_t lhs, expr_holder_t rhs);

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST,
                                        NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  expr_holder_t dispatch(tensor_to_scalar_zero const &);

  expr_holder_t dispatch(tensor_to_scalar_add const &);

  expr_holder_t dispatch(tensor_to_scalar_negative const &);

  template <typename Type> expr_holder_t dispatch(Type const &);

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

} // namespace simplifier
} // namespace tensor_to_scalar_detail
} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_SIMPLIFIER_ADD_H
