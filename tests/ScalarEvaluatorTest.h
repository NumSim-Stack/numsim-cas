// #ifndef SCALAREVALUATORTEST_H
// #define SCALAREVALUATORTEST_H

// #include "cas_test_helpers.h"
// #include "numsim_cas/numsim_cas.h"
// #include "gtest/gtest.h"

// #include <cmath>
// #include <limits>
// #include <tuple>
// #include <type_traits>

// // ---------- Scalar fixture ----------
// template <typename T> struct ScalarEvalFixture : ::testing::Test {
//   using value_type = T;
//   using scalar_expr =
//       numsim::cas::expression_holder<numsim::cas::scalar_expression<T>>;

//   scalar_expr x, y, z;
//   scalar_expr _1, _2, _3;
//   scalar_expr _zero{numsim::cas::get_scalar_zero<T>()};
//   scalar_expr _one{numsim::cas::get_scalar_one<T>()};

//   ScalarEvalFixture() {
//     std::tie(x, y, z) = numsim::cas::make_scalar_variable<T>("x", "y", "z");
//     std::tie(_1, _2, _3) = numsim::cas::make_scalar_constant<T>(1, 2, 3);
//   }

//   // ---- Helper: set variable value robustly ----
//   inline static void set_value(scalar_expr &var, value_type v) {
//     auto &s = var.template get<numsim::cas::scalar<value_type>>();

//     if constexpr (requires { s.data() = v; }) {
//       // data() returns a mutable reference
//       s.data() = v;
//     } else if constexpr (requires { s.set_data(v); }) {
//       s.set_data(v);
//     } else if constexpr (requires { s.data(v); }) {
//       // setter overload
//       s.data(v);
//     } else {
//       static_assert(sizeof(value_type) == 0,
//                     "No supported way to set scalar<T> value. "
//                     "Add one of: data() ref, set_data(T), or data(T).");
//     }
//   }

//   inline static value_type tol() {
//     if constexpr (std::is_same_v<value_type, float>)
//       return static_cast<value_type>(1e-5f);
//     else if constexpr (std::is_same_v<value_type, double>)
//       return static_cast<value_type>(1e-12);
//     else
//       return static_cast<value_type>(0);
//   }

//   inline static void expect_eq_or_near(value_type actual, value_type
//   expected) {
//     if constexpr (std::is_floating_point_v<value_type>) {
//       EXPECT_NEAR(actual, expected, tol());
//     } else {
//       EXPECT_EQ(actual, expected);
//     }
//   }
// };

// using ScalarTypes =
//     ::testing::Types<double, float, int /*, std::complex<double>*/>;
// TYPED_TEST_SUITE(ScalarEvalFixture, ScalarTypes);

// // Bring numeric overloads; ADL will pick CAS overloads for expressions
// using std::abs;
// using std::acos;
// using std::asin;
// using std::atan;
// using std::cos;
// using std::exp;
// using std::log;
// using std::pow;
// using std::sin;
// using std::sqrt;
// using std::tan;

// //
// // EVAL_FUNDAMENTALS — literals, constants, basic arithmetic
// //
// TYPED_TEST(ScalarEvalFixture, EVAL_Fundamentals) {
//   using value_type = typename TestFixture::value_type;
//   numsim::cas::scalar_evaluator<value_type> ev;

//   auto &_1 = this->_1;
//   auto &_2 = this->_2;
//   auto &_3 = this->_3;

//   this->expect_eq_or_near(ev.apply(_1), static_cast<value_type>(1));
//   this->expect_eq_or_near(ev.apply(_1 + _2 + _3),
//   static_cast<value_type>(6)); this->expect_eq_or_near(ev.apply(_2 * _3),
//   static_cast<value_type>(6));

//   // Division behavior depends on ValueType (int truncates)
//   if constexpr (std::is_floating_point_v<value_type>) {
//     this->expect_eq_or_near(ev.apply(_1 / _2), static_cast<value_type>(0.5));
//   } else {
//     this->expect_eq_or_near(ev.apply(_1 / _2),
//                             static_cast<value_type>(0)); // 1/2 -> 0
//   }
// }

// //
// // EVAL_VARIABLES — variable lookup + mixed expression
// //
// TYPED_TEST(ScalarEvalFixture, EVAL_Variables) {
//   using value_type = typename TestFixture::value_type;
//   numsim::cas::scalar_evaluator<value_type> ev;

