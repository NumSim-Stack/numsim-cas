#ifndef TENSOR_ASSUME_H
#define TENSOR_ASSUME_H

#include <numsim_cas/tensor/projector_algebra.h>
#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

// --- Set assumptions ---

inline void
assume_symmetric(expression_holder<tensor_expression> const &expr) {
  expr.data()->set_space({Symmetric{}, AnyTraceTag{}});
}

inline void assume_skew(expression_holder<tensor_expression> const &expr) {
  expr.data()->set_space({Skew{}, AnyTraceTag{}});
}

inline void
assume_volumetric(expression_holder<tensor_expression> const &expr) {
  expr.data()->set_space({Symmetric{}, VolumetricTag{}});
}

inline void
assume_deviatoric(expression_holder<tensor_expression> const &expr) {
  expr.data()->set_space({Symmetric{}, DeviatoricTag{}});
}

// --- Query assumptions ---

inline bool is_symmetric(expression_holder<tensor_expression> const &expr) {
  auto const &sp = expr.get().space();
  if (!sp)
    return false;
  auto kind = classify_space(*sp);
  // Sym, Vol, Dev are all subspaces of Sym
  return kind == ProjKind::Sym || kind == ProjKind::Vol ||
         kind == ProjKind::Dev;
}

inline bool is_skew(expression_holder<tensor_expression> const &expr) {
  auto const &sp = expr.get().space();
  if (!sp)
    return false;
  return classify_space(*sp) == ProjKind::Skew;
}

inline bool is_volumetric(expression_holder<tensor_expression> const &expr) {
  auto const &sp = expr.get().space();
  if (!sp)
    return false;
  return classify_space(*sp) == ProjKind::Vol;
}

inline bool is_deviatoric(expression_holder<tensor_expression> const &expr) {
  auto const &sp = expr.get().space();
  if (!sp)
    return false;
  return classify_space(*sp) == ProjKind::Dev;
}

} // namespace numsim::cas

#endif // TENSOR_ASSUME_H
