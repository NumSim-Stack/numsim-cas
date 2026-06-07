// Assumptions basics: the SymPy-style assumption system.
//
// User-asserted facts (positive, integer, symmetric, positive_definite, ...)
// flow into the simplifier and the is_* query helpers. This example
// walks through the canonical scalar and tensor usage patterns:
//
//   1. Fluent variadic API:  x.assumption(positive{}, integer{})
//   2. Chaining:              A.assumption(Sym).assumption(PD)
//   3. Implication chains:    positive → nonzero → real_tag
//   4. Throw-on-non-Symbol:   compounds and constants reject assertions
//   5. Closed-form constants: scalar_constant(5) self-classifies
//
// The legacy assume(holder, tag) / assume_*(holder) helpers still work
// but are now [[deprecated]] — use expression_holder::assumption().

#include <numsim_cas/numsim_cas.h>

#include <iostream>
#include <stdexcept>
#include <tuple>

using namespace numsim::cas;

int main() {
  std::cout << "=== Assumptions Basics ===\n\n";

  // ── 1. Scalar: single-fact and multi-fact assertion ────────────────
  std::cout << "--- Scalar single + multi-fact ---\n";
  auto [x, y] = make_scalar_variable("x", "y");

  x.assumption(positive{});
  std::cout << "x.assumption(positive{}):\n"
            << "  is_positive(x)    = " << is_positive(x) << "\n"
            << "  is_nonnegative(x) = " << is_nonnegative(x) << "  (implied)\n"
            << "  is_nonzero(x)     = " << is_nonzero(x) << "  (implied)\n"
            << "  is_real(x)        = " << is_real(x) << "  (implied)\n\n";

  y.assumption(positive{}, integer{});
  std::cout << "y.assumption(positive{}, integer{}):\n"
            << "  is_positive(y) = " << is_positive(y) << "\n"
            << "  is_integer(y)  = " << is_integer(y) << "\n"
            << "  is_rational(y) = " << is_rational(y)
            << "  (integer⇒rational)\n"
            << "  is_real(y)     = " << is_real(y) << "  (chain)\n\n";

  // ── 2. Chaining (returns *this) ────────────────────────────────────
  std::cout << "--- Chaining ---\n";
  auto [z] = make_scalar_variable("z");
  z.assumption(prime{}).assumption(real_tag{});
  std::cout << "z.assumption(prime{}).assumption(real_tag{}):\n"
            << "  is_prime via assumptions = "
            << z.get().assumptions().contains(prime{}) << "\n"
            << "  is_integer(z) = " << is_integer(z) << "  (prime⇒integer)\n\n";

  // ── 3. Tensor: structural + algebraic facts ────────────────────────
  std::cout << "--- Tensor structural + algebraic ---\n";
  auto [A] = make_tensor_variable(std::tuple{"A", 3, 2});
  A.assumption(Symmetric{}, positive_definite{});
  std::cout << "A.assumption(Symmetric{}, positive_definite{}):\n"
            << "  is_symmetric(A)            = " << is_symmetric(A) << "\n"
            << "  is_positive_definite(A)    = " << is_positive_definite(A)
            << "\n"
            << "  is_positive_semidefinite(A) = " << is_positive_semidefinite(A)
            << "  (PD⇒PSD)\n\n";

  // ── 4. Non-Symbol assertions throw ─────────────────────────────────
  std::cout << "--- Throw on non-Symbol ---\n";
  auto compound = x + y;
  try {
    compound.assumption(positive{});
    std::cout << "FAIL: expected throw on compound\n";
    return 1;
  } catch (invalid_assumption_error const &e) {
    std::cout << "compound.assumption(positive{}) threw as expected:\n"
              << "  " << e.what() << "\n\n";
  }

  // ── 5. Closed-form constants self-classify ─────────────────────────
  std::cout << "--- Closed-form constants ---\n";
  auto c5 = make_expression<scalar_constant>(5);
  auto c_neg = make_expression<scalar_constant>(-2.5);
  std::cout << "scalar_constant(5):\n"
            << "  is_positive  = " << is_positive(c5) << "\n"
            << "  is_integer   = " << is_integer(c5) << "\n";
  std::cout << "scalar_constant(-2.5):\n"
            << "  is_negative  = " << is_negative(c_neg) << "\n"
            << "  is_integer   = " << is_integer(c_neg)
            << "  (non-zero doubles don't claim integer)\n\n";

  auto I = make_expression<identity_tensor>(std::size_t{3}, std::size_t{2});
  std::cout << "identity_tensor(3, 2): is_symmetric = " << is_symmetric(I)
            << "  (ctor pre-annotates Sym)\n\n";

  // ── 6. Propagation through compound tensors ────────────────────────
  std::cout << "--- Compound propagation ---\n";
  auto [B] = make_tensor_variable(std::tuple{"B", 3, 2});
  B.assumption(Symmetric{});
  auto sum = A + B;
  std::cout << "is_symmetric(A + B) = " << is_symmetric(sum)
            << "  (n_ary_tree joins both children's Sym tag)\n";

  std::cout << "\nAll assertions passed.\n";
  return 0;
}
