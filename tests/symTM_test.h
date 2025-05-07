#ifndef TST_GTEST_H
#define TST_GTEST_H

#include <complex>
#include "symTM.h"
#include "gtest/gtest.h"

using complexd = std::complex<double>;
using complexf = std::complex<float>;

//template<typename ValueType, std::size_t Dim, std::size_t Rank>
//static constexpr inline auto& convert(numsim::cas::tensor_data_base<ValueType> & tensor){
//  return static_cast<numsim::cas::tensor_data<ValueType,Dim,Rank>&>(tensor);
//}

//template<typename ValueType, std::size_t Dim, std::size_t Rank>
//static constexpr inline auto& convert(std::unique_ptr<numsim::cas::tensor_data_base<ValueType>> & tensor){
//  return static_cast<numsim::cas::tensor_data<ValueType,Dim,Rank>&>(tensor);
//}




#define call_test(test_name, ValueType, Dim) \
test_name(ValueType, Dim, 1) \
    test_name(ValueType, Dim, 2) \
    test_name(ValueType, Dim, 3) \
    test_name(ValueType, Dim, 4) \
    test_name(ValueType, Dim, 5) \
    test_name(ValueType, Dim, 6) \
    test_name(ValueType, Dim, 7) \
    test_name(ValueType, Dim, 8)

//adding tensors
#define add_tensors(ValueType, Dim, Rank)  \
    TEST(gtest, add_tensors_##ValueType##_##Dim##_##Rank){ \
      auto [asym,bsym,csym]{numsim::cas::make_tensor_variable<ValueType>(numsim::cas::tuple("a", Dim, Rank),numsim::cas::tuple("b", Dim, Rank),numsim::cas::tuple("c", Dim, Rank))}; \
      tmech::tensor<ValueType, Dim, Rank> a, b, c; \
      a.randn(); \
      b.randn(); \
      c.randn(); \
      asym.get<numsim::cas::tensor<ValueType>>() = a; \
      bsym.get<numsim::cas::tensor<ValueType>>() = b; \
      csym.get<numsim::cas::tensor<ValueType>>() = c; \
const auto result{numsim::cas::evaluate(asym+bsym+csym)}; \
      EXPECT_EQ(true, tmech::almost_equal(numsim::cas::get_tensor<ValueType, Dim, Rank>(result), a+b+c, 5e-7)); \
}

//subtracting tensors
#define sub_tensors(ValueType, Dim, Rank)  \
TEST(gtest, sub_tensors_##ValueType##_##Dim##_##Rank){ \
      auto [asym,bsym,csym]{numsim::cas::make_tensor_variable<ValueType>(numsim::cas::tuple("a", Dim, Rank),numsim::cas::tuple("b", Dim, Rank),numsim::cas::tuple("c", Dim, Rank))}; \
      tmech::tensor<ValueType, Dim, Rank> a, b, c; \
      a.randn(); \
      b.randn(); \
      c.randn(); \
      asym.get<numsim::cas::tensor<ValueType>>() = a; \
      bsym.get<numsim::cas::tensor<ValueType>>() = b; \
      csym.get<numsim::cas::tensor<ValueType>>() = c; \
      const auto result{numsim::cas::evaluate(asym-bsym-csym)}; \
      EXPECT_EQ(true, tmech::almost_equal(numsim::cas::get_tensor<ValueType, Dim, Rank>(result), a-b-c, 5e-7)); \
}

#define negative_tensor(ValueType, Dim, Rank)  \
TEST(gtest, negative_tensor_##ValueType##_##Dim##_##Rank){ \
auto [asym]{numsim::cas::make_tensor_variable<ValueType>(numsim::cas::tuple("a", Dim, Rank))}; \
      tmech::tensor<ValueType, Dim, Rank> a; \
      a.randn(); \
      asym.get<numsim::cas::tensor<ValueType>>() = a; \
      const auto result{numsim::cas::evaluate(-asym)}; \
      EXPECT_EQ(true, tmech::almost_equal(numsim::cas::get_tensor<ValueType, Dim, Rank>(result), -a, 5e-7)); \
}