//   auto &x = this->x;
//   auto &y = this->y;

//   TestFixture::set_value(x, static_cast<value_type>(2));
//   TestFixture::set_value(y, static_cast<value_type>(3));

//   // 2 + 3*2 + 1 = 9
//   auto expr = x + y * this->_2 + this->_1;
//   this->expect_eq_or_near(ev.apply(expr), static_cast<value_type>(9));

//   // -(x - y) = -(2-3)=1
//   auto expr2 = -(x - y);
//   this->expect_eq_or_near(ev.apply(expr2), static_cast<value_type>(1));
// }

// //
// // EVAL_ALL_NODE_TYPES — ensure every node type has an overload and runs
// //
// TYPED_TEST(ScalarEvalFixture, EVAL_CoversAllNodeTypes) {
//   using value_type = typename TestFixture::value_type;
//   numsim::cas::scalar_evaluator<value_type> ev;

//   auto &x = this->x;
//   auto &zero = this->_zero;
//   auto &one = this->_one;
//   auto &_1 = this->_1;
//   auto &_2 = this->_2;
//   auto &_3 = this->_3;

//   // ---- Base nodes ----
//   this->expect_eq_or_near(ev.apply(zero), static_cast<value_type>(0));
//   this->expect_eq_or_near(ev.apply(one), static_cast<value_type>(1));
//   this->expect_eq_or_near(ev.apply(_2), static_cast<value_type>(2));

//   // ---- Choose inputs depending on ValueType ----
//   if constexpr (std::is_floating_point_v<value_type>) {
//     TestFixture::set_value(x, static_cast<value_type>(0.5));

//     // Algebraic nodes
//     this->expect_eq_or_near(ev.apply(x), static_cast<value_type>(0.5));
//     this->expect_eq_or_near(ev.apply(x + _2), static_cast<value_type>(2.5));
//     this->expect_eq_or_near(ev.apply(x * _2), static_cast<value_type>(1.0));
//     this->expect_eq_or_near(ev.apply(-x), static_cast<value_type>(-0.5));
//     this->expect_eq_or_near(ev.apply(x / _2), static_cast<value_type>(0.25));

//     // scalar_function
//     auto func =
//         numsim::cas::make_expression<numsim::cas::scalar_function<value_type>>(
//             "f", x * x + _1);
//     this->expect_eq_or_near(ev.apply(func), static_cast<value_type>(1.25));

//     // Transcendentals (compare to std)
//     this->expect_eq_or_near(ev.apply(sin(x)),
//                             static_cast<value_type>(std::sin(0.5)));
//     this->expect_eq_or_near(ev.apply(cos(x)),
//                             static_cast<value_type>(std::cos(0.5)));
//     this->expect_eq_or_near(ev.apply(tan(x)),
//                             static_cast<value_type>(std::tan(0.5)));

//     this->expect_eq_or_near(ev.apply(asin(x)),
//                             static_cast<value_type>(std::asin(0.5)));
//     this->expect_eq_or_near(ev.apply(acos(x)),
//                             static_cast<value_type>(std::acos(0.5)));
//     this->expect_eq_or_near(ev.apply(atan(x)),
//                             static_cast<value_type>(std::atan(0.5)));

//     this->expect_eq_or_near(ev.apply(exp(x)),
//                             static_cast<value_type>(std::exp(0.5)));
//     this->expect_eq_or_near(ev.apply(sqrt(x)),
//                             static_cast<value_type>(std::sqrt(0.5)));
//     this->expect_eq_or_near(ev.apply(log(x)),
//                             static_cast<value_type>(std::log(0.5)));

//     this->expect_eq_or_near(ev.apply(pow(x, _2)),
//                             static_cast<value_type>(std::pow(0.5, 2.0)));

//     // abs/sign
//     auto absx =
//         numsim::cas::make_expression<numsim::cas::scalar_abs<value_type>>(-x);
//     auto signx =
//         numsim::cas::make_expression<numsim::cas::scalar_sign<value_type>>(x);
//     this->expect_eq_or_near(ev.apply(absx), static_cast<value_type>(0.5));
//     this->expect_eq_or_near(ev.apply(signx), static_cast<value_type>(1));
//   } else {
//     // Integral: pick integer inputs (avoid fractional expectations)
//     TestFixture::set_value(x, static_cast<value_type>(2));

