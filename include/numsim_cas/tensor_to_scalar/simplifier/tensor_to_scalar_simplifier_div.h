// #ifndef TENSOR_TO_SCALAR_SIMPLIFIER_DIV_H
// #define TENSOR_TO_SCALAR_SIMPLIFIER_DIV_H

// #include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

// namespace numsim::cas {
// namespace tensor_to_scalar_detail {
// namespace simplifier {

// template <typename Derived> class div_default  : public
// tensor_to_scalar_visitor_return_expr_t {
//   using expr_holder_t = expression_holder<tensor_to_scalar_expression>;

//   div_default(expr_holder_t lhs, expr_holder_t rhs)
//       : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

// protected:
// #define NUMSIM_ADD_OVR(T) \
//   expr_holder_t dispatch(T const &n) override { \
//         if constexpr (std::is_void_v<Derived>) { \
//           return dispatch(n); \
//     } else { \
//           return static_cast<Derived *>(this)->dispatch(n); \
//     } \
//   }
//   NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_ADD_OVR, NUMSIM_ADD_OVR)
// #undef NUMSIM_ADD_OVR

//   auto get_default() {
//     auto div_new{make_expression<tensor_to_scalar_div>(m_lhs, m_rhs)};
//     return std::move(div_new);
//   }

//   template <typename Expr> expr_holder_t dispatch(Expr const &) {
//     return get_default();
//   }

//   expr_holder_t m_lhs;
//   expr_holder_t m_rhs;
// };

// template <typename ExprLHS, typename ExprRHS>
// class wrapper_tensor_to_scalar_div_div final
//     : public div_default<ExprLHS, ExprRHS> {
// public:
//   using expr_holder_t = expression_holder<tensor_to_scalar_expression>;
//   using base = div_default<ExprLHS, ExprRHS>;

//   wrapper_tensor_to_scalar_div_div(expr_holder_t lhs, expr_holder_t rhs)
//       : base(std::move(lhs), std::move(rhs)),
//         m_data(m_lhs.template get<tensor_to_scalar_with_scalar_div>()) {}

//   // not needed.
//   // see operator overload dispatch
//   // using base::dispatch;

//   // (A/a) / (B/b) = (A*b)/(a*B)
//   expr_holder_t
//   dispatch(tensor_to_scalar_with_scalar_div const &rhs) {
//     return (m_data.expr_lhs() * rhs.expr_rhs()) /
//            (m_data.expr_rhs() * rhs.expr_lhs());
//   }

//   // (A/a) / (b/B) = (A*B)/(a*b)  (your existing one is OK)
//   expr_holder_t
//   dispatch(scalar_with_tensor_to_scalar_div const &rhs) {
//     return (m_data.expr_lhs() * rhs.expr_rhs()) /
//            (m_data.expr_rhs() * rhs.expr_lhs());
//   }

//   // generic RHS: (A/a)/R = A/(a*R)
//   template <class RhsNode>
//   expr_holder_t dispatch(RhsNode const &) {
//     return m_data.expr_lhs() /
//            (m_data.expr_rhs() * std::move(m_rhs));
//   }

// private:
//   using base::m_lhs;
//   using base::m_rhs;
//   tensor_to_scalar_with_scalar_div const &m_data;
// };

// template <typename ExprLHS, typename ExprRHS>
// class wrapper_scalar_div_div final : public div_default<ExprLHS, ExprRHS> {
// public:
//   using expr_holder_t = expression_holder<tensor_to_scalar_expression>;
//   using base = div_default<ExprLHS, ExprRHS>;

//   wrapper_scalar_div_div(expr_holder_t lhs, expr_holder_t rhs)
//       : base(std::move(lhs), std::move(rhs)),
//         m_data(m_lhs.template get<scalar_with_tensor_to_scalar_div>()) {}

//   // using base::dispatch;

//   // (a/A) / (d/B) = (a/d)*(B/A)  (your existing rule is OK)
//   expr_holder_t
//   dispatch(scalar_with_tensor_to_scalar_div const &rhs) {
//     return (m_data.expr_lhs() / rhs.expr_lhs()) *
//            (rhs.expr_rhs() / m_data.expr_rhs());
//   }