#define negative_sub_tensors(ValueType, Dim, Rank)  \
TEST(gtest, negative_sub_tensors_##ValueType##_##Dim##_##Rank){ \
      auto [asym,bsym,csym]{numsim::cas::make_tensor_variable<ValueType>(numsim::cas::tuple("a", Dim, Rank),numsim::cas::tuple("b", Dim, Rank),numsim::cas::tuple("c", Dim, Rank))}; \
      tmech::tensor<ValueType, Dim, Rank> a, b, c; \
      a.randn(); \
      b.randn(); \
      c.randn(); \
      asym.get<numsim::cas::tensor<ValueType>>() = a; \
      bsym.get<numsim::cas::tensor<ValueType>>() = b; \
      csym.get<numsim::cas::tensor<ValueType>>() = c; \
      const auto result{numsim::cas::evaluate(-asym-bsym-csym)}; \
      EXPECT_EQ(true, tmech::almost_equal(numsim::cas::get_tensor<ValueType, Dim, Rank>(result), -a-b-c, 5e-7)); \
}

#define negative_add_tensors(ValueType, Dim, Rank)  \
TEST(gtest, negative_add_tensors_##ValueType##_##Dim##_##Rank){ \
      auto [asym,bsym,csym]{numsim::cas::make_tensor_variable<ValueType>(numsim::cas::tuple("a", Dim, Rank),numsim::cas::tuple("b", Dim, Rank),numsim::cas::tuple("c", Dim, Rank))}; \
      tmech::tensor<ValueType, Dim, Rank> a, b, c; \
      a.randn(); \
      b.randn(); \
      c.randn(); \
      asym.get<numsim::cas::tensor<ValueType>>() = a; \
      bsym.get<numsim::cas::tensor<ValueType>>() = b; \
      csym.get<numsim::cas::tensor<ValueType>>() = c; \
      const auto result{numsim::cas::evaluate(-asym+bsym+csym)}; \
      EXPECT_EQ(true, tmech::almost_equal(numsim::cas::get_tensor<ValueType, Dim, Rank>(result), -a+b+c, 5e-7)); \
}


#define inner_product_34_12(ValueType, Dim, RankLHS, RankRHS)  \
TEST(gtest, inner_product_34_12_##ValueType##_##Dim##_##RankLHS##_##RankRHS){ \
      using Seq = std::vector<std::size_t>; \
      auto [asym,bsym]{numsim::cas::make_tensor_variable<ValueType>(numsim::cas::tuple("a", Dim, RankLHS),numsim::cas::tuple("b", Dim, RankRHS))}; \
      tmech::tensor<ValueType, Dim, RankLHS> a; \
      tmech::tensor<ValueType, Dim, RankRHS> b; \
      a.randn(); \
      b.randn(); \
      asym.get<numsim::cas::tensor<ValueType>>() = a; \
      bsym.get<numsim::cas::tensor<ValueType>>() = b; \
      const auto result{numsim::cas::evaluate(numsim::cas::inner_product(asym, Seq{3,4}, bsym, Seq{1,2}))}; \
      auto result_t{tmech::inner_product<tmech::sequence<3,4>,tmech::sequence<1,2>>(a,b)};\
      EXPECT_EQ(true, tmech::almost_equal(numsim::cas::get_tensor<ValueType, Dim, RankLHS+RankRHS-4>(result), result_t, 5e-7)); \
}

#define inner_product_12_34(ValueType, Dim, RankLHS, RankRHS)  \
TEST(gtest, inner_product_12_34_##ValueType##_##Dim##_##RankLHS##_##RankRHS){ \
      using Seq = std::vector<std::size_t>; \
      auto [asym,bsym]{numsim::cas::make_tensor_variable<ValueType>(numsim::cas::tuple("a", Dim, RankLHS),numsim::cas::tuple("b", Dim, RankRHS))}; \
      tmech::tensor<ValueType, Dim, RankLHS> a; \
      tmech::tensor<ValueType, Dim, RankRHS> b; \
      a.randn(); \
      b.randn(); \
      asym.get<numsim::cas::tensor<ValueType>>() = a; \
      bsym.get<numsim::cas::tensor<ValueType>>() = b; \
      const auto result{numsim::cas::evaluate(numsim::cas::inner_product(asym, Seq{1,2}, bsym, Seq{3,4}))}; \
      auto result_t{tmech::inner_product<tmech::sequence<1,2>,tmech::sequence<3,4>>(a,b)};\
      EXPECT_EQ(true, tmech::almost_equal(numsim::cas::get_tensor<ValueType, Dim, RankLHS+RankRHS-4>(result), result_t, 5e-7)); \
}

