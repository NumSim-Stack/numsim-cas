// #ifndef SCALAR_SIMPLIFIER_DIV_H
// #define SCALAR_SIMPLIFIER_DIV_H

// #include <numsim_cas/scalar/scalar_definitions.h>
// #include <numsim_cas/scalar/scalar_expression.h>
// #include <numsim_cas/core/operators.h>
// #include <numsim_cas/scalar/scalar_operators.h>
// namespace numsim::cas {

// namespace detail{
// template<typename T>
// struct scalar_number_type_visitor
// {
//   template<typename Type>
//   constexpr inline auto operator()([[maybe_unused]]Type const& val)const
//   noexcept{
//     if constexpr (std::is_same_v<T, Type>){
//       return true;
//     }else{
//       return false;
//     }
//   }
// };
// }

// template<typename T>
// constexpr inline auto is_same(scalar_number const& v){
//   return std::visit(detail::scalar_number_type_visitor<T>{}, v.raw());
// }

// namespace scalar_detail {
// namespace simplifier {

// template <typename ExprLHS, typename ExprRHS>
// struct div_default : public scalar_visitor_return_expr_t {
//   using expr_holder_t = expression_holder<scalar_expression>;

//   div_default(ExprLHS &&lhs, ExprRHS &&rhs)
//       : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs))
//       {}

// #define NUMSIM_ADD_OVR_FIRST(T)                                                \
//   expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
// #define NUMSIM_ADD_OVR_NEXT(T)                                                 \
//   expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
//   NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
// #undef NUMSIM_ADD_OVR_FIRST
// #undef NUMSIM_ADD_OVR_NEXT

//   template <typename Expr>
//   inline expr_holder_t dispatch(Expr const &) {
//     return get_default();
//   }

//   // expr / 1 --> expr
//   inline expr_holder_t dispatch(scalar_one const &) {
//     return std::forward<ExprLHS>(m_lhs);
//   }

//   inline expr_holder_t dispatch(scalar_pow const &rhs) {
//     if (m_lhs.get().hash_value() == rhs.expr_lhs().get().hash_value()) {
//       return pow(rhs.expr_lhs(), get_scalar_one() - rhs.expr_rhs());
//     }
//     return get_default();
//   }

// protected:
//   auto get_default() {
//     if (m_lhs.get().hash_value() == m_rhs.get().hash_value()) {
//       return get_scalar_one();
//     }
//     return make_expression<scalar_div>(m_lhs, m_rhs);
//   }

//   expr_holder_t m_lhs;
//   expr_holder_t m_rhs;
// };

// template <typename ExprLHS, typename ExprRHS>
// struct scalar_div_simplifier final : public div_default<ExprLHS, ExprRHS> {
//   using expr_holder_t = expression_holder<scalar_expression>;
//   using base = div_default<ExprLHS, ExprRHS>;
//   using base::dispatch;

//   scalar_div_simplifier(ExprLHS &&lhs, ExprRHS &&rhs)
//       : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
//         m_expr(m_lhs.template get<scalar_div>()) {}

// #define NUMSIM_ADD_OVR_FIRST(T)                                                \
//   expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
// #define NUMSIM_ADD_OVR_NEXT(T)                                                 \
//   expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
//   NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
// #undef NUMSIM_ADD_OVR_FIRST
// #undef NUMSIM_ADD_OVR_NEXT

//   // (a/b)/(c/d) --> a*d/(b*c)
//   inline expr_holder_t dispatch(scalar_div const &rhs) {
//     return make_expression<scalar_div>(m_expr.expr_lhs() * rhs.expr_rhs(),
//                                        m_expr.expr_rhs() * rhs.expr_lhs());
//   }

//   // a/b/c --> a/(b*c)
//   template <typename Expr>
//   inline expr_holder_t dispatch(Expr const &) {
//     return make_expression<scalar_div>(m_expr.expr_lhs(),
//                                        m_expr.expr_rhs() * m_rhs);
//   }

// private:
//   using base::m_lhs;
//   using base::m_rhs;
//   scalar_div const &m_expr;
// };

// template <typename ExprLHS, typename ExprRHS>
// struct constant_div_simplifier final : public div_default<ExprLHS, ExprRHS> {
//   using expr_holder_t = expression_holder<scalar_expression>;
//   using base = div_default<ExprLHS, ExprRHS>;
//   using base::dispatch;
//   using base::get_default;

//   constant_div_simplifier(ExprLHS &&lhs, ExprRHS &&rhs)
//       : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
//         m_expr(m_lhs.template get<scalar_constant>()) {}

// #define NUMSIM_ADD_OVR_FIRST(T)                                                \
//   expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
// #define NUMSIM_ADD_OVR_NEXT(T)                                                 \
//   expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
//   NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
// #undef NUMSIM_ADD_OVR_FIRST
// #undef NUMSIM_ADD_OVR_NEXT

//   inline expr_holder_t dispatch(scalar_constant const &rhs) {
//     // TODO check if both constants.value() == int --> rational
//     if (rhs.value() == 1) {
//       return m_lhs;
//     }

