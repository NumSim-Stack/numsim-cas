#ifndef SCALAR_FUNCTION_SIMPLIFIER_H
#define SCALAR_FUNCTION_SIMPLIFIER_H

#include <numsim_cas/scalar/visitors/scalar_rebuild_visitor.h>

namespace numsim::cas {

// Opt-in rewrite pass (#417): applies the branch-safe mixed inverse-trig
// identities (cos(asin x) → √(1-x²), sin(acos x) → √(1-x²), ...) throughout an
// expression. These expand node count, so they are deliberately NOT applied at
// construction — run this pass explicitly to opt in. Parallels
// `tensor_projector_simplifier`. The rewrites themselves are the
// `scalar_rules::try_*_of_*` contract rules; this pass only drives them.
class scalar_function_simplifier : public scalar_rebuild_visitor {
public:
  expr_holder_t apply(expr_holder_t const &expr) override;

  void operator()(scalar_sin const &v) override;
  void operator()(scalar_cos const &v) override;
  void operator()(scalar_tan const &v) override;
};

} // namespace numsim::cas

#endif // SCALAR_FUNCTION_SIMPLIFIER_H
