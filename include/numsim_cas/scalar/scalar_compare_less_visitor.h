// #ifndef SCALAR_COMPARE_LESS_VISITOR_H
// #define SCALAR_COMPARE_LESS_VISITOR_H

// #include <numsim_cas/compare_less_visitor.h>
// #include <numsim_cas/expression_holder.h>
// #include <numsim_cas/scalar/scalar_expression.h>

// namespace numsim::cas {

// template <typename LHS>
// struct compare_less_pretty_print_rhs final : public scalar_visitor_const_t,
//                                              public
//                                              compare_less_pretty_print_base
// {
//   using compare_less_pretty_print_base::dispatch;

//   compare_less_pretty_print_rhs(LHS const& lhs)
//       : m_lhs(lhs) {}

// #define NUMSIM_ADD_OVR_FIRST(T)                                                \
//   void operator()(T const &lhs) const noexcept override { return dispatch(lhs); }
// #define NUMSIM_ADD_OVR_NEXTT)                                                 \
//   void operator()(T const &lhs) const noexcept override { return dispatch(lhs); }
//   NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
// #undef NUMSIM_ADD_OVR_FIRST
// #undef NUMSIM_ADD_OVR_NEXT

//   template<typename RHS>
//   constexpr inline auto dispatch(RHS const& rhs)const{
//     m_result = dispatch(m_lhs, rhs);
//   }

//   mutable bool m_result{false};
// private:
//   LHS const& m_lhs;
// };

// struct compare_less_pretty_print final : public scalar_visitor_const_t
// {
//   using expr_holder_t = expression_holder<scalar_expression>;

//   compare_less_pretty_print(){}

// #define NUMSIM_ADD_OVR_FIRST(T)                                                \
//   void operator()(T const &lhs) const noexcept override { return dispatch(lhs); }
// #define NUMSIM_ADD_OVR_NEXT(T)                                                 \
//   void operator()(T const &lhs) const noexcept override { return dispatch(lhs); }
//   NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
// #undef NUMSIM_ADD_OVR_FIRST
// #undef NUMSIM_ADD_OVR_NEXT

//   template<typename LHSType>
//   inline void dispatch(LHSType const& lhs)const noexcept{
//     compare_less_pretty_print_rhs<LHSType> visitor(lhs);
//     m_rhs->get<scalar_visitable_t>().accept(visitor);
//     m_result = visitor.m_result;
//   }

//   inline auto operator()(expr_holder_t const& lhs, expr_holder_t const&
//   rhs)const{
//     m_rhs = &rhs;
//     lhs.get<scalar_visitable_t>().accept(*this);
//     return m_result;
//   }

//   mutable bool m_result{false};
// private:
//   mutable expr_holder_t  const *m_rhs;
// };

// } // namespace numsim::cas

// #endif // SCALAR_COMPARE_LESS_VISITOR_H
