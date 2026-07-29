#include <numsim_cas/scalar/simplifier/scalar_function_simplifier.h>

#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/scalar/simplifier/scalar_function_rules.h>

namespace numsim::cas {

scalar_function_simplifier::expr_holder_t
scalar_function_simplifier::apply(expr_holder_t const &expr) {
  return scalar_rebuild_visitor::apply(expr);
}

void scalar_function_simplifier::operator()(scalar_sin const &v) {
  auto arg = apply(v.expr());
  if (auto r = scalar_rules::try_sin_of_acos(arg)) {
    m_result = std::move(*r);
    return;
  }
  if (auto r = scalar_rules::try_sin_of_atan(arg)) {
    m_result = std::move(*r);
    return;
  }
  m_result = sin(std::move(arg));
}

void scalar_function_simplifier::operator()(scalar_cos const &v) {
  auto arg = apply(v.expr());
  if (auto r = scalar_rules::try_cos_of_asin(arg)) {
    m_result = std::move(*r);
    return;
  }
  if (auto r = scalar_rules::try_cos_of_atan(arg)) {
    m_result = std::move(*r);
    return;
  }
  m_result = cos(std::move(arg));
}

void scalar_function_simplifier::operator()(scalar_tan const &v) {
  auto arg = apply(v.expr());
  if (auto r = scalar_rules::try_tan_of_asin(arg)) {
    m_result = std::move(*r);
    return;
  }
  m_result = tan(std::move(arg));
}

} // namespace numsim::cas