//     if(is_same<std::int64_t>(m_expr.value()) &&
//     is_same<std::int64_t>(rhs.value())){
//       return make_expression<scalar_rational>(m_lhs, m_rhs);
//     }

//     return make_expression<scalar_constant>(m_expr.value() / rhs.value());
//     // return get_default();
//   }

// private:
//   using base::m_lhs;
//   using base::m_rhs;
//   scalar_constant const &m_expr;
// };

// template <typename ExprLHS, typename ExprRHS>
// struct pow_div_simplifier final : public div_default<ExprLHS, ExprRHS> {
//   using expr_holder_t = expression_holder<scalar_expression>;
//   using base = div_default<ExprLHS, ExprRHS>;
//   using base::dispatch;

//   pow_div_simplifier(ExprLHS &&lhs, ExprRHS &&rhs)
//       : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
//         m_expr(m_lhs.template get<scalar_pow>()) {}

// #define NUMSIM_ADD_OVR_FIRST(T)                                                \
//   expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
// #define NUMSIM_ADD_OVR_NEXT(T)                                                 \
//   expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
//   NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
// #undef NUMSIM_ADD_OVR_FIRST
// #undef NUMSIM_ADD_OVR_NEXT

//   inline expr_holder_t dispatch(scalar const &rhs) {
//     if (rhs.hash_value() == m_expr.expr_lhs().get().hash_value()) {
//       const auto expo{m_expr.expr_rhs() - get_scalar_one()};
//       if (is_same<scalar_constant>(expo)) {
//         if (expo.template get<scalar_constant>().value() == 1) {
//           return m_rhs;
//         }
//       }
//       return make_expression<scalar_pow>(m_expr.expr_lhs(), expo);
//     }
//     return base::get_default();
//   }

//   // pow(lhs, expo_lhs) / pow(rhs, expo_rhs)
//   // lhs == rhs --> pow(expr, expo_lhs - expo_rhs)
//   inline expr_holder_t dispatch(scalar_pow const &rhs) {
//     if (m_expr.expr_lhs().get().hash_value() ==
//         rhs.expr_lhs().get().hash_value()) {
//       const auto expo{m_expr.expr_rhs() - rhs.expr_rhs()};
//       if (is_same<scalar_constant>(expo)) {
//         if (expo.template get<scalar_constant>().value() == 1) {
//           return rhs.expr_lhs();
//         }
//       }
//       return make_expression<scalar_pow>(m_expr.expr_lhs(), expo);
//     }
//     return base::get_default();
//   }

// private:
//   using base::m_lhs;
//   using base::m_rhs;
//   scalar_pow const &m_expr;
// };

// template <typename ExprLHS, typename ExprRHS>
// class div_base final : public scalar_visitor_return_expr_t {
// public:
//   using expr_holder_t = expression_holder<scalar_expression>;

//   div_base(ExprLHS &&lhs, ExprRHS &&rhs)
//       : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs))
//       {}

// #define NUMSIM_ADD_OVR_FIRST(T)                                                \
//   expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
// #define NUMSIM_ADD_OVR_NEXT(T)                                                 \
//   expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
//   NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
// #undef NUMSIM_ADD_OVR_FIRST
// #undef NUMSIM_ADD_OVR_NEXT

//   template <typename Type>
//   inline expr_holder_t dispatch(Type const &) {
//     auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
//     div_default<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
//                                           std::forward<ExprRHS>(m_rhs));
//     return _rhs.accept(visitor);
//   }

//   // (-rhs) / lhs -->
//   inline expr_holder_t dispatch(scalar_negative const &lhs) {
//     return -(lhs.expr() / m_rhs);
//   }

//   // 0 / expr
//   inline expr_holder_t dispatch(scalar_zero const &) {
//     return get_scalar_zero();
//   }

//   inline expr_holder_t dispatch(scalar_div const &) {
//     auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
//     scalar_div_simplifier<ExprLHS, ExprRHS> visitor(
//         std::forward<ExprLHS>(m_lhs), std::forward<ExprRHS>(m_rhs));
//     return _rhs.accept(visitor);
//   }

//   inline expr_holder_t dispatch(scalar_pow const &) {
//     auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
//     pow_div_simplifier<ExprLHS, ExprRHS>
//     visitor(std::forward<ExprLHS>(m_lhs),
//                                                  std::forward<ExprRHS>(m_rhs));
//     return _rhs.accept(visitor);
//   }

//   inline expr_holder_t dispatch(scalar_constant const &) {
//     auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
//     constant_div_simplifier<ExprLHS, ExprRHS> visitor(
//         std::forward<ExprLHS>(m_lhs), std::forward<ExprRHS>(m_rhs));
//     return _rhs.accept(visitor);
//   }

// private:
//   expr_holder_t m_lhs;
//   expr_holder_t m_rhs;
// };

// } // namespace simplifier
// } // namespace scalar_detail
// } // namespace numsim::cas

// #endif // SCALAR_SIMPLIFIER_DIV_H
