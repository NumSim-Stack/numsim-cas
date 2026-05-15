#ifndef NUMSIM_CAS_TENSOR_OPERATORS_H
#define NUMSIM_CAS_TENSOR_OPERATORS_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/make_constant.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/core/promote_expr.h>

#include <numsim_cas/scalar/scalar_globals.h>
#include <numsim_cas/tensor/operators/tensor/tensor_add.h>
#include <numsim_cas/tensor/simplifier/tensor_simplifier_add.h>
#include <numsim_cas/tensor/simplifier/tensor_simplifier_mul.h>
#include <numsim_cas/tensor/simplifier/tensor_simplifier_sub.h>
#include <numsim_cas/tensor/simplifier/tensor_with_scalar_simplifier_mul.h>
#include <numsim_cas/tensor/tensor_zero.h>

namespace numsim::cas::detail {

// True iff `a` is trans(X), i.e. a basis_change_imp with index pattern {2,1}
// over an inner expression matching `inner_target`. Used by the add and sub
// operators to recognize trans(A) ± A (or its commutation) and annotate the
// result as Skew at construction time.
inline bool
is_trans_of(expression_holder<tensor_expression> const &a,
            expression_holder<tensor_expression> const &inner_target) {
  if (!is_same<basis_change_imp>(a))
    return false;
  auto const &bc = a.template get<basis_change_imp>();
  if (bc.indices() != sequence{2, 1})
    return false;
  return bc.expr() == inner_target;
}

// scalar binary ops
template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<tensor_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<tensor_expression>>
inline expression_holder<tensor_expression> tag_invoke(add_fn, L &&lhs,
                                                       R &&rhs) {
  if (is_same<tensor_zero>(lhs))
    return std::forward<R>(rhs);
  if (is_same<tensor_zero>(rhs))
    return std::forward<L>(lhs);

  if (is_same<tensor_negative>(rhs) &&
      lhs == rhs.template get<tensor_negative>().expr()) {
    return make_expression<tensor_zero>(lhs.get().dim(), lhs.get().rank());
  }

  if (is_same<tensor_negative>(lhs) &&
      rhs == lhs.template get<tensor_negative>().expr()) {
    return make_expression<tensor_zero>(lhs.get().dim(), lhs.get().rank());
  }

  // trans(A) + (-A) and (-A) + trans(A) are skew-symmetric in any dimension
  // (same algebra as trans(A) - A). Mirrors the sub branch annotation so
  // structural pattern fast-paths (e.g. skew(skew_expr) -> skew_expr) fire
  // regardless of which spelling the user wrote.
  if (lhs.get().rank() == 2) {
    auto is_trans_of_neg = [](auto const &a, auto const &b) {
      if (!is_same<tensor_negative>(b))
        return false;
      return is_trans_of(a, b.template get<tensor_negative>().expr());
    };
    if (is_trans_of_neg(lhs, rhs) || is_trans_of_neg(rhs, lhs)) {
      auto &_lhs{lhs.template get<tensor_visitable_t>()};
      simplifier::tensor_detail::add_base visitor(std::forward<L>(lhs),
                                                  std::forward<R>(rhs));
      auto result = _lhs.accept(visitor);
      // See sub branch: set_space post-construction is safe today because the
      // n_ary_tree hash excludes the space annotation.
      if (result.is_valid())
        result.data()->set_space({Skew{}, AnyTraceTag{}});
      return result;
    }
  }

  auto &_lhs{lhs.template get<tensor_visitable_t>()};
  simplifier::tensor_detail::add_base visitor(std::forward<L>(lhs),
                                              std::forward<R>(rhs));
  return _lhs.accept(visitor);
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<tensor_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<tensor_expression>>
inline expression_holder<tensor_expression> tag_invoke(sub_fn, L &&lhs,
                                                       R &&rhs) {
  if (is_same<tensor_zero>(rhs))
    return std::forward<L>(lhs);
  if (is_same<tensor_zero>(lhs))
    return -std::forward<R>(rhs);
  if (lhs == rhs) {
    return make_expression<tensor_zero>(lhs.get().dim(), lhs.get().rank());
  }
  // trans(A) - A and A - trans(A) are skew-symmetric in any dimension by
  // definition: ((trans(A)-A))^T = A - trans(A) = -(trans(A)-A). Annotate
  // unconditionally; inv()'s rejection of singular skew is dim-gated where
  // it belongs.
  if (lhs.get().rank() == 2) {
    if (is_trans_of(lhs, rhs) || is_trans_of(rhs, lhs)) {
      auto &_lhs{lhs.template get<tensor_visitable_t>()};
      tensor_detail::simplifier::sub_base visitor(std::forward<L>(lhs),
                                                  std::forward<R>(rhs));
      auto result = _lhs.accept(visitor);
      // set_space mutates the expression node after construction. Safe today
      // because the n_ary_tree hash excludes the space annotation; if that
      // ever changes, this must move before any operation that may finalize
      // the hash.
      if (result.is_valid())
        result.data()->set_space({Skew{}, AnyTraceTag{}});
      return result;
    }
  }
  auto &_lhs{lhs.template get<tensor_visitable_t>()};
  tensor_detail::simplifier::sub_base visitor(std::forward<L>(lhs),
                                              std::forward<R>(rhs));
  return _lhs.accept(visitor);
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<tensor_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<tensor_expression>>
inline expression_holder<tensor_expression>
tag_invoke(mul_fn, L &&lhs, [[maybe_unused]] R &&rhs) {
  if (is_same<tensor_zero>(lhs) || is_same<tensor_zero>(rhs)) {
    const auto rank{rhs.get().rank() + lhs.get().rank() - 2};
    return make_expression<tensor_zero>(lhs.get().dim(), rank);
  }
  auto &_lhs{lhs.template get<tensor_visitable_t>()};
  tensor_detail::simplifier::mul_base visitor(std::forward<L>(lhs),
                                              std::forward<R>(rhs));
  return _lhs.accept(visitor);
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<tensor_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<scalar_expression>>
inline expression_holder<tensor_expression> tag_invoke(mul_fn, L &&lhs,
                                                       R &&rhs) {
  if (is_same<tensor_zero>(lhs) ||
      (is_same<scalar_zero>(rhs) ||
       (is_same<scalar_constant>(rhs) &&
        rhs.template get<scalar_constant>().value() == 0))) {
    return make_expression<tensor_zero>(lhs.get().dim(), lhs.get().rank());
  }
  if (is_same<scalar_one>(rhs) ||
      (is_same<scalar_constant>(rhs) &&
       rhs.template get<scalar_constant>().value() == 1)) {
    return std::forward<L>(lhs);
  }

  auto &_lhs{lhs.template get<tensor_visitable_t>()};
  tensor_with_scalar_detail::simplifier::mul_base visitor(std::forward<R>(rhs),
                                                          std::forward<L>(lhs));
  return _lhs.accept(visitor);
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<scalar_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<tensor_expression>>
inline expression_holder<tensor_expression> tag_invoke(mul_fn, L &&lhs,
                                                       R &&rhs) {
  if (is_same<tensor_zero>(rhs) ||
      (is_same<scalar_zero>(lhs) ||
       (is_same<scalar_constant>(lhs) &&
        lhs.template get<scalar_constant>().value() == 0))) {
    return make_expression<tensor_zero>(rhs.get().dim(), rhs.get().rank());
  }
  if (is_same<scalar_one>(lhs) ||
      (is_same<scalar_constant>(lhs) &&
       lhs.template get<scalar_constant>().value() == 1)) {
    return std::forward<R>(rhs);
  }

  auto &_rhs{rhs.template get<tensor_visitable_t>()};
  tensor_with_scalar_detail::simplifier::mul_base visitor(std::forward<L>(lhs),
                                                          std::forward<R>(rhs));
  return _rhs.accept(visitor);
}

template <class L, class R>
requires std::same_as<std::remove_cvref_t<L>,
                      expression_holder<tensor_expression>> &&
         std::same_as<std::remove_cvref_t<R>,
                      expression_holder<scalar_expression>>
inline expression_holder<tensor_expression> tag_invoke(div_fn, L &&lhs,
                                                       R &&rhs) {
  if (is_same<tensor_zero>(lhs)) {
    return lhs;
  }
  if (is_same<scalar_one>(rhs) ||
      (is_same<scalar_constant>(rhs) &&
       rhs.template get<scalar_constant>().value() == 1)) {
    return std::forward<L>(lhs);
  }
  return lhs * pow(rhs, -get_scalar_one());
}

template <class T>
requires std::is_arithmetic_v<std::remove_cvref_t<T>>
expression_holder<scalar_expression>
tag_invoke(make_constant_fn, std::type_identity<tensor_expression>, T &&v) {
  return tag_invoke(make_constant_fn{}, std::type_identity<scalar_expression>{},
                    std::forward<T>(v));
}

} // namespace numsim::cas::detail

#endif // NUMSIM_CAS_TENSOR_OPERATORS_H