//   // (a/A) / (B/b) = (a*b)/(A*B)  FIXED
//   expr_holder_t
//   dispatch(tensor_to_scalar_with_scalar_div const &rhs) {
//     return (m_data.expr_lhs() * rhs.expr_rhs()) /
//            (m_data.expr_rhs() * rhs.expr_lhs());
//   }

//   // generic RHS: (a/A)/R = a/(A*R)
//   template <class RhsNode>
//   expr_holder_t dispatch(RhsNode const &) {
//     return m_data.expr_lhs() /
//            (m_data.expr_rhs() * std::move(m_rhs));
//   }

// private:
//   using base::m_lhs;
//   using base::m_rhs;
//   scalar_with_tensor_to_scalar_div const &m_data;
// };

// template <typename ExprLHS, typename ExprRHS>
// class wrapper_scalar_mul_div final : public div_default<ExprLHS, ExprRHS> {
// public:
//   using expr_holder_t = expression_holder<tensor_to_scalar_expression>;
//   using base = div_default<ExprLHS, ExprRHS>;

//   wrapper_scalar_mul_div(expr_holder_t lhs, expr_holder_t rhs)
//       : base(std::move(lhs), std::move(rhs)),
//         m_data(m_lhs.template get<tensor_to_scalar_with_scalar_mul>()) {}

//   // using base::dispatch;

//   // (A*c)/(B*d) = (c/d)*(A/B)  (your existing one is OK)
//   expr_holder_t
//   dispatch(tensor_to_scalar_with_scalar_mul const &expr) {
//     return (m_data.expr_rhs() / expr.expr_rhs()) *
//            (m_data.expr_lhs() / expr.expr_lhs());
//   }

//   // generic RHS: (A*c)/R = c*(A/R)
//   template <class RhsNode>
//   expr_holder_t dispatch(RhsNode const &) {
//     return m_data.expr_rhs() *
//            (m_data.expr_lhs() / std::move(m_rhs));
//   }

// private:
//   using base::m_lhs;
//   using base::m_rhs;
//   tensor_to_scalar_with_scalar_mul const &m_data;
// };

// template <typename ExprLHS, typename ExprRHS> struct div_base {
//   using expr_holder_t = expression_holder<tensor_to_scalar_expression>;

//   div_base(expr_holder_t lhs, expr_holder_t rhs)
//       : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

//   template <typename Type> expr_holder_t dispatch(Type const &) {
//     auto &expr_rhs{*m_rhs};
//     return visit(div_default<ExprLHS, ExprRHS>(std::move(m_lhs),
//                                                std::move(m_rhs)),
//                  expr_rhs);
//   }

//   expr_holder_t
//   dispatch(tensor_to_scalar_with_scalar_div const &) {
//     auto &expr_rhs{*m_rhs};
//     return visit(
//         wrapper_tensor_to_scalar_div_div<ExprLHS, ExprRHS>(
//             std::move(m_lhs), std::move(m_rhs)),
//         expr_rhs);
//   }

//   expr_holder_t
//   dispatch(scalar_with_tensor_to_scalar_div const &) {
//     auto &expr_rhs{*m_rhs};
//     return visit(
//         wrapper_scalar_div_div<ExprLHS, ExprRHS>(std::move(m_lhs),
//                                                  std::move(m_rhs)),
//         expr_rhs);
//   }

//   expr_holder_t
//   dispatch(tensor_to_scalar_with_scalar_mul const &) {
//     auto &expr_rhs{*m_rhs};
//     return visit(
//         wrapper_scalar_mul_div<ExprLHS, ExprRHS>(std::move(m_lhs),
//                                                  std::move(m_rhs)),
//         expr_rhs);
//   }

//   expr_holder_t m_lhs;
//   expr_holder_t m_rhs;
// };

// } // namespace simplifier
// } // namespace tensor_to_scalar_detail
// } // namespace numsim::cas

// #endif // TENSOR_TO_SCALAR_SIMPLIFIER_DIV_H
