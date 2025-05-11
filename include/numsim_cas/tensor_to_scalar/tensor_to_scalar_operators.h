#ifndef TENSOR_SCALAR_OPERATORS_H
#define TENSOR_SCALAR_OPERATORS_H

#include "../tensor/tensor_expression.h"
#include "../scalar/scalar_expression.h"
#include "tensor_to_scalar_expression.h"
#include "simplifier/tensor_to_scalar_simplifier_add.h"
#include "simplifier/tensor_to_scalar_simplifier_sub.h"
#include "simplifier/tensor_to_scalar_simplifier_mul.h"
#include "simplifier/tensor_to_scalar_simplifier_div.h"
#include "simplifier/tensor_to_scalar_with_scalar_simplifier_add.h"
#include "simplifier/tensor_to_scalar_with_scalar_simplifier_mul.h"


namespace numsim::cas {

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline auto binary_add_tensor_to_scalar_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs){
  return visit(tensor_to_scalar_detail::simplifier::add_base<ExprTypeLHS, ExprTypeRHS>(std::forward<ExprTypeLHS>(lhs),std::forward<ExprTypeRHS>(rhs)), *lhs);
}

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline auto binary_sub_tensor_to_scalar_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs){
  return visit(tensor_to_scalar_detail::simplifier::sub_base<ExprTypeLHS, ExprTypeRHS>(std::forward<ExprTypeLHS>(lhs),std::forward<ExprTypeRHS>(rhs)), *lhs);
}

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline auto binary_mul_tensor_to_scalar_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs){
  return visit(tensor_to_scalar_detail::simplifier::mul_base<ExprTypeLHS, ExprTypeRHS>(std::forward<ExprTypeLHS>(lhs),std::forward<ExprTypeRHS>(rhs)), *lhs);
}

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline auto binary_div_tensor_to_scalar_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs){
  return visit(tensor_to_scalar_detail::simplifier::div_base<ExprTypeLHS, ExprTypeRHS>(std::forward<ExprTypeLHS>(lhs),std::forward<ExprTypeRHS>(rhs)), *lhs);
}

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline auto binary_add_tensor_to_scalar_with_scalar_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs){
  return visit(tensor_to_scalar_with_scalar_detail::simplifier::add_base<ExprTypeLHS, ExprTypeRHS>(std::forward<ExprTypeLHS>(lhs),std::forward<ExprTypeRHS>(rhs)), *lhs);
}

template <typename ExprTypeLHS, typename ExprTypeRHS>
[[nodiscard]] constexpr inline auto binary_mul_tensor_to_scalar_with_scalar_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs){
  return visit(tensor_to_scalar_with_scalar_detail::simplifier::mul_base<ExprTypeLHS, ExprTypeRHS>(std::forward<ExprTypeLHS>(lhs),std::forward<ExprTypeRHS>(rhs)), *lhs);
}

