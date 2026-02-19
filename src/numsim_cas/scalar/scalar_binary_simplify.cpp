#include <numsim_cas/scalar/scalar_binary_simplify_fwd.h>
#include <numsim_cas/scalar/simplifier/scalar_simplifier_pow.h>

namespace numsim::cas {

expression_holder<scalar_expression>
binary_scalar_pow_simplify(expression_holder<scalar_expression> lhs,
                           expression_holder<scalar_expression> rhs) {
  auto &_lhs{lhs.template get<scalar_visitable_t>()};
  simplifier::pow_base visitor(std::move(lhs), std::move(rhs));
  return _lhs.accept(visitor);
}

} // namespace numsim::cas
