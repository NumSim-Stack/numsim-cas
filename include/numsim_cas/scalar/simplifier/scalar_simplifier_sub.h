#ifndef SCALAR_SIMPLIFIER_SUB_H
#define SCALAR_SIMPLIFIER_SUB_H

#include <numsim_cas/core/simplifier/simplifier_sub.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_domain_traits.h>
#include <numsim_cas/scalar/scalar_globals.h>

namespace numsim::cas {

using scalar_traits = domain_traits<scalar_expression>;

namespace simplifier {

// Thin wrapper: fallback dispatch (no LHS specialization)
class sub_default_visitor final
    : public scalar_visitor_return_expr_t,
      public detail::sub_dispatch<scalar_traits, void> {
  using algo = detail::sub_dispatch<scalar_traits, void>;

public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using algo::algo;

#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override { return this->dispatch(n); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

// Thin wrapper: LHS is negative
class negative_sub final
    : public scalar_visitor_return_expr_t,
      public detail::negative_sub_dispatch<scalar_traits> {
  using algo = detail::negative_sub_dispatch<scalar_traits>;

public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using algo::algo;

#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override { return this->dispatch(n); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

// Thin wrapper: LHS is constant
class constant_sub final
    : public scalar_visitor_return_expr_t,
      public detail::constant_sub_dispatch<scalar_traits> {
  using algo = detail::constant_sub_dispatch<scalar_traits>;

public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using algo::algo;

#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override { return this->dispatch(n); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

// Thin wrapper: LHS is add (n-ary sum)
class n_ary_sub final
    : public scalar_visitor_return_expr_t,
      public detail::n_ary_sub_dispatch<scalar_traits> {
  using algo = detail::n_ary_sub_dispatch<scalar_traits>;

public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using algo::algo;

#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override { return this->dispatch(n); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

// Thin wrapper: LHS is mul
class n_ary_mul_sub final
    : public scalar_visitor_return_expr_t,
      public detail::n_ary_mul_sub_dispatch<scalar_traits> {
  using algo = detail::n_ary_mul_sub_dispatch<scalar_traits>;

public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using algo::algo;

#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override { return this->dispatch(n); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

// Thin wrapper: LHS is symbol
class symbol_sub final
    : public scalar_visitor_return_expr_t,
      public detail::symbol_sub_dispatch<scalar_traits> {
  using algo = detail::symbol_sub_dispatch<scalar_traits>;

public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using algo::algo;

#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override { return this->dispatch(n); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

// Thin wrapper: LHS is one
class scalar_one_sub final
    : public scalar_visitor_return_expr_t,
      public detail::one_sub_dispatch<scalar_traits> {
  using algo = detail::one_sub_dispatch<scalar_traits>;

public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using algo::algo;

#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override { return this->dispatch(n); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

// Dispatcher: analyzes LHS type and creates appropriate specialized visitor
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