template<typename ValueType>
struct operator_overload<expression_holder<tensor_to_scalar_expression<ValueType>>,
                         expression_holder<tensor_to_scalar_expression<ValueType>>>{

  template<typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto add(ExprTypeLHS && lhs, ExprTypeRHS && rhs){
    return binary_add_tensor_to_scalar_simplify(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
  }

  template<typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto mul(ExprTypeLHS && lhs, ExprTypeRHS && rhs){
    return binary_mul_tensor_to_scalar_simplify(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
  }

  template<typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto sub(ExprTypeLHS && lhs, ExprTypeRHS && rhs){
    return binary_sub_tensor_to_scalar_simplify(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
  }

  template<typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto div(ExprTypeLHS && lhs, ExprTypeRHS && rhs){
    return binary_div_tensor_to_scalar_simplify(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
  }
};


template<typename ValueType>
struct operator_overload<expression_holder<scalar_expression<ValueType>>,
                         expression_holder<tensor_to_scalar_expression<ValueType>>>{

  template<typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto add([[maybe_unused]]ExprTypeLHS && lhs, [[maybe_unused]]ExprTypeRHS && rhs){
    //return binary_add_tensor_to_scalar_with_scalar_simplify<tensor_to_scalar_expression<ValueType>>();
    return binary_add_tensor_to_scalar_with_scalar_simplify(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
  }

  template<typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto mul([[maybe_unused]]ExprTypeLHS && lhs, [[maybe_unused]]ExprTypeRHS && rhs){
    return binary_mul_tensor_to_scalar_with_scalar_simplify(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
  }

  template<typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto sub([[maybe_unused]]ExprTypeLHS && lhs, [[maybe_unused]]ExprTypeRHS && rhs){
    return expression_holder<tensor_to_scalar_expression<ValueType>>();
    //return binary_sub_tensor_simplify(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
  }

  template<typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto div([[maybe_unused]]ExprTypeLHS && lhs, [[maybe_unused]]ExprTypeRHS && rhs){
    return expression_holder<tensor_to_scalar_expression<ValueType>>();
    //return binary_sub_tensor_simplify(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
  }
};


template<typename ValueType>
struct operator_overload<expression_holder<tensor_to_scalar_expression<ValueType>>,
                         expression_holder<scalar_expression<ValueType>>>{

  template<typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto add([[maybe_unused]]ExprTypeLHS && lhs, [[maybe_unused]]ExprTypeRHS && rhs){
    return binary_add_tensor_to_scalar_with_scalar_simplify(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
  }

  template<typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto mul([[maybe_unused]]ExprTypeLHS && lhs, [[maybe_unused]]ExprTypeRHS && rhs){
    return binary_mul_tensor_to_scalar_with_scalar_simplify(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
  }

  template<typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto sub([[maybe_unused]]ExprTypeLHS && lhs, [[maybe_unused]]ExprTypeRHS && rhs){
    return expression_holder<tensor_to_scalar_expression<ValueType>>();
    //return binary_sub_tensor_simplify(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
  }

  template<typename ExprTypeLHS, typename ExprTypeRHS>
  [[nodiscard]] static constexpr inline auto div([[maybe_unused]]ExprTypeLHS && lhs, [[maybe_unused]]ExprTypeRHS && rhs){
    return expression_holder<tensor_to_scalar_expression<ValueType>>();
    //return binary_sub_tensor_simplify(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
  }
};


//template<typename ValueType>
//struct operator_overload<expression_holder<tensor_expression<ValueType>>,
//                         expression_holder<tensor_to_scalar_expression<ValueType>>>{

//  template<typename ExprTypeLHS, typename ExprTypeRHS>
//  [[nodiscard]] static constexpr inline auto add(ExprTypeLHS && lhs, ExprTypeRHS && rhs){
//    return binary_add_tensor_simplify(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
//  }

//  template<typename ExprTypeLHS, typename ExprTypeRHS>
//  [[nodiscard]] static constexpr inline auto mul(ExprTypeLHS && lhs, ExprTypeRHS && rhs){
//  }

//  template<typename ExprTypeLHS, typename ExprTypeRHS>
//  [[nodiscard]] static constexpr inline auto sub(ExprTypeLHS && lhs, ExprTypeRHS && rhs){
//    return binary_sub_tensor_simplify(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
//  }
//};


//template<typename ValueType>
//struct operator_overload<expression_holder<tensor_to_scalar_expression<ValueType>>,
//                         expression_holder<tensor_expression<ValueType>>>{

//  template<typename ExprTypeLHS, typename ExprTypeRHS>
//  [[nodiscard]] static constexpr inline auto add(ExprTypeLHS && lhs, ExprTypeRHS && rhs){
//    return binary_add_tensor_simplify(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
//  }

//  template<typename ExprTypeLHS, typename ExprTypeRHS>
//  [[nodiscard]] static constexpr inline auto mul(ExprTypeLHS && lhs, ExprTypeRHS && rhs){
//  }

//  template<typename ExprTypeLHS, typename ExprTypeRHS>
//  [[nodiscard]] static constexpr inline auto sub(ExprTypeLHS && lhs, ExprTypeRHS && rhs){
//    return binary_sub_tensor_simplify(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
//  }
//};
} // NAMESPACE symTM

#endif // TENSOR_SCALAR_OPERATORS_H