#define basis_change_(ValueType, Dim, Rank, Line, ...)  \
TEST(gtest, basis_change_##ValueType##_##Dim##_##Rank##_##Line){ \
      using Seq = std::vector<std::size_t>; \
      auto [asym]{numsim::cas::make_tensor_variable<ValueType>(numsim::cas::tuple("a", Dim, Rank))}; \
      tmech::tensor<ValueType, Dim, Rank> a; \
      a.randn(); \
      asym.get<numsim::cas::tensor<ValueType>>() = a; \
      const auto result{numsim::cas::evaluate(numsim::cas::basis_change(asym, Seq{__VA_ARGS__}))}; \
      auto result_t{tmech::basis_change<tmech::sequence<__VA_ARGS__>>(a)};\
      EXPECT_EQ(true, tmech::almost_equal(numsim::cas::get_tensor<ValueType, Dim, Rank>(result), result_t, 5e-7)); \
}

#define basis_change_expr(ValueType, Dim, Rank, Line, ...)  \
TEST(gtest, basis_change_expr_##ValueType##_##Dim##_##Rank##_##Line){ \
      using Seq = std::vector<std::size_t>; \
      auto [asym,bsym]{numsim::cas::make_tensor_variable<ValueType>(numsim::cas::tuple("a", Dim, Rank),numsim::cas::tuple("b", Dim, Rank))}; \
      tmech::tensor<ValueType, Dim, Rank> a,b; \
      a.randn(); \
      b.randn(); \
      asym.get<numsim::cas::tensor<ValueType>>() = a; \
      bsym.get<numsim::cas::tensor<ValueType>>() = b; \
      const auto result{numsim::cas::evaluate(numsim::cas::basis_change(asym+bsym, Seq{__VA_ARGS__}))}; \
      auto result_t{tmech::basis_change<tmech::sequence<__VA_ARGS__>>(a+b)};\
      EXPECT_EQ(true, tmech::almost_equal(numsim::cas::get_tensor<ValueType, Dim, Rank>(result), result_t, 5e-7)); \
}

#define inner_product_34_12_expr(ValueType, Dim, RankLHS, RankRHS)  \
TEST(gtest, inner_product_34_12_expr_##ValueType##_##Dim##_##RankLHS##_##RankRHS){ \
      using Seq = std::vector<std::size_t>; \
      auto [asym,bsym]{numsim::cas::make_tensor_variable<ValueType>(numsim::cas::tuple("a", Dim, RankLHS),numsim::cas::tuple("b", Dim, RankRHS))}; \
      tmech::tensor<ValueType, Dim, RankLHS> a; \
      tmech::tensor<ValueType, Dim, RankRHS> b; \
      a.randn(); \
      b.randn(); \
      asym.get<numsim::cas::tensor<ValueType>>() = a; \
      bsym.get<numsim::cas::tensor<ValueType>>() = b; \
      const auto result{numsim::cas::evaluate(numsim::cas::inner_product(asym+asym, Seq{3,4}, bsym+bsym, Seq{1,2}))}; \
      auto result_t{tmech::inner_product<tmech::sequence<3,4>,tmech::sequence<1,2>>(a+a,b+b)};\
      EXPECT_EQ(true, tmech::almost_equal(numsim::cas::get_tensor<ValueType, Dim, RankLHS+RankRHS-4>(result), result_t, 5e-7)); \
}

#define inner_product_12_34_expr(ValueType, Dim, RankLHS, RankRHS)  \
TEST(gtest, inner_product_12_34_expr_##ValueType##_##Dim##_##RankLHS##_##RankRHS){ \
      using Seq = std::vector<std::size_t>; \
      auto [asym,bsym]{numsim::cas::make_tensor_variable<ValueType>(numsim::cas::tuple("a", Dim, RankLHS),numsim::cas::tuple("b", Dim, RankRHS))}; \
      tmech::tensor<ValueType, Dim, RankLHS> a; \
      tmech::tensor<ValueType, Dim, RankRHS> b; \
      a.randn(); \
      b.randn(); \
      asym.get<numsim::cas::tensor<ValueType>>() = a; \
      bsym.get<numsim::cas::tensor<ValueType>>() = b; \
      const auto result{numsim::cas::evaluate(numsim::cas::inner_product(asym+asym, Seq{1,2}, bsym+bsym, Seq{3,4}))}; \
      auto result_t{tmech::inner_product<tmech::sequence<1,2>,tmech::sequence<3,4>>(a+a,b+b)};\
      EXPECT_EQ(true, tmech::almost_equal(numsim::cas::get_tensor<ValueType, Dim, RankLHS+RankRHS-4>(result), result_t, 5e-7)); \
}

