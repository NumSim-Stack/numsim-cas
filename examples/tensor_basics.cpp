#include <numsim_cas/numsim_cas.h>
#include <numsim_cas/tensor/data/tensor_data.h>
#include <numsim_cas/tensor/visitors/tensor_evaluator.h>

#include <cstddef>
#include <iostream>
#include <memory>
#include <tuple>

using namespace numsim::cas;

// Helper: create tensor data from an initializer list of raw values
template <std::size_t Dim, std::size_t Rank>
auto make_test_data(std::initializer_list<double> values) {
  auto ptr = std::make_shared<tensor_data<double, Dim, Rank>>();
  auto *raw = ptr->raw_data();
  std::size_t i = 0;
  for (auto v : values)
    raw[i++] = v;
  return ptr;
}

// Helper: print a 3x3 matrix stored in row-major order
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
  // Create rank-2 tensor variables in 3D
  auto [X, Y] =
      make_tensor_variable(std::tuple{"X", 3, 2}, std::tuple{"Y", 3, 2});

  // Create scalar variables and constants
  auto [x] = make_scalar_variable("x");
  auto [_2, _3] = make_scalar_constant(2, 3);

  std::cout << "=== Tensor Basics ===\n\n";

  // --- Arithmetic ---
  std::cout << "--- Arithmetic ---\n";
  std::cout << "X + Y      = " << to_string(X + Y) << "\n";
  std::cout << "X - Y      = " << to_string(X - Y) << "\n";
  std::cout << "-X         = " << to_string(-X) << "\n";
  std::cout << "x * X      = " << to_string(x * X) << "\n";
  std::cout << "X / x      = " << to_string(X / x) << "\n";
  std::cout << "X + X      = " << to_string(X + X) << "\n";

  // --- Matrix operations ---
  std::cout << "\n--- Matrix Operations ---\n";
  using seq = sequence;
  std::cout << "X * Y (contract idx 2,1) = "
            << to_string(inner_product(X, seq{2}, Y, seq{1})) << "\n";
  std::cout << "trans(X)                 = " << to_string(trans(X)) << "\n";
  std::cout << "inv(X)                   = " << to_string(inv(X)) << "\n";
  std::cout << "X * X  = pow(X,2)        = " << to_string(X * X) << "\n";
  std::cout << "X * X * X = pow(X,3)     = " << to_string(X * X * X) << "\n";

  // --- Projection decompositions ---
  std::cout << "\n--- Projections ---\n";
  std::cout << "dev(X)     = " << to_string(dev(X)) << "\n";
  std::cout << "sym(X)     = " << to_string(sym(X)) << "\n";
  std::cout << "vol(X)     = " << to_string(vol(X)) << "\n";
  std::cout << "skew(X)    = " << to_string(skew(X)) << "\n";

  // --- Projector algebra (construction-time simplification) ---
  std::cout << "\n--- Projector Algebra ---\n";
  std::cout << "dev(dev(X))      = " << to_string(dev(dev(X)))
            << "  (idempotent)\n";
  std::cout << "vol(dev(X))      = " << to_string(vol(dev(X)))
            << "  (orthogonal -> zero)\n";
  std::cout << "vol(X) + dev(X)  = " << to_string(vol(X) + dev(X))
            << "  (complementary -> sym)\n";
  std::cout << "sym(X) + skew(X) = " << to_string(sym(X) + skew(X))
            << "  (full decomposition -> X)\n";

  // --- Numeric evaluation ---
  std::cout << "\n--- Numeric Evaluation ---\n";
  {
    tensor_evaluator<double> ev;

    // X = [1 2 0; 3 4 0; 0 0 5], Y = [5 6 0; 7 8 0; 0 0 1]
    // clang-format off
    ev.set(X, make_test_data<3, 2>({1.0, 2.0, 0.0,
                                     3.0, 4.0, 0.0,
                                     0.0, 0.0, 5.0}));
    ev.set(Y, make_test_data<3, 2>({5.0, 6.0, 0.0,
                                     7.0, 8.0, 0.0,
                                     0.0, 0.0, 1.0}));
    // clang-format on
    ev.set_scalar(x, 2.0);

    // X + Y
    auto sum = ev.apply(X + Y);
    std::cout << "X + Y =\n";
    print_matrix_3x3(sum->raw_data());

    // trans(X)
    auto t = ev.apply(trans(X));
    std::cout << "\ntrans(X) =\n";
    print_matrix_3x3(t->raw_data());

    // x * X  (x = 2)
    auto scaled = ev.apply(x * X);
    std::cout << "\n2 * X =\n";
    print_matrix_3x3(scaled->raw_data());

    // X * Y (matrix multiply)
    auto product = ev.apply(inner_product(X, seq{2}, Y, seq{1}));
    std::cout << "\nX * Y =\n";
    print_matrix_3x3(product->raw_data());
    std::cout << "  (expected: [19 22 0; 43 50 0; 0 0 5])\n";

    // dev(X)
    auto devX = ev.apply(dev(X));
    std::cout << "\ndev(X) =\n";
    print_matrix_3x3(devX->raw_data());
  }
}
