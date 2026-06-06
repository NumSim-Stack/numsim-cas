#ifndef SCALAR_CONSTANT_H
#define SCALAR_CONSTANT_H

#include <numsim_cas/core/assumptions.h>
#include <numsim_cas/core/hash_functions.h>
#include <numsim_cas/core/scalar_number.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_constant final : public scalar_node_base_t<scalar_constant> {
public:
  using base = scalar_node_base_t<scalar_constant>;
  scalar_constant() = delete;
  template <typename T>
  explicit scalar_constant(T const &v) : base(), m_value(v) {
    // SymPy-style closed-form constant: derive numeric assumptions from
    // the value at construction time (same pattern as
    // tensor_to_scalar_zero/one). The user can no longer call
    // assume(c, positive{}) on a literal (step 4 rejects non-Symbols),
    // and they shouldn't need to — a literal 5 IS positive by value.
    annotate_from_value();
  }
  scalar_constant(scalar_constant const &) = default;
  scalar_constant(scalar_constant &&) noexcept = default;
  scalar_constant &operator=(scalar_constant const &) = delete;
  scalar_constant &operator=(scalar_constant &&) noexcept = delete;
  ~scalar_constant() override = default;

  auto const &value() const noexcept { return m_value; }

  friend inline bool operator==(scalar_constant const &a,
                                scalar_constant const &b) {
    return a.value() == b.value();
  }
  friend inline bool operator!=(scalar_constant const &a,
                                scalar_constant const &b) {
    return !(a == b);
  }
  friend inline bool operator<(scalar_constant const &a,
                               scalar_constant const &b) {
    return a.value() < b.value();
  }
  friend inline bool operator>(scalar_constant const &a,
                               scalar_constant const &b) {
    return b < a;
  }

protected:
  void update_hash_value() const noexcept override {
    this->m_hash_value = 0;
    hash_combine(this->m_hash_value, this->id());
    std::visit([&](auto const &x) { hash_combine(this->m_hash_value, x); },
               m_value.raw());
  }

private:
  scalar_number m_value;

  // Derive numeric assumptions from the value. int64, double, rational
  // get sign + real classification; complex gets no sign predicates
  // (not orderable) and no real_tag.
  //
  // Zero handling: 0, 0.0, rational_t{0,1} all carry the same fact set,
  // including integer + rational. Zero is mathematically an integer
  // regardless of storage representation.
  //
  // Non-zero doubles deliberately do NOT claim integer — this is an
  // intent rule, not a representability rule. Many double literals
  // (1.0, 2.0, 1024.0, ...) are exact in IEEE 754, but a user who
  // writes `5.0` is signaling floating-point intent. Auto-promoting
  // representable integer-valued doubles to integer-domain would
  // invert that intent and surprise users who deliberately chose a
  // float spelling to avoid integer-only rewrites (e.g. `pow(x, 5.0)`
  // vs `pow(x, 5)`). SymPy follows the same convention.
  //
  // Note: NOT noexcept. std::set::insert can throw std::bad_alloc.
  // Callers (the ctor) propagate to the heap-exhaustion handler.
  void annotate_from_value() {
    auto &a = this->assumptions();
    std::visit(
        [&a](auto const &v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, std::int64_t>) {
            a.insert(integer{});
            a.insert(rational{});
            a.insert(real_tag{});
            if (v > 0) {
              a.insert(positive{});
              a.insert(nonnegative{});
              a.insert(nonzero{});
            } else if (v < 0) {
              a.insert(negative{});
              a.insert(nonpositive{});
              a.insert(nonzero{});
            } else {
              a.insert(nonnegative{});
              a.insert(nonpositive{});
            }
          } else if constexpr (std::is_same_v<T, double>) {
            a.insert(real_tag{});
            if (v > 0.0) {
              a.insert(positive{});
              a.insert(nonnegative{});
              a.insert(nonzero{});
            } else if (v < 0.0) {
              a.insert(negative{});
              a.insert(nonpositive{});
              a.insert(nonzero{});
            } else {
              // 0.0 case: align with int 0 — zero is integer + rational
              // regardless of spelling.
              a.insert(integer{});
              a.insert(rational{});
              a.insert(nonnegative{});
              a.insert(nonpositive{});
            }
          } else if constexpr (std::is_same_v<T, rational_t>) {
            a.insert(rational{});
            a.insert(real_tag{});
            if (v.den == 1)
              a.insert(integer{});
            // num/den signs: den > 0 invariant per scalar_number ctor.
            if (v.num > 0) {
              a.insert(positive{});
              a.insert(nonnegative{});
              a.insert(nonzero{});
            } else if (v.num < 0) {
              a.insert(negative{});
              a.insert(nonpositive{});
              a.insert(nonzero{});
            } else {
              a.insert(nonnegative{});
              a.insert(nonpositive{});
            }
          }
          // complex: no sign predicates apply; also not real.
        },
        m_value.raw());
    a.set_inferred();
  }
};

} // namespace numsim::cas

#endif // SCALAR_CONSTANT_H
