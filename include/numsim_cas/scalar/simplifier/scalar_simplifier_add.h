#ifndef SCALAR_SIMPLIFIER_ADD_H
#define SCALAR_SIMPLIFIER_ADD_H

#include <numsim_cas/core/simplifier/simplifier_add.h>
#include <numsim_cas/scalar/scalar_domain_traits.h>
#include <numsim_cas/scalar/scalar_globals.h>

namespace numsim::cas {

using scalar_traits = domain_traits<scalar_expression>;

namespace simplifier {

// Thin wrapper: fallback dispatch (no LHS specialization)
class add_default_visitor final
    : public scalar_visitor_return_expr_t,
      public detail::add_dispatch<scalar_traits, void> {
  using algo = detail::add_dispatch<scalar_traits, void>;

public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using algo::algo;

#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override { return this->dispatch(n); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

// Thin wrapper: LHS is constant
class constant_add final
    : public scalar_visitor_return_expr_t,
      public detail::constant_add_dispatch<scalar_traits> {
  using algo = detail::constant_add_dispatch<scalar_traits>;

public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using algo::algo;

#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override { return this->dispatch(n); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

// Thin wrapper: LHS is one
class add_scalar_one final
    : public scalar_visitor_return_expr_t,
      public detail::one_add_dispatch<scalar_traits> {
  using algo = detail::one_add_dispatch<scalar_traits>;

public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using algo::algo;

#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override { return this->dispatch(n); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

// Thin wrapper: LHS is add (n-ary sum)
class n_ary_add final
    : public scalar_visitor_return_expr_t,
      public detail::n_ary_add_dispatch<scalar_traits> {
  using algo = detail::n_ary_add_dispatch<scalar_traits>;

public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using algo::algo;

#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override { return this->dispatch(n); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

// Thin wrapper: LHS is mul
class n_ary_mul_add final
    : public scalar_visitor_return_expr_t,
      public detail::n_ary_mul_add_dispatch<scalar_traits> {
  using algo = detail::n_ary_mul_add_dispatch<scalar_traits>;

public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using algo::algo;

#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override { return this->dispatch(n); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

// Thin wrapper: LHS is symbol
class symbol_add final
    : public scalar_visitor_return_expr_t,
      public detail::symbol_add_dispatch<scalar_traits> {
  using algo = detail::symbol_add_dispatch<scalar_traits>;

public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using algo::algo;

#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override { return this->dispatch(n); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

// Thin wrapper: LHS is negative
class add_negative final
    : public scalar_visitor_return_expr_t,
      public detail::negative_add_dispatch<scalar_traits> {
  using algo = detail::negative_add_dispatch<scalar_traits>;

public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using algo::algo;

#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override { return this->dispatch(n); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

// Dispatcher: analyzes LHS type and creates appropriate specialized visitor
struct add_base final : public scalar_visitor_return_expr_t {
  using expr_holder_t = expression_holder<scalar_expression>;

  add_base(expr_holder_t lhs, expr_holder_t rhs);

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

private:
  expr_holder_t dispatch(scalar_constant const &);

  expr_holder_t dispatch(scalar_one const &);

  expr_holder_t dispatch(scalar_add const &);

  expr_holder_t dispatch(scalar_mul const &);

  expr_holder_t dispatch(scalar const &);

  expr_holder_t dispatch(scalar_negative const &);

  expr_holder_t dispatch(scalar_zero const &);

  template <typename Type> expr_holder_t dispatch(Type const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    add_default_visitor visitor(std::move(m_lhs), std::move(m_rhs));
    return _rhs.accept(visitor);
  }

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};
} // namespace simplifier

} // namespace numsim::cas
#endif // SCALAR_SIMPLIFIER_ADD_H
