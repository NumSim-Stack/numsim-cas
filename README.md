# NumSim-CAS

A C++23 computer algebra system for symbolic tensor calculus, automatic differentiation, and numerical evaluation.

## Features

- **Three expression domains**: scalar, rank-N tensor, and tensor-to-scalar
- **Automatic simplification**: identity laws, constant folding, like-term merging, power rules
- **Symbolic differentiation**: Leibniz rule, chain rule, product/quotient rules
- **Numerical evaluation**: template-based evaluator with [tmech](https://github.com/petlenz/tmech) tensor backend
- **Projection tensor algebra**: `sym`, `dev`, `vol`, `skew` decompositions with idempotence, orthogonality, and subspace rules
- **Tensor space assumptions**: `assume_symmetric`, `assume_deviatoric`, etc. with propagation through derived expressions

## Quick Start

```cpp
#include <numsim_cas/numsim_cas.h>
#include <iostream>

using namespace numsim::cas;
using std::pow;
using std::sin;

int main() {
    // Symbolic variables and constants
    auto [x, y] = make_scalar_variable("x", "y");
    auto _2 = make_scalar_constant(2);

    // Expressions simplify automatically
    auto f = pow(x, _2) + _2 * x * y;
    std::cout << "f     = " << to_string(f) << "\n";

    // Symbolic differentiation
    auto df = diff(f, x);
    std::cout << "df/dx = " << to_string(df) << "\n";

    // Numerical evaluation
    scalar_evaluator<double> ev;
    ev.set(x, 3.0);
    ev.set(y, 2.0);
    std::cout << "f(3,2) = " << ev.apply(f) << "\n";
}
```

## Tensor Example

```cpp
#include <numsim_cas/numsim_cas.h>

using namespace numsim::cas;

int main() {
    // Rank-2 tensors in 3D
    auto [A, B] = make_tensor_variable(
        std::tuple{"A", 3, 2}, std::tuple{"B", 3, 2});
    auto [x] = make_scalar_variable("x");

    // Arithmetic and projections
    auto expr = x * A + B;
    std::cout << to_string(sym(expr)) << "\n";   // symmetric part
    std::cout << to_string(dev(A)) << "\n";       // deviatoric part

    // Projector algebra simplifies at construction time
    std::cout << to_string(dev(dev(A))) << "\n";        // dev(A)
    std::cout << to_string(vol(dev(A))) << "\n";        // 0
    std::cout << to_string(vol(A) + dev(A)) << "\n";    // sym(A)
    std::cout << to_string(sym(A) + skew(A)) << "\n";   // A

    // Tensor space assumptions
    assume_symmetric(A);
    std::cout << to_string(sym(A)) << "\n";  // A (simplified)
    std::cout << to_string(skew(A)) << "\n"; // 0

    // Inner products and contractions
    using seq = sequence;
    auto C = inner_product(A, seq{2}, B, seq{1});  // matrix multiply
    std::cout << to_string(trace(A)) << "\n";
    std::cout << to_string(det(A)) << "\n";
}
```

## Building

Requires a C++23 compiler (GCC 14+, Clang 18+).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```

### Options

| Option | Default | Description |
|--------|---------|-------------|
| `NUMSIM_CAS_BUILD_TESTS` | `ON` | Build test suite |
| `NUMSIM_CAS_BUILD_EXAMPLES` | `OFF` | Build example programs |
| `NUMSIM_CAS_BUILD_BENCHMARK` | `OFF` | Build benchmarks |
| `NUMSIM_CAS_SANITIZERS` | `OFF` | Enable ASAN + UBSAN |

### Dependencies

- [tmech](https://github.com/petlenz/tmech) -- fetched automatically via CMake FetchContent
- [GoogleTest](https://github.com/google/googletest) -- fetched automatically for tests

## Architecture

```
scalar_expression ── scalar nodes, simplifiers, visitors
tensor_expression ── tensor nodes, simplifiers, visitors, projector algebra
tensor_to_scalar  ── cross-domain nodes (trace, det, norm, dot)
core/             ── shared infrastructure (n_ary_tree, domain_traits, visitors)
```

Each domain follows the same pattern: expression base class, node types, a virtual visitor for dispatch, and construction-time simplifiers that produce canonical forms.

## License

NumSim-CAS is available under a dual-license model:

- **Open Source:** GNU General Public License v3.0 (GPL-3.0-only) — see [LICENSE](LICENSE)
- **Commercial:** A separate commercial license is available for proprietary/closed-source use — contact <YOUR_EMAIL>.

If you want to use NumSim-CAS in a closed-source product, you must obtain a commercial license.
