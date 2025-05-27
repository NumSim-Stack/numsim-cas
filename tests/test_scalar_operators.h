#ifndef TEST_SCALAR_OPERATORS_H
#define TEST_SCALAR_OPERATORS_H

#include "numsim_cas/numsim_cas.h"
#include "gtest/gtest.h"

//TEST(gtest, scalar_add_variables){
//  auto [x,y,z]{numsim_cas::make_scalar_variable<double>("x","y","z")};
//  auto expr{x+y+z};
//  EXPECT_EQ(std::to_string(expr), "x+y+z");
//}

//TEST(gtest, scalar_add_same_variables){
//  auto [x,y,z]{numsim_cas::make_scalar_variable<double>("x","y","z")};
//  auto expr{x+y+z+x+y+z};
//  EXPECT_EQ(std::to_string(expr), "2*x+2*y+2*z");
//}

//TEST(gtest, scalar_add_constant_variable){
//  auto [x,y,z]{numsim_cas::make_scalar_variable<double>("x","y","z")};
//  auto [_1,_2,_3]{numsim_cas::make_scalar_constant<double>(1,2,3)};
//  auto expr{x+_1+y+_2+z+_3};
//  EXPECT_EQ(std::to_string(expr), "6+x+y+z");
//}

//TEST(gtest, scalar_add_constant_variable_inplace){
//  auto [x,y,z]{numsim_cas::make_scalar_variable<double>("x","y","z")};
//  auto expr{x+1+y+2+z+3};
//  EXPECT_EQ(std::to_string(expr), "6+x+y+z");
//}

#endif // TEST_SCALAR_OPERATORS_H