//TEST(gtest, print_tensor_expression){
//}
TEST(gtest, scalar_add_neu){
      auto [x,y,z]{numsim::cas::make_scalar_variable<double>("x","y","z")};
  auto c{numsim::cas::make_scalar_constant<double>(1.0)};

      double _x{5.99},_y{8.89},_z{9.99};
      x.get<numsim::cas::scalar<double>>()=_x;
      y.get<numsim::cas::scalar<double>>()=_y;
      z.get<numsim::cas::scalar<double>>()=_z;
      EXPECT_EQ(_x+_y+_z, numsim::cas::evaluate(x+y+z));
}

#define scalar_add(ValueType)  \
TEST(gtest, scalar_add_##ValueType){ \
      auto [x,y,z]{numsim::cas::make_scalar_variable<ValueType>("x","y","z")}; \
      ValueType _x{5.99},_y{8.89},_z{9.99}; \
      x.get<numsim::cas::scalar<ValueType>>()=_x; \
      y.get<numsim::cas::scalar<ValueType>>()=_y; \
      z.get<numsim::cas::scalar<ValueType>>()=_z; \
      EXPECT_EQ(_x+_y+_z, numsim::cas::evaluate(x+y+z)); \
} \


#define scalar_sub(ValueType)  \
TEST(gtest, scalar_sub_##ValueType){ \
      auto [x,y,z]{numsim::cas::make_scalar_variable<ValueType>("x","y","z")}; \
      ValueType _x{5.99},_y{8.89},_z{9.99}; \
      x.get<numsim::cas::scalar<ValueType>>()=_x; \
      y.get<numsim::cas::scalar<ValueType>>()=_y; \
      z.get<numsim::cas::scalar<ValueType>>()=_z; \
      EXPECT_EQ(-_x-_y-_z, numsim::cas::evaluate(-x-y-z)); \
} \

#define scalar_complex_expr(ValueType)  \
TEST(gtest, scalar_complex_expr_##ValueType){ \
      auto [x,y,z]{numsim::cas::make_scalar_variable<ValueType>("x","y","z")}; \
      ValueType _x{5.99},_y{8.89},_z{9.99}; \
      x.get<numsim::cas::scalar<ValueType>>()=_x; \
      y.get<numsim::cas::scalar<ValueType>>()=_y; \
      z.get<numsim::cas::scalar<ValueType>>()=_z; \
      EXPECT_EQ(_x-_y+_z, numsim::cas::evaluate(x-y+z)); \
}

       /// TODO
       /// - outer_product
       /// - sym

//call_test(add_tensors, double, 1);
//call_test(add_tensors, double, 2);
//call_test(add_tensors, double, 3);

//call_test(sub_tensors, double, 1);
//call_test(sub_tensors, double, 2);
//call_test(sub_tensors, double, 3);

//call_test(negative_tensor, double, 1);
//call_test(negative_tensor, double, 2);
//call_test(negative_tensor, double, 3);

//call_test(negative_add_tensors, double, 1);
//call_test(negative_add_tensors, double, 2);
//call_test(negative_add_tensors, double, 3);

//call_test(negative_sub_tensors, double, 1);
//call_test(negative_sub_tensors, double, 2);
//call_test(negative_sub_tensors, double, 3);

//inner_product_34_12(double,1,4,2);
//inner_product_34_12(double,2,4,2);
//inner_product_34_12(double,3,4,2);

//inner_product_12_34(double,1,2,4);
//inner_product_12_34(double,2,2,4);
//inner_product_12_34(double,3,2,4);

//inner_product_34_12(double,1,4,4);
//inner_product_34_12(double,2,4,4);
//inner_product_34_12(double,3,4,4);

//inner_product_12_34(double,1,4,4);
//inner_product_12_34(double,2,4,4);
//inner_product_12_34(double,3,4,4);

//basis_change_(double,1,2, 0, 2,1);
//basis_change_(double,2,2, 1, 2,1);
//basis_change_(double,3,2, 2, 2,1);

//basis_change_(double,1,4, 3, 2,1,3,4);
//basis_change_(double,2,4, 4, 2,1,3,4);
//basis_change_(double,3,4, 5, 2,1,3,4);

//basis_change_(double,1,4, 6, 3,4,1,2);
//basis_change_(double,2,4, 7, 3,4,1,2);
//basis_change_(double,3,4, 8, 3,4,1,2);

/////expression variation
//basis_change_expr(double,1,2, 0, 2,1);
//basis_change_expr(double,2,2, 1, 2,1);
//basis_change_expr(double,3,2, 2, 2,1);