//     // Algebraic nodes
//     this->expect_eq_or_near(ev.apply(x), static_cast<value_type>(2));
//     this->expect_eq_or_near(ev.apply(x + _2), static_cast<value_type>(4));
//     this->expect_eq_or_near(ev.apply(x * _2), static_cast<value_type>(4));
//     this->expect_eq_or_near(ev.apply(-x), static_cast<value_type>(-2));
//     this->expect_eq_or_near(ev.apply(x / _2),
//                             static_cast<value_type>(1)); // 2/2

//     // scalar_function
//     auto func =
//         numsim::cas::make_expression<numsim::cas::scalar_function<value_type>>(
//             "f", x * x + _1); // 4+1=5
//     this->expect_eq_or_near(ev.apply(func), static_cast<value_type>(5));

//     // Still hit trig/invtrig/exp/log/sqrt nodes at safe points:
//     // use 0 for trig/invtrig; 1 for log; 4 for sqrt.
//     this->expect_eq_or_near(ev.apply(sin(zero)), static_cast<value_type>(0));
//     this->expect_eq_or_near(ev.apply(cos(zero)), static_cast<value_type>(1));
//     this->expect_eq_or_near(ev.apply(tan(zero)), static_cast<value_type>(0));

//     this->expect_eq_or_near(ev.apply(asin(zero)),
//     static_cast<value_type>(0));
//     // acos(0) = pi/2 -> truncates to 1 for int (implementation uses
//     std::acos
//     // -> double -> int)
//     this->expect_eq_or_near(ev.apply(acos(zero)),
//                             static_cast<value_type>(std::acos(0.0)));
//     this->expect_eq_or_near(ev.apply(atan(zero)),
//     static_cast<value_type>(0));

//     this->expect_eq_or_near(ev.apply(exp(zero)), static_cast<value_type>(1));
//     this->expect_eq_or_near(ev.apply(log(one)), static_cast<value_type>(0));

//     // sqrt(4) via sqrt(1+3)
//     auto four = _1 + _3;
//     this->expect_eq_or_near(ev.apply(sqrt(four)),
//     static_cast<value_type>(2));

//     // pow(2,3) = 8
//     this->expect_eq_or_near(ev.apply(pow(_2, _3)),
//     static_cast<value_type>(8));

//     // abs/sign
//     auto absx =
//         numsim::cas::make_expression<numsim::cas::scalar_abs<value_type>>(-x);
//     auto signp =
//         numsim::cas::make_expression<numsim::cas::scalar_sign<value_type>>(x);
//     auto signn =
//         numsim::cas::make_expression<numsim::cas::scalar_sign<value_type>>(-x);
//     this->expect_eq_or_near(ev.apply(absx), static_cast<value_type>(2));
//     this->expect_eq_or_near(ev.apply(signp), static_cast<value_type>(1));
//     this->expect_eq_or_near(ev.apply(signn), static_cast<value_type>(-1));
//   }
// }

// //
// // EVAL_EXCEPTIONS — division by zero, sqrt domain, log domain (floating
// only)
// //
// TYPED_TEST(ScalarEvalFixture, EVAL_Throws) {
//   using value_type = typename TestFixture::value_type;
//   numsim::cas::scalar_evaluator<value_type> ev;

//   auto &x = this->x;
//   TestFixture::set_value(x, static_cast<value_type>(1));

//   // division by zero
//   EXPECT_THROW(ev.apply(x / this->_zero), std::runtime_error);

//   if constexpr (std::is_floating_point_v<value_type>) {
//     // sqrt(-1) should throw in real evaluation
//     auto minus_one =
//         numsim::cas::make_expression<numsim::cas::scalar_constant<value_type>>(
//             static_cast<value_type>(-1));
//     EXPECT_THROW(ev.apply(sqrt(minus_one)), std::runtime_error);

//     // log(0) / log(-1) should throw in real evaluation
//     EXPECT_THROW(ev.apply(log(this->_zero)), std::runtime_error);
//     EXPECT_THROW(ev.apply(log(minus_one)), std::runtime_error);
//   }
// }
// #endif // SCALAREVALUATORTEST_H
