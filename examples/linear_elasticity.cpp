// Linear elasticity: a small but realistic constitutive model.
//
// Given a (small-strain) strain tensor ε and Lamé parameters λ, μ, this
// example shows the canonical symbolic CAS workflow:
//
//   1. Build the strain energy ψ(ε) = λ/2 (tr ε)² + μ (ε : ε).
//   2. Differentiate symbolically: stress σ = ∂ψ/∂ε.
//   3. Numerically evaluate σ at a sample strain.
//
// The expected closed-form result is σ = λ·tr(ε)·I + 2μ·ε.

#include <numsim_cas/numsim_cas.h>
#include <numsim_cas/tensor/data/tensor_data.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor/visitors/tensor_evaluator.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_diff.h>

#include <cstddef>
#include <iostream>
#include <memory>
#include <tuple>

using namespace numsim::cas;

template <std::size_t Dim, std::size_t Rank>
auto make_test_data(std::initializer_list<double> values) {
  auto ptr = std::make_shared<tensor_data<double, Dim, Rank>>();
  auto *raw = ptr->raw_data();
  std::size_t i = 0;
  for (auto v : values)
    raw[i++] = v;
  return ptr;
}

void print_matrix_3x3(double const *raw) {
  for (int i = 0; i < 3; ++i) {
    std::cout << "  [";
    for (int j = 0; j < 3; ++j) {
      if (j > 0)
        std::cout << ", ";
      std::cout << raw[i * 3 + j];
    }
    std::cout << "]\n";
  }
}

int main() {
  std::cout << "=== Linear Elasticity Constitutive Model ===\n\n";

  // Symbolic inputs: strain ε (rank-2 in 3D) + Lamé parameters λ, μ.
  auto eps = make_expression<tensor>("eps", 3, 2);
  auto [lambda, mu] = make_scalar_variable("lambda", "mu");
  auto _2 = make_scalar_constant(2);

  // Strain energy: ψ(ε) = λ/2 (tr ε)² + μ (ε : ε)
  // tr(ε) is t2s; the squaring is via t2s × t2s product. ε:ε is the
  // double-contraction `dot(ε)` returning a t2s.
  auto tr_eps = trace(eps);
  auto eps_dot_eps = dot(eps);                 // ε : ε
  auto psi = (lambda / _2) * (tr_eps * tr_eps) // λ/2 (tr ε)²
             + mu * eps_dot_eps;               // + μ (ε : ε)

  std::cout << "Strain energy ψ(ε) = " << to_string(psi) << "\n\n";

  // Symbolic stress: σ = ∂ψ/∂ε
  auto sigma = diff(psi, eps);
  std::cout << "Stress σ = ∂ψ/∂ε =\n  " << to_string(sigma) << "\n\n";

  // Numerical evaluation.
  // Sample strain: ε = [[1, 2, 0], [2, 3, 0], [0, 0, 4]] · 1e-3
  // Lamé: λ = 115.4 GPa, μ = 76.9 GPa (steel-ish).
  tensor_evaluator<double> ev;
  // clang-format off
  ev.set(eps, make_test_data<3, 2>({0.001, 0.002, 0.000,
                                     0.002, 0.003, 0.000,
                                     0.000, 0.000, 0.004}));
  // clang-format on
  ev.set_scalar(lambda, 115.4e9);
  ev.set_scalar(mu, 76.9e9);

  auto sigma_num = ev.apply(sigma);
  std::cout << "Numerical σ at sample strain:\n";
  print_matrix_3x3(sigma_num->raw_data());

  // Hand-check: σ = λ·tr(ε)·I + 2μ·ε
  // tr(ε) = 0.001 + 0.003 + 0.004 = 0.008
  // λ·tr(ε) = 115.4e9 * 0.008 = 9.232e8
  // 2μ·ε[0][0] = 2·76.9e9 · 0.001 = 1.538e8
  // σ[0][0] = 9.232e8 + 1.538e8 = 1.077e9
  std::cout << "\nExpected σ[0][0] = λ·tr(ε) + 2μ·ε[0][0] = "
            << 115.4e9 * 0.008 + 2 * 76.9e9 * 0.001 << "\n";

  return 0;
}
