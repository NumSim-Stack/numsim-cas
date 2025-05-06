#ifndef SYMTM_PRINT_TEST_H
#define SYMTM_PRINT_TEST_H

#include <sstream>
#include "symTM.h"
#include "gtest/gtest.h"

using Seq = std::vector<std::size_t>;



//TEST(gtest, print_tensor){
//  auto [X,Y,Z]{symTM::make_tensor_variable<double>(symTM::tuple("X", 3, 2),symTM::tuple("Y", 3, 2),symTM::tuple("Z", 3, 2))};
//  EXPECT_EQ("X", std::to_string(X));
//  auto expr = X+Y+Z;
//  EXPECT_EQ("X+Y+Z", std::to_string(expr));
//  expr = -X-Y-Z;
//  EXPECT_EQ("-X-Y-Z", std::to_string(expr));
//  expr = X-(Y+Z);
//  EXPECT_EQ("X-(Y+Z)", std::to_string(expr));
//  expr = -X-(Y+Z);
//  EXPECT_EQ("-X-(Y+Z)", std::to_string(expr));
//  expr = symTM::basis_change(X, Seq{2,1});
//  EXPECT_EQ("basis_change<2,1>(X)", std::to_string(expr));
//  expr = symTM::inner_product(X, Seq{2}, Y, Seq{2});
//  EXPECT_EQ("inner_product<<2>,<2>>(X,Y)", std::to_string(expr));
//  expr = symTM::outer_product(X, Seq{2,1}, Y, Seq{4,3});
//  EXPECT_EQ("outer_product<<2,1>,<4,3>>(X,Y)", std::to_string(expr));
//}

#endif // SYMTM_PRINT_TEST_H
