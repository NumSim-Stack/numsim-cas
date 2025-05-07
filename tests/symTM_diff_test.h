#ifndef SYMTM_DIFF_TEST_H
#define SYMTM_DIFF_TEST_H

#include <string>
#include "symTM.h"
#include "gtest/gtest.h"

//TEST(gtest, diff_scalar){
//  auto [X,Y,Z]{numsim::cas::make_scalar_variable<double>("X", "Y", "Z")};
//  auto expr = X;
//  auto dexpr = diff(expr, X);
//  EXPECT_EQ("simple_outer(I, I)", std::to_string(dexpr));

//  //expr := symbol assigning an expression which contains the
//  //same variable result into an inf loop, therefor free the expr
//  expr.free();
//  expr = -X + Y + Z;
//  dexpr = diff(expr, X);
//  EXPECT_EQ("-simple_outer(I, I)", std::to_string(dexpr));

//  expr = X + Y + Z;
//  dexpr = diff(expr, X);
//  EXPECT_EQ("simple_outer(I, I)", std::to_string(dexpr));
//}

//TEST(gtest, diff_tensor){
//  auto [X,Y,Z]{numsim::cas::make_tensor_variable<double>(numsim::cas::tuple("X", 3, 2),numsim::cas::tuple("Y", 3, 2),numsim::cas::tuple("Z", 3, 2))};
//  auto expr = X;
//  auto dexpr = diff(expr, X);
//  EXPECT_EQ("basis_change<1,3,2,4>(simple_outer(I, I))", std::to_string(dexpr));

//  //expr := symbol assigning an expression which contains the
//  //same variable result into an inf loop, therefor free the expr
//  expr.free();
//  expr = -X + Y + Z;
//  dexpr = diff(expr, X);
//  EXPECT_EQ("-basis_change<1,3,2,4>(simple_outer(I, I))", std::to_string(dexpr));

//  expr = X + Y + Z;
//  dexpr = diff(expr, X);
//  EXPECT_EQ("basis_change<1,3,2,4>(simple_outer(I, I))", std::to_string(dexpr));
//}


#endif // SYMTM_DIFF_TEST_H