//basis_change_expr(double,1,4, 3, 2,1,3,4);
//basis_change_expr(double,2,4, 4, 2,1,3,4);
//basis_change_expr(double,3,4, 5, 2,1,3,4);

//basis_change_expr(double,1,4, 6, 3,4,1,2);
//basis_change_expr(double,2,4, 7, 3,4,1,2);
//basis_change_expr(double,3,4, 8, 3,4,1,2);

//inner_product_34_12_expr(double,1,4,2);
//inner_product_34_12_expr(double,2,4,2);
//inner_product_34_12_expr(double,3,4,2);

//inner_product_12_34_expr(double,1,2,4);
//inner_product_12_34_expr(double,2,2,4);
//inner_product_12_34_expr(double,3,2,4);

//inner_product_34_12_expr(double,1,4,4);
//inner_product_34_12_expr(double,2,4,4);
//inner_product_34_12_expr(double,3,4,4);

//inner_product_12_34_expr(double,1,4,4);
//inner_product_12_34_expr(double,2,4,4);
//inner_product_12_34_expr(double,3,4,4);

/////
//call_test(add_tensors, float, 1);
//call_test(add_tensors, float, 2);
//call_test(add_tensors, float, 3);

//call_test(sub_tensors, float, 1);
//call_test(sub_tensors, float, 2);
//call_test(sub_tensors, float, 3);

//call_test(negative_tensor, float, 1);
//call_test(negative_tensor, float, 2);
//call_test(negative_tensor, float, 3);

//call_test(negative_add_tensors, float, 1);
//call_test(negative_add_tensors, float, 2);
//call_test(negative_add_tensors, float, 3);

//call_test(negative_sub_tensors, float, 1);
//call_test(negative_sub_tensors, float, 2);
//call_test(negative_sub_tensors, float, 3);

//inner_product_34_12(float,1,4,2);
//inner_product_34_12(float,2,4,2);
//inner_product_34_12(float,3,4,2);

//inner_product_12_34(float,1,2,4);
//inner_product_12_34(float,2,2,4);
//inner_product_12_34(float,3,2,4);

//inner_product_34_12(float,1,4,4);
//inner_product_34_12(float,2,4,4);
//inner_product_34_12(float,3,4,4);

//inner_product_12_34(float,1,4,4);
//inner_product_12_34(float,2,4,4);
//inner_product_12_34(float,3,4,4);

//basis_change_(float,1,2, 0, 2,1);
//basis_change_(float,2,2, 1, 2,1);
//basis_change_(float,3,2, 2, 2,1);

//basis_change_(float,1,4, 3, 2,1,3,4);
//basis_change_(float,2,4, 4, 2,1,3,4);
//basis_change_(float,3,4, 5, 2,1,3,4);

//basis_change_(float,1,4, 6, 3,4,1,2);
//basis_change_(float,2,4, 7, 3,4,1,2);
//basis_change_(float,3,4, 8, 3,4,1,2);

/////expression variation
//basis_change_expr(float,1,2, 0, 2,1);
//basis_change_expr(float,2,2, 1, 2,1);
//basis_change_expr(float,3,2, 2, 2,1);

//basis_change_expr(float,1,4, 3, 2,1,3,4);
//basis_change_expr(float,2,4, 4, 2,1,3,4);
//basis_change_expr(float,3,4, 5, 2,1,3,4);

//basis_change_expr(float,1,4, 6, 3,4,1,2);
//basis_change_expr(float,2,4, 7, 3,4,1,2);
//basis_change_expr(float,3,4, 8, 3,4,1,2);

//inner_product_34_12_expr(float,1,4,2);
//inner_product_34_12_expr(float,2,4,2);
//inner_product_34_12_expr(float,3,4,2);

//inner_product_12_34_expr(float,1,2,4);
//inner_product_12_34_expr(float,2,2,4);
//inner_product_12_34_expr(float,3,2,4);

//inner_product_34_12_expr(float,1,4,4);
//inner_product_34_12_expr(float,2,4,4);
//inner_product_34_12_expr(float,3,4,4);

//inner_product_12_34_expr(float,1,4,4);
//inner_product_12_34_expr(float,2,4,4);
//inner_product_12_34_expr(float,3,4,4);


///// scalar
scalar_add(double);
scalar_sub(double);
scalar_complex_expr(double);

scalar_add(float);
scalar_sub(float);
scalar_complex_expr(float);

scalar_add(complexd);
scalar_sub(complexd);
scalar_complex_expr(complexd);

scalar_add(complexf);
scalar_sub(complexf);
scalar_complex_expr(complexf);

#endif // TST_GTEST_H
