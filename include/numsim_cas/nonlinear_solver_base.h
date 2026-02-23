// #ifndef NONLINEAR_SOLVER_BASE_H
// #define NONLINEAR_SOLVER_BASE_H

// #include "expression_holder.h"
// #include <eigen3/Eigen/Eigen>

// namespace numsim::cas {

// // nonlinear_scalar_solver solver;
// // solver.add_equation(x);
// // solver.add_variable(y);
// // solver.solve();

// template <typename ValueType, typename... Args> class visitor_diff {
// public:
//   using value_type = std::variant<symTM::expression_holder<Args>...>;

//   explicit visitor_diff(value_type const &expr) : m_arg(expr) {}

//   constexpr inline auto
//   operator()(expression_holder<scalar_expression<ValueType>> &expr) {
//     // scalar_diff<ValueType> diff(m_arg);
//     return expr;
//   }

// private:
//   value_type const &m_arg;
// };

// template <typename ValueType> class visitor_eval {
// public:
//   visitor_eval() {}

//   constexpr inline auto
//   operator()(expression_holder<scalar_expression<ValueType>> &expr) {
//     return;
//   }
// };

// template <typename ValueType> class visitor_assign {
// public:
//   visitor_assign() {}

//   constexpr inline auto
//   operator()(expression_holder<scalar_expression<ValueType>> &expr) {
//     return;
//   }
// };

// // VisitorDiff, VisitorEval, VisitorAssign
// template <typename ValueType,
//           template <class T, class... ARGS> class VisitorDiff,
//           template <class T> class VisitorEval,
//           template <class T> class VisitorAssign,
//           typename... Args>
// class nonlinear_solver_base //: public solver_base<ValueType, Args...>
// {
// public:
//   using diff_visitor = VisitorDiff<ValueType, Args...>;
//   using eval_visitor = VisitorEval<ValueType>;
//   using assign_visitor = VisitorAssign<ValueType>;
//   using variant_type = std::variant<symTM::expression_holder<Args>...>;

//   nonlinear_solver_base() {}

//   template <typename Expr,
//             typename = typename std::enable_if<(
//                 ... || std::is_base_of_v<Args,
//                 symTM::get_type_t<Expr>>)>::type>
//   void add_equation(Expr &&expr) {
//     this->m_expr.emplace_back(std::forward<Expr>(expr));
//   }

//   template <typename Expr,
//             typename = typename std::enable_if<(
//                 ... || std::is_base_of_v<Args,
//                 symTM::get_type_t<Expr>>)>::type>
//   void add_variable(Expr &&expr) {
//     this->m_args.emplace_back(std::forward<Expr>(expr));
//   }

//   void solve() { setup_diff(); }

// private:
//   void setup_diff() {
//     m_jacobian.reserve(m_expr.size());
//     for (auto &r_i : m_expr) {
//       m_jacobian.push_back(std::vector<variant_type>());
//       m_jacobian.back().reserve(m_args.size());
//       for (auto &arg_i : m_args) {
//         diff_visitor visit(arg_i);
//         m_jacobian.back().emplace_back(std::visit(visit, r_i));
//       }
//     }
//   }

//   void run_eval() {
//     // m_x_old = m_x;
//     // m_x = m_x - solve(m_mat, m_r);
//   }

//   std::vector<variant_type> m_expr;
//   std::vector<variant_type> m_args;
//   std::vector<std::vector<variant_type>> m_jacobian;

//   Eigen::Matrix<ValueType, Eigen::Dynamic, Eigen::Dynamic> m_mat;
//   Eigen::Vector<ValueType, Eigen::Dynamic> m_r;
//   Eigen::Vector<ValueType, Eigen::Dynamic> m_x;
//   Eigen::Vector<ValueType, Eigen::Dynamic> m_x_old;
// };

// using nonlinear_scalar_solver =
//     nonlinear_solver_base<double, symTM::visitor_diff, symTM::visitor_eval,
//                           symTM::visitor_assign,
//                           symTM::scalar_expression<double>>;

// } // namespace numsim::cas

// #endif // NONLINEAR_SOLVER_BASE_H
