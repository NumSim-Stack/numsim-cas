#ifndef SCALAR_FUNCTION_SIMPLIFIER_H
#define SCALAR_FUNCTION_SIMPLIFIER_H

#include <numsim_cas/scalar/visitors/scalar_rebuild_visitor.h>

namespace numsim::cas {

// Opt-in pass for the mixed inverse-trig rules (cos(asin x) → √(1-x²), ...),
// which expand node count and so are not applied at construction.
class scalar_function_simplifier : public scalar_rebuild_visitor {
public:
  expr_holder_t apply(expr_holder_t const &expr) override;

  void operator()(scalar_sin const &v) override;
  void operator()(scalar_cos const &v) override;
  void operator()(scalar_tan const &v) override;
};

} // namespace numsim::cas

#endif // SCALAR_FUNCTION_SIMPLIFIER_H
