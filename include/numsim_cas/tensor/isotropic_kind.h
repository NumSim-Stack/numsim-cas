#ifndef NUMSIM_CAS_ISOTROPIC_KIND_H
#define NUMSIM_CAS_ISOTROPIC_KIND_H

#include <string_view>

namespace numsim::cas {

// Which scalar function an isotropic tensor function applies to the
// eigenvalues (#227): f(A) = Σ f(λ_i) E_i.
enum class isotropic_kind { log, exp, sqrt };

[[nodiscard]] constexpr std::string_view name(isotropic_kind k) noexcept {
  switch (k) {
  case isotropic_kind::log:
    return "log";
  case isotropic_kind::exp:
    return "exp";
  case isotropic_kind::sqrt:
    return "sqrt";
  }
  return "?";
}

} // namespace numsim::cas

#endif // NUMSIM_CAS_ISOTROPIC_KIND_H
